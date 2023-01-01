#pragma once

#include "remote_base.h"

namespace esphome {
namespace remote_base {

struct Smoke558RFData {
  uint32_t device;
  uint8_t group;
  uint8_t payload;

  bool operator==(const Smoke558RFData &rhs) const {
    return device == rhs.device && group == rhs.group && payload == rhs.payload;
  }
};

class Smoke558RFProtocol : public RemoteProtocol<Smoke558RFData> {
 public:
  void one(RemoteTransmitData *dst) const;
  void zero(RemoteTransmitData *dst) const;
  void sync(RemoteTransmitData *dst) const;
  void stop(RemoteTransmitData *dst) const;

  void encode(RemoteTransmitData *dst, const Smoke558RFData &data) override;
  optional<Smoke558RFData> decode(RemoteReceiveData src) override;
  void dump(const Smoke558RFData &data) override;
};

DECLARE_REMOTE_PROTOCOL(Smoke558RF);

template<typename... Ts> class Smoke558RFAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint8_t, device)
  TEMPLATABLE_VALUE(uint32_t, group)
  TEMPLATABLE_VALUE(uint8_t, payload)
  void encode(RemoteTransmitData *dst, Ts... x) override {
    Smoke558RFData data{};
    data.device = this->device_.value(x...);
    data.group = this->group_.value(x...);
    data.payload = this->payload_.value(x...);
    Smoke558RFProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome
