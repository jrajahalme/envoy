#pragma once

#include "envoy/network/filter.h"

#include "common/common/logger.h"

namespace Envoy {
namespace Filter {

/**
 * Implementation of an original destination listener filter.
 */
class OriginalDst : public Network::ListenerFilter, Logger::Loggable<Logger::Id::filter> {
public:
  // Network::ListenerFilter
  Network::FilterStatus onAccept(Network::ListenerFilterCallbacks& cb) override;

  virtual Network::Address::InstanceConstSharedPtr getOriginalDst(int fd);
};

} // namespace Filter
} // namespace Envoy
