#pragma once

#include <unistd.h>

#include <memory>
#include <string>

#include "envoy/network/connection.h"
#include "envoy/network/listen_socket.h"

#include "common/ssl/context_impl.h"

namespace Envoy {
namespace Network {

class ListenSocketImpl : public ListenSocket {
public:
  ~ListenSocketImpl() { close(); }

  // Network::ListenSocket
  Address::InstanceConstSharedPtr localAddress() const override { return local_address_; }
  int fd() override { return fd_; }

  void close() override {
    if (fd_ != -1) {
      ::close(fd_);
      fd_ = -1;
    }
  }

protected:
  void doBind();

  int fd_;
  Address::InstanceConstSharedPtr local_address_;
};

/**
 * Wraps a unix socket.
 */
class TcpListenSocket : public ListenSocketImpl {
public:
  TcpListenSocket(Address::InstanceConstSharedPtr address, bool bind_to_port);
  TcpListenSocket(int fd, Address::InstanceConstSharedPtr address);
};

typedef std::unique_ptr<TcpListenSocket> TcpListenSocketPtr;

class UdsListenSocket : public ListenSocketImpl {
public:
  UdsListenSocket(const std::string& uds_path);
};

class AcceptSocketImpl : public AcceptSocket {
public:
  AcceptSocketImpl(int fd, Address::InstanceConstSharedPtr&& local_address,
                   Address::InstanceConstSharedPtr&& remote_address)
      : fd_(fd), local_address_reset_(false), so_mark_(Network::SO_MARK_NONE),
        local_address_(std::move(local_address)), remote_address_(std::move(remote_address)) {}
  ~AcceptSocketImpl() { close(); }

  // Network::AcceptSocket
  Address::InstanceConstSharedPtr localAddress() const override { return local_address_; }
  void resetLocalAddress(Address::InstanceConstSharedPtr& local_address) override {
    local_address_ = local_address;
    local_address_reset_ = true;
  }
  bool localAddressReset() const override { return local_address_reset_; }
  Address::InstanceConstSharedPtr remoteAddress() const override { return remote_address_; }
  void resetRemoteAddress(Address::InstanceConstSharedPtr& remote_address) override {
    remote_address_ = remote_address;
  }
  uint32_t socketMark() const override { return so_mark_; }
  void setSocketMark(uint32_t so_mark) override { so_mark_ = so_mark; }
  int fd() const override { return fd_; }

  int takeFd() override {
    int fd = fd_;
    fd_ = -1;
    return fd;
  }

  void clearReset() override { local_address_reset_ = false; }

  void close() override {
    if (fd_ != -1) {
      ::close(fd_);
      fd_ = -1;
    }
  }

protected:
  int fd_;
  bool local_address_reset_;
  uint32_t so_mark_;
  Address::InstanceConstSharedPtr local_address_;
  Address::InstanceConstSharedPtr remote_address_;
};

} // namespace Network
} // namespace Envoy
