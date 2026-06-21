#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include "aup.hpp"
#include "wmmm.hpp"

namespace esphome {
namespace unilux_uart {

class UniluxUartComponent : public Component, public uart::UARTDevice {
public:
  void dump_config() override;
  void setup() override;
  void loop() override;

private:
  /// Decode the WMMM message inside @p frame and log it at debug level.
  void log_frame_(const unilux::aup::Frame &frame);

  unilux::aup::Decoder decoder_; ///< AUP byte-framing decoder.
  unilux::Decoder wmmm_decoder_; ///< WMMM message decoder.
};

} // namespace unilux_uart
} // namespace esphome
