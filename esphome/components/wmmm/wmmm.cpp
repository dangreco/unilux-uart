#include "wmmm.h"

namespace esphome {
namespace wmmm {

static const char *const TAG = "wmmm";

void WmmmComponent::setup() {
  ESP_LOGCONFIG(TAG, "WMMM:");
  check_uart_settings(115200);
}

void WmmmComponent::loop() {}

void WmmmComponent::dump_config() {}

} // namespace wmmm
} // namespace esphome
