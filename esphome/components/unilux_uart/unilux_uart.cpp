#include "unilux_uart.h"

#include "esphome/core/alloc_helpers.h"
#include "esphome/core/log.h"

#include "messages.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace esphome {
namespace unilux_uart {

static const char *const TAG = "unilux_uart";

// AUP header is constant for this protocol; outgoing frames reuse these values
// (the decoder never verifies the checksum, so it is sent as zero).
static constexpr uint8_t AUP_TX_CHECKSUM = 0x00;
static constexpr uint8_t AUP_TX_FLAG = 0x00;
static constexpr uint8_t AUP_TX_TYPE = 0x01;
static constexpr uint8_t AUP_TX_COMMAND = 0x10;

void UniluxUartComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Unilux UART:");
  check_uart_settings(115200);
}

void UniluxUartComponent::setup() {}

void UniluxUartComponent::send_message(const unilux::Message &msg) {
  // Message -> WMMM frame -> WMMM bytes -> AUP frame -> AUP bytes -> UART.
  std::vector<uint8_t> wmmm_bytes = this->wmmm_encoder_.encode(msg.encode());

  unilux::aup::Frame aup;
  aup.checksum = AUP_TX_CHECKSUM;
  aup.flag = AUP_TX_FLAG;
  aup.type = AUP_TX_TYPE;
  aup.command = AUP_TX_COMMAND;
  aup.length = static_cast<uint16_t>(wmmm_bytes.size());
  aup.payload = std::move(wmmm_bytes);

  std::vector<uint8_t> out = this->aup_encoder_.encode(aup);
  this->write_array(out.data(), out.size());
}

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

    // Decode the WMMM frame into a concrete message type and switch on it.
    if (auto msg = unilux::decode_message(*wmmm)) {
      std::visit(
          [&](auto &&m) {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T, unilux::message::Temperature>) {
              ESP_LOGD(TAG, "│ Temperature t1=%.1f°C t2=%.1f°C",
                       static_cast<double>(m.t1), static_cast<double>(m.t2));
              if (this->t1_sensor_ != nullptr)
                this->t1_sensor_->publish_state(m.t1);
              if (this->t2_sensor_ != nullptr)
                this->t2_sensor_->publish_state(m.t2);
              // Channel 1 drives the climate entity's current temperature.
              if (this->climate_ != nullptr)
                this->climate_->publish_current_temperature(m.t1);
            } else if constexpr (std::is_same_v<
                                     T, unilux::message::TargetTemperature>) {
              ESP_LOGD(TAG, "│ TargetTemperature t1=%.1f°C t2=%.1f°C",
                       static_cast<double>(m.t1), static_cast<double>(m.t2));
              // Channel 1 drives the climate entity's setpoint (reflecting
              // device state; this does not transmit).
              if (this->climate_ != nullptr)
                this->climate_->publish_target_temperature(m.t1);
            }
          },
          *msg);
    } else {
      ESP_LOGD(TAG, "│ <no typed message for id 0x%02X>", wmmm->msg_id);
    }
  } else {
    // Payload too short to be a WMMM message; show the raw AUP payload instead.
    ESP_LOGD(TAG, "│ WMMM <undecodable, %u bytes>",
             (unsigned)frame.payload.size());
    ESP_LOGD(TAG, "│ payload (%u): %s", (unsigned)frame.payload.size(),
             format_hex_pretty(frame.payload, ' ', false).c_str());
  }
  ESP_LOGD(TAG, "└───────────────────────────────────");
}

// --- UniluxUartClimate ------------------------------------------------------

void UniluxUartClimate::setup() {
  // Start in the single supported mode so the entity has a defined state before
  // the first frame arrives.
  this->mode = climate::CLIMATE_MODE_AUTO;
  this->publish_state();
}

void UniluxUartClimate::dump_config() {
  LOG_CLIMATE("", "Unilux UART Climate", this);
}

climate::ClimateTraits UniluxUartClimate::traits() {
  climate::ClimateTraits traits;
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_supported_modes({climate::CLIMATE_MODE_AUTO});
  // Defaults; overridden by any `visual:` block in the YAML config.
  traits.set_visual_min_temperature(5.0f);
  traits.set_visual_max_temperature(35.0f);
  traits.set_visual_temperature_step(0.5f);
  return traits;
}

void UniluxUartClimate::control(const climate::ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
    // Encode and transmit the new setpoint to the device. Only channel 1 is
    // meaningful; channel 2 is sent as zero.
    if (this->parent_ != nullptr) {
      this->parent_->send_message(
          unilux::message::TargetTemperature(this->target_temperature, 0.0f));
    }
  }
  this->mode = climate::CLIMATE_MODE_AUTO;
  this->publish_state();
}

void UniluxUartClimate::publish_current_temperature(float temperature) {
  this->current_temperature = temperature;
  this->mode = climate::CLIMATE_MODE_AUTO;
  this->publish_state();
}

void UniluxUartClimate::publish_target_temperature(float temperature) {
  this->target_temperature = temperature;
  this->mode = climate::CLIMATE_MODE_AUTO;
  this->publish_state();
}

} // namespace unilux_uart
} // namespace esphome
