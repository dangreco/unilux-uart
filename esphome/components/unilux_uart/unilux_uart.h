#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include "aup.hpp"
#include "message.hpp"
#include "messages/fan_speed.hpp"
#include "messages/mode.hpp"
#include "wmmm.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace esphome {
namespace unilux_uart {

class UniluxUartClimate;
class UniluxNumber;
class UniluxSelect;

class UniluxUartComponent : public Component, public uart::UARTDevice {
public:
  void dump_config() override;
  void setup() override;
  void loop() override;

  void set_t1_sensor(sensor::Sensor *t1_sensor) {
    this->t1_sensor_ = t1_sensor;
  }
  void set_t2_sensor(sensor::Sensor *t2_sensor) {
    this->t2_sensor_ = t2_sensor;
  }
  void set_climate(UniluxUartClimate *climate) { this->climate_ = climate; }

  /// Register a setting number/select entity; the component tracks it so it can
  /// publish received values back to it (the entity carries its own msg_id).
  void add_number(UniluxNumber *number) { this->numbers_.push_back(number); }
  void add_select(UniluxSelect *select) { this->selects_.push_back(select); }

  /// Serialise @p msg through the WMMM and AUP encoders and write it out the
  /// UART, using this protocol's constant AUP header.
  void send_message(const unilux::Message &msg);

  /// Transmit a scalar-temperature setting (i16 BE, tenths of a degree) for the
  /// given WMMM message id. Unknown ids are ignored with a warning.
  void send_temperature_setting(uint8_t msg_id, float value);
  /// Transmit a single-byte enum setting for the given WMMM message id. Unknown
  /// ids are ignored with a warning.
  void send_byte_setting(uint8_t msg_id, uint8_t value);

protected:
  /// Publish a received scalar value to the number entity bound to @p msg_id.
  void publish_number_(uint8_t msg_id, float value);
  /// Publish a received enum byte to the select entity bound to @p msg_id.
  void publish_select_(uint8_t msg_id, uint8_t value);

  /// Decode the WMMM message in @p frame, log it, and publish entity states.
  void log_frame_(const unilux::aup::Frame &frame);

  unilux::aup::Decoder decoder_;     ///< AUP byte-framing decoder.
  unilux::Decoder wmmm_decoder_;     ///< WMMM message decoder.
  unilux::Encoder wmmm_encoder_;     ///< WMMM message encoder (TX).
  unilux::aup::Encoder aup_encoder_; ///< AUP byte-framing encoder (TX).

  /// Temperature channels; nullptr when the channel is disabled in the config.
  sensor::Sensor *t1_sensor_{nullptr};
  sensor::Sensor *t2_sensor_{nullptr};

  /// Target-temperature climate entity; nullptr when not configured.
  UniluxUartClimate *climate_{nullptr};

  /// Setting entities; each knows its own msg_id for receive-side routing.
  std::vector<UniluxNumber *> numbers_;
  std::vector<UniluxSelect *> selects_;
};

/// @brief A device setting surfaced as an ESPHome number (a scalar temperature,
/// i16 BE tenths of a degree). Changing it transmits the matching WMMM frame.
class UniluxNumber : public number::Number {
public:
  void set_parent(UniluxUartComponent *parent) { this->parent_ = parent; }
  void set_msg_id(uint8_t msg_id) { this->msg_id_ = msg_id; }
  uint8_t get_msg_id() const { return this->msg_id_; }

protected:
  void control(float value) override;

  UniluxUartComponent *parent_{nullptr};
  uint8_t msg_id_{0};
};

/// @brief A device setting surfaced as an ESPHome select (an enum byte).
/// Changing it transmits the matching WMMM frame. The displayed options map
/// one-to-one (by index) to @ref option_bytes_.
class UniluxSelect : public select::Select {
public:
  void set_parent(UniluxUartComponent *parent) { this->parent_ = parent; }
  void set_msg_id(uint8_t msg_id) { this->msg_id_ = msg_id; }
  uint8_t get_msg_id() const { return this->msg_id_; }
  void set_option_bytes(const std::vector<uint8_t> &bytes) {
    this->option_bytes_ = bytes;
  }

  /// Publish the option whose mapped byte equals @p value, if any.
  void publish_byte(uint8_t value);

protected:
  void control(const std::string &value) override;

  UniluxUartComponent *parent_{nullptr};
  uint8_t msg_id_{0};
  std::vector<uint8_t> option_bytes_;
};

/// @brief Two-way target-temperature control surfaced as an ESPHome climate.
///
/// A received TargetTemperature (WMMM id 0x2A) frame updates this entity's
/// setpoint; changing the setpoint in Home Assistant encodes and transmits a
/// TargetTemperature frame back to the device via the parent hub.
class UniluxUartClimate : public climate::Climate, public Component {
public:
  void set_parent(UniluxUartComponent *parent) { this->parent_ = parent; }

  void setup() override;
  void dump_config() override;

  /// Update the displayed current temperature (driven by the Temperature
  /// message's first channel) and republish.
  void publish_current_temperature(float temperature);
  /// Update the target temperature from a received frame and republish (does
  /// not transmit; this reflects device state).
  void publish_target_temperature(float temperature);
  /// Update the HVAC mode from a received Mode frame and republish (does not
  /// transmit; this reflects device state). Unknown values are ignored.
  void publish_mode(unilux::message::Mode::Value value);
  /// Update the power state from a received Power frame and republish (does not
  /// transmit; this reflects device state).
  void publish_power(bool on);
  /// Update the fan speed from a received FanSpeed frame and republish (does
  /// not transmit; this reflects device state). Unknown values are ignored.
  void publish_fan_speed(unilux::message::FanSpeed::Value value);

protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  /// Recompute the HA mode from power + active mode and publish it.
  void publish_combined_mode_();

  UniluxUartComponent *parent_{nullptr};

  /// Power (0x21) and HVAC mode (0x5C) arrive as separate messages; HA folds
  /// them into one mode enum (OFF when powered off, else the active mode).
  bool power_on_{true};
  climate::ClimateMode active_mode_{climate::CLIMATE_MODE_HEAT};
};

} // namespace unilux_uart
} // namespace esphome
