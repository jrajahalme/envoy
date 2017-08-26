#pragma once

#include "envoy/event/dispatcher.h"

#include "common/event/dispatcher_impl.h"

namespace Envoy {
namespace Event {

/**
 * Config-validation-only implementation of Event::Dispatcher. This class delegates all calls to
 * Event::DispatcherImpl, except for the methods involved with network events. Those methods are
 * disallowed at validation time.
 */
class ValidationDispatcher : public DispatcherImpl {
public:
  Network::ClientConnectionPtr
      createClientConnection(Network::Address::InstanceConstSharedPtr,
                             Network::Address::InstanceConstSharedPtr) override;
  Network::ClientConnectionPtr
  createSslClientConnection(Ssl::ClientContext&, Network::Address::InstanceConstSharedPtr,
                            Network::Address::InstanceConstSharedPtr) override;
  Network::DnsResolverSharedPtr createDnsResolver(
      const std::vector<Network::Address::InstanceConstSharedPtr>& resolvers) override;
  Network::ListenerPtr createListener(Network::ListenSocket&, Network::ListenerCallbacks&,
                                      bool bind_to_port) override;
};

} // namespace Event
} // namespace Envoy
