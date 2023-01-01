#include "smoke_558RF_protocol.h"
#include "esphome/core/log.h"

  // Protocol is mainly adobted from rtl443 library: https://github.com/merbanan/rtl_433/blob/master/src/devices/smoke_gs558.c
  /*

  S HHHH HGGG GGGG GGGG GGGG DDDD E

  S = Sync bit.
  H = First 5 bits is device ID.
  G = Group ID, 15 bits.
  D = Payload, 4 bits
  E = Ending packet with stop sequence

  The ending sequence is directly followed by a new SYNC bit. Then the packet is repeated 7 more times (8 times total)


  */

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.smoke558rf";

static const uint32_t BIT_SYNC_STOP_HIGH_US = 450;
static const uint32_t BIT_STOP_LOW_US = 450;
static const uint32_t BIT_SYNC_LOW_US = 11880;

static const uint32_t BIT_LONG_HIGH_US = 1240;
static const uint32_t BIT_LONG_LOW_US = 510;
static const uint32_t BIT_SHORT_HIGH_US = 470;
static const uint32_t BIT_SHORT_LOW_US = 1300;

void Smoke558RFProtocol::one(RemoteTransmitData *dst) const {
  // '1' => '10'
  dst->item(BIT_LONG_HIGH_US, BIT_LONG_LOW_US);
}

void Smoke558RFProtocol::zero(RemoteTransmitData *dst) const {
  // '0' => '01'
  dst->item(BIT_SHORT_HIGH_US, BIT_SHORT_LOW_US);
}

void Smoke558RFProtocol::sync(RemoteTransmitData *dst) const { dst->item(BIT_SYNC_STOP_HIGH_US, BIT_SYNC_LOW_US); }

void Smoke558RFProtocol::stop(RemoteTransmitData *dst) const {
  dst->item(BIT_SYNC_STOP_HIGH_US, BIT_STOP_LOW_US);
  dst->item(BIT_SYNC_STOP_HIGH_US, BIT_STOP_LOW_US);
}

void Smoke558RFProtocol::encode(RemoteTransmitData *dst, const Smoke558RFData &data) {
  dst->set_carrier_frequency(0);

  // Send SYNC
  this->sync(dst);

  // Device (5 bits)
  for (int16_t i = 5 - 1; i >= 0; i--) {
    if (data.device & (1 << i)) {
      this->one(dst);
    } else {
      this->zero(dst);
    }
  }

  // Group (15 bits)
  for (int16_t i = 15 - 1; i >= 0; i--) {
    if (data.group & (1 << i)) {
      this->one(dst);
    } else {
      this->zero(dst);
    }
  }

  // payload (4 bit)
  for (int16_t i = 4 - 1; i >= 0; i--) {
    if (data.payload & (1 << i)) {
      this->one(dst);
    } else {
      this->zero(dst);
    }
  }

  // Send stop
  this->stop(dst);
}

optional<Smoke558RFData> Smoke558RFProtocol::decode(RemoteReceiveData src) {
  Smoke558RFData out{
      .device = 0,
      .group = 0,
      .payload = 0
  };


  // Require a SYNC pulse + long gap
  if (!src.expect_pulse_with_gap(BIT_SYNC_STOP_HIGH_US, BIT_SYNC_LOW_US))
    return {};

  // Device
  for (uint8_t i = 0; i < 5; i++) {
    out.device <<= 1UL;
    if (src.expect_pulse_with_gap(BIT_LONG_HIGH_US, BIT_LONG_LOW_US)) {
      out.device |= 0x01;
    } else if (src.expect_pulse_with_gap(BIT_SHORT_HIGH_US, BIT_SHORT_LOW_US)) {
      out.device |= 0x00;
    } else {
      // This should not happen...failed command
      return {};
    }
  }

  // GROUP
  for (uint8_t i = 0; i < 15; i++) {
    out.group <<= 1UL;
    if (src.expect_pulse_with_gap(BIT_LONG_HIGH_US, BIT_LONG_LOW_US)) {
      out.group |= 0x01;
    } else if (src.expect_pulse_with_gap(BIT_SHORT_HIGH_US, BIT_SHORT_LOW_US)) {
      out.group |= 0x00;
    } else {
      // This should not happen...failed command
      return {};
    }
  }

  // Payload
  for (uint8_t i = 0; i < 4; i++) {
    out.payload <<= 1UL;
    if (src.expect_pulse_with_gap(BIT_LONG_HIGH_US, BIT_LONG_LOW_US)) {
      out.payload |= 0x01;
    } else if (src.expect_pulse_with_gap(BIT_SHORT_HIGH_US, BIT_SHORT_LOW_US)) {
      out.payload |= 0x00;
    } else {
      // This should not happen...failed command
      return {};
    }
  }

  // Require stop sequence
  if (!(src.expect_pulse_with_gap(BIT_SYNC_STOP_HIGH_US, BIT_SYNC_LOW_US) &&
        src.expect_pulse_with_gap(BIT_SYNC_STOP_HIGH_US, BIT_SYNC_LOW_US))) {
      
      return {};
  }

  return out;
}

void Smoke558RFProtocol::dump(const Smoke558RFData &data) {
  ESP_LOGD(TAG, "Received Smoke558RF: device=0x%04X group=%d payload=%d", data.device, data.group,
           data.payload);
}

}  // namespace remote_base
}  // namespace esphome
