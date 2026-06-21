#include "unilux_uart.h"

#include "esphome/core/alloc_helpers.h"
#include "esphome/core/log.h"

#include <vector>

namespace esphome {
namespace unilux_uart {

static const char *const TAG = "unilux_uart";

void UniluxUartComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Unilux UART:");
  check_uart_settings(115200);
}

void UniluxUartComponent::setup() {}

void UniluxUartComponent::loop() {
  uint8_t byte;
  while (this->available() && this->read_byte(&byte)) {
    if (auto frame = decoder_.consume(byte)) {
      log_frame_(*frame);
    }
  }
}

void UniluxUartComponent::log_frame_(const unilux::aup::Frame &frame) {
  ESP_LOGD(TAG, "┌─ frame ───────────────────────────");
  ESP_LOGD(TAG,
           "│ AUP  type=0x%02X cmd=0x%02X flag=0x%02X chksum=0x%02X len=%u",
           frame.type, frame.command, frame.flag, frame.checksum, frame.length);

  if (auto wmmm = wmmm_decoder_.decode(frame.payload)) {
    std::vector<uint8_t> reserved(wmmm->reserved, wmmm->reserved + 3);
    ESP_LOGD(TAG, "│ WMMM msg_id=0x%02X  reserved=%s", wmmm->msg_id,
             format_hex_pretty(reserved, '.', false).c_str());
    ESP_LOGD(TAG, "│ payload (%u): %s", (unsigned)wmmm->payload.size(),
             format_hex_pretty(wmmm->payload, ' ', false).c_str());
  } else {
    // Payload too short to be a WMMM message; show the raw AUP payload instead.
    ESP_LOGD(TAG, "│ WMMM <undecodable, %u bytes>",
             (unsigned)frame.payload.size());
    ESP_LOGD(TAG, "│ payload (%u): %s", (unsigned)frame.payload.size(),
             format_hex_pretty(frame.payload, ' ', false).c_str());
  }
  ESP_LOGD(TAG, "└───────────────────────────────────");
}

} // namespace unilux_uart
} // namespace esphome
