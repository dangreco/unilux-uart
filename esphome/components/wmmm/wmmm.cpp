#include "wmmm.h"

namespace esphome {
namespace wmmm {

static const char *const TAG = "wmmm";

void WmmmComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "WMMM:");
  check_uart_settings(115200);
}

void WmmmComponent::setup() {
  // Plumbing check: forces src/aup.cpp to link. Real decode wiring is a
  // follow-up.
  (void)decoder_.consume(0x00);
}

void WmmmComponent::loop() {}

} // namespace wmmm
} // namespace esphome
