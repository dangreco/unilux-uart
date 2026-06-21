#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include "aup.hpp"

namespace esphome {
namespace wmmm {

class WmmmComponent : public Component, public uart::UARTDevice {
public:
  void dump_config() override;
  void setup() override;
  void loop() override;

private:
  ::wmmm::aup::Decoder decoder_;
};

} // namespace wmmm
} // namespace esphome
