#include "server/config_validation/dispatcher.h"

#include "common/common/assert.h"

namespace Envoy {
namespace Event {

Network::ClientConnectionPtr
ValidationDispatcher::createClientConnection(Network::Address::InstanceConstSharedPtr,
                                             Network::Address::InstanceConstSharedPtr) {
  NOT_IMPLEMENTED;
}

Network::ClientConnectionPtr
ValidationDispatcher::createSslClientConnection(Ssl::ClientContext&,
                                                Network::Address::InstanceConstSharedPtr,
                                                Network::Address::InstanceConstSharedPtr) {
  NOT_IMPLEMENTED;
}

Network::DnsResolverSharedPtr ValidationDispatcher::createDnsResolver(
    const std::vector<Network::Address::InstanceConstSharedPtr>&) {
  NOT_IMPLEMENTED;
}

Network::ListenerPtr ValidationDispatcher::createListener(Network::ListenSocket&,
                                                          Network::ListenerCallbacks&, bool) {
  NOT_IMPLEMENTED;
}

} // namespace Event
} // namespace Envoy
