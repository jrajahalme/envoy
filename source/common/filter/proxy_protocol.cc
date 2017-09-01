#include "common/filter/proxy_protocol.h"

#include <unistd.h>

#include <cstdint>
#include <memory>
#include <string>

#include "envoy/common/exception.h"
#include "envoy/event/dispatcher.h"
#include "envoy/network/listen_socket.h"
#include "envoy/stats/stats.h"

#include "common/common/assert.h"
#include "common/common/empty_string.h"
#include "common/common/utility.h"
#include "common/network/address_impl.h"
#include "common/network/utility.h"

namespace Envoy {
namespace Filter {
namespace ProxyProtocol {

Config::Config(Stats::Scope& scope) : stats_{ALL_PROXY_PROTOCOL_STATS(POOL_COUNTER(scope))} {}

Network::FilterStatus Instance::onAccept(Network::ListenerFilterCallbacks& cb) {
  ENVOY_LOG(info, "proxy_protocol: New connection accepted");
  Network::AcceptSocket& socket = cb.socket();
  ASSERT(file_event_.get() == nullptr);
  file_event_ =
      cb.dispatcher().createFileEvent(socket.fd(),
                                      [this](uint32_t events) {
                                        ASSERT(events == Event::FileReadyType::Read);
                                        UNREFERENCED_PARAMETER(events);
                                        onRead();
                                      },
                                      Event::FileTriggerType::Edge, Event::FileReadyType::Read);
  cb_ = &cb;
  return Network::FilterStatus::StopIteration;
}

void Instance::onRead() {
  try {
    onReadWorker();
  } catch (const EnvoyException& ee) {
    config_->stats_.downstream_cx_proxy_proto_error_.inc();
    cb_->continueFilterChain(false);
  }
}

void Instance::onReadWorker() {
  Network::AcceptSocket& socket = cb_->socket();
  std::string proxy_line;
  if (!readLine(socket.fd(), proxy_line)) {
    return;
  }

  // Remove the line feed at the end
  StringUtil::rtrim(proxy_line);

  // Parse proxy protocol line with format: PROXY TCP4/TCP6 SOURCE_ADDRESS DESTINATION_ADDRESS
  // SOURCE_PORT DESTINATION_PORT.
  const auto line_parts = StringUtil::split(proxy_line, " ", true);

  if (line_parts.size() != 6 || line_parts[0] != "PROXY") {
    throw EnvoyException("failed to read proxy protocol");
  }

  Network::Address::IpVersion protocol_version;
  Network::Address::InstanceConstSharedPtr remote_address;
  Network::Address::InstanceConstSharedPtr local_address;
  if (line_parts[1] == "TCP4") {
    protocol_version = Network::Address::IpVersion::v4;
    remote_address =
        Network::Utility::parseInternetAddressAndPort(line_parts[2] + ":" + line_parts[4]);
    local_address =
        Network::Utility::parseInternetAddressAndPort(line_parts[3] + ":" + line_parts[5]);
  } else if (line_parts[1] == "TCP6") {
    protocol_version = Network::Address::IpVersion::v6;
    remote_address =
        Network::Utility::parseInternetAddressAndPort("[" + line_parts[2] + "]:" + line_parts[4]);
    local_address =
        Network::Utility::parseInternetAddressAndPort("[" + line_parts[3] + "]:" + line_parts[5]);
  } else {
    throw EnvoyException("failed to read proxy protocol");
  }

  // Error check the source and destination fields.  Most errors are cought by the address
  // parsing above, but a malformed IPv6 address may combine with a malformed port and parse as
  // an IPv6 address when parsing for an IPv4 address.  Remote address refers to the source
  // address.
  const auto remote_version = remote_address->ip()->version();
  const auto local_version = local_address->ip()->version();
  if (remote_version != protocol_version || local_version != protocol_version) {
    throw EnvoyException("failed to read proxy protocol");
  }
  // Check that both addresses are valid unicast addresses, as required for TCP
  if (!remote_address->ip()->isUnicastAddress() || !local_address->ip()->isUnicastAddress()) {
    throw EnvoyException("failed to read proxy protocol");
  }
  socket.resetLocalAddress(local_address);
  socket.resetRemoteAddress(remote_address);

  cb_->continueFilterChain(true);
}

bool Instance::readLine(int fd, std::string& s) {
  while (buf_off_ < MAX_PROXY_PROTO_LEN) {
    ssize_t nread = recv(fd, buf_ + buf_off_, MAX_PROXY_PROTO_LEN - buf_off_, MSG_PEEK);

    if (nread == -1 && errno == EAGAIN) {
      return false;
    } else if (nread < 1) {
      throw EnvoyException("failed to read proxy protocol");
    }

    bool found = false;
    // continue searching buf_ from where we left off
    for (; search_index_ < buf_off_ + nread; search_index_++) {
      if (buf_[search_index_] == '\n' && buf_[search_index_ - 1] == '\r') {
        search_index_++;
        found = true;
        break;
      }
    }

    // Read the data upto and including the line feed, if available, but not past it.
    // This should never fail, as search_index_ - buf_off_ <= nread, so we're asking
    // only for bytes we have already seen.
    nread = recv(fd, buf_ + buf_off_, search_index_ - buf_off_, 0);
    ASSERT(size_t(nread) == search_index_ - buf_off_);

    buf_off_ += nread;

    if (found) {
      s.assign(buf_, buf_off_);
      return true;
    }
  }

  throw EnvoyException("failed to read proxy protocol");
}

} // namespace ProxyProtocol
} // namespace Filter
} // namespace Envoy
