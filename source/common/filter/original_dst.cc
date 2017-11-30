#include "common/filter/original_dst.h"

#include "envoy/network/listen_socket.h"

#include "common/common/assert.h"
#include "common/network/utility.h"

namespace Envoy {
namespace Filter {

Network::Address::InstanceConstSharedPtr OriginalDst::getOriginalDst(int fd) {
  return Network::Utility::getOriginalDst(fd);
}

Network::FilterStatus OriginalDst::onAccept(Network::ListenerFilterCallbacks& cb) {
  ENVOY_LOG(info, "original_dst: New connection accepted");
  Network::AcceptSocket& socket = cb.socket();
  Network::Address::InstanceConstSharedPtr local_address = socket.localAddress();

  if (local_address->type() == Network::Address::Type::Ip) {
    Network::Address::InstanceConstSharedPtr original_local_address = getOriginalDst(socket.fd());

    // A listener that has the use_original_dst flag set to true can still receive
    // connections that are NOT redirected using iptables. If a connection was not redirected,
    // the address returned by getOriginalDst() matches the local address of the new socket.
    // In this case the listener handles the connection directly and does not hand it off.
    if (original_local_address && (*original_local_address != *local_address)) {
      socket.resetLocalAddress(original_local_address);
    }
  }

  return Network::FilterStatus::Continue;
}

} // namespace Filter
} // namespace Envoy
