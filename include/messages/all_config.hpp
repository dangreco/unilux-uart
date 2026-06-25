/*
 * unilux-uart -- a clean-room C++ library for the AUP/WMMM UART protocol.
 *
 * Copyright (C) 2026  Dan Greco <git@dangre.co>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

/// @file all_config.hpp
/// @brief Typed WMMM message carrying the thermostat's full configuration.
///
/// The AllConfig message (id @c 0x76, firmware name @c WMMM_ALL_CONFIG) carries
/// the device's complete configuration/status block in a 61-byte payload. The
/// MCU firmware (handler @c FUN_400dca44) parses the first 52 bytes into the
/// named fields below; the byte at offset 41 and the nine bytes at offsets
/// 52..60 are not interpreted by the firmware and are carried verbatim so that
/// @ref encode is an exact inverse of @ref decode.
///
/// Multi-byte numerics are big-endian. Temperatures are tenths of a degree
/// Celsius on the wire (÷10, as for @ref Temperature). The @c power and
/// @c price fields are 32-bit on-wire counts; their ÷100 display scale is
/// inferred from the firmware (a @c 100.0 constant on the store path) rather
/// than confirmed, so they are exposed as raw integers.
///
/// @code
// clang-format off
/// off size field                 off size field
///   0 i16  setpoint (/10)         28 u32  price
///   2 u8   setpoint_status        32 c8   currency_unit
///   3 u8   ctrl_mode              40 u8   vacation_hold
///   4 u8   main_mode              41 u8   (reserved)
///   5 u8   temp_unit              42 u8   fan_status
///   6 u8   time_format            43 u8   fan_mode
///   7 u8   adaptive_ctrl          44 u8   force_vent
///   8 i16  switching_diff (/10)   45 u8   change_over_mode
///  10 u8   system_mode            46 i16  change_over_temp_heat (/10)
///  11 u8   sensor_mode            48 i16  change_over_temp_cool (/10)
///  12 i16  internal_offset (/10)  50 i16  switching_diff_cool (/10)
///  14 i16  floor_limited (/10)    52 b9   (trailing, unparsed)
///  16 u32  power                  20 c8   power_unit
// clang-format on
/// @endcode

#include "message.hpp"
#include "utils.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

namespace unilux::message {

/// @brief Full thermostat configuration block (WMMM message id @c 0x76).
class AllConfig : public unilux::Message {

public:
  /// WMMM message id for this message.
  static constexpr uint8_t ID = 0x76;
  /// Expected payload length in bytes.
  static constexpr size_t PAYLOAD_SIZE = 61;
  /// Fixed-point scale for temperature fields (on-wire value = value * 10).
  static constexpr float SCALE = 10.0f;
  /// Width of the unit string slots (@ref power_unit, @ref currency_unit).
  static constexpr size_t UNIT_LEN = 8;
  /// Number of trailing bytes the firmware leaves unparsed (offsets 52..60).
  static constexpr size_t TRAILING_LEN = 9;

  float setpoint{};          ///< Setpoint temperature, °C (offset 0, /10).
  uint8_t setpoint_status{}; ///< Setpoint status (offset 2).
  uint8_t ctrl_mode{};       ///< Control mode (offset 3).
  uint8_t main_mode{};       ///< Main mode (offset 4).
  uint8_t temp_unit{};       ///< Temperature unit selector (offset 5).
  uint8_t time_format{};     ///< Time format selector (offset 6).
  uint8_t adaptive_ctrl{};   ///< Adaptive control flag (offset 7).
  float switching_diff{};    ///< Switching differential, °C (offset 8, /10).
  uint8_t system_mode{};     ///< System mode (offset 10).
  uint8_t sensor_mode{};     ///< Sensor mode (offset 11).
  float internal_offset{};   ///< Internal sensor offset, °C (offset 12, /10).
  float floor_limited{};     ///< Floor temperature limit, °C (offset 14, /10).
  uint32_t power{};       ///< Power reading, raw (offset 16; ÷100 to display).
  std::string power_unit; ///< Power unit string (offset 20, 8-byte slot).
  uint32_t price{};       ///< Energy price, raw (offset 28; ÷100 to display).
  std::string currency_unit; ///< Currency unit string (offset 32, 8-byte slot).
  uint8_t vacation_hold{};   ///< Vacation hold flag (offset 40).
  uint8_t fan_status{};      ///< Fan status (offset 42).
  uint8_t fan_mode{};        ///< Fan mode (offset 43).
  uint8_t force_vent{};      ///< Forced ventilation flag (offset 44).
  uint8_t change_over_mode{}; ///< Heat/cool change-over mode (offset 45).
  float
      change_over_temp_heat{}; ///< Change-over heat temp, °C (offset 46, /10).
  float
      change_over_temp_cool{}; ///< Change-over cool temp, °C (offset 48, /10).
  float switching_diff_cool{}; ///< Cooling switching differential (offset 50,
                               ///< /10).

  /// Byte at offset 41, unparsed by the firmware; carried verbatim.
  uint8_t reserved_41{};
  /// Bytes 52..60, unparsed by the firmware; carried verbatim.
  std::array<uint8_t, TRAILING_LEN> trailing{};

  AllConfig() = default;

  uint8_t id() const override { return ID; }

  Frame encode() const override {
    Frame frame;
    frame.msg_id = ID;
    frame.reserved[0] = 0;
    frame.reserved[1] = 0;
    frame.reserved[2] = 0;
    frame.payload.assign(PAYLOAD_SIZE, 0);
    uint8_t *p = frame.payload.data();

    to_be16(p + 0, static_cast<uint16_t>(setpoint * SCALE));
    p[2] = setpoint_status;
    p[3] = ctrl_mode;
    p[4] = main_mode;
    p[5] = temp_unit;
    p[6] = time_format;
    p[7] = adaptive_ctrl;
    to_be16(p + 8, static_cast<uint16_t>(switching_diff * SCALE));
    p[10] = system_mode;
    p[11] = sensor_mode;
    to_be16(p + 12, static_cast<uint16_t>(internal_offset * SCALE));
    to_be16(p + 14, static_cast<uint16_t>(floor_limited * SCALE));
    to_be32(p + 16, power);
    write_unit(p + 20, power_unit);
    to_be32(p + 28, price);
    write_unit(p + 32, currency_unit);
    p[40] = vacation_hold;
    p[41] = reserved_41;
    p[42] = fan_status;
    p[43] = fan_mode;
    p[44] = force_vent;
    p[45] = change_over_mode;
    to_be16(p + 46, static_cast<uint16_t>(change_over_temp_heat * SCALE));
    to_be16(p + 48, static_cast<uint16_t>(change_over_temp_cool * SCALE));
    to_be16(p + 50, static_cast<uint16_t>(switching_diff_cool * SCALE));
    for (size_t i = 0; i < TRAILING_LEN; ++i) {
      p[52 + i] = trailing[i];
    }
    return frame;
  }

  std::string to_string() const override {
    char buf[160];
    std::snprintf(
        buf, sizeof(buf),
        "AllConfig(setpoint=%.1f, main=%u, ctrl=%u, fan_status=%u, "
        "changeOver=%u heat=%.1f cool=%.1f, power=%u%s)",
        static_cast<double>(setpoint), static_cast<unsigned>(main_mode),
        static_cast<unsigned>(ctrl_mode), static_cast<unsigned>(fan_status),
        static_cast<unsigned>(change_over_mode),
        static_cast<double>(change_over_temp_heat),
        static_cast<double>(change_over_temp_cool),
        static_cast<unsigned>(power), power_unit.c_str());
    return buf;
  }

  /// @brief Decode an AllConfig message from a generic WMMM frame.
  /// @param frame The frame to interpret.
  /// @return The decoded message, or @c std::nullopt if the payload is not
  ///         exactly @ref PAYLOAD_SIZE bytes.
  static std::optional<AllConfig> decode(const Frame &frame) {
    if (frame.payload.size() != PAYLOAD_SIZE) {
      return std::nullopt;
    }
    const uint8_t *p = frame.payload.data();

    AllConfig msg;
    msg.setpoint =
        static_cast<float>(static_cast<int16_t>(from_be16(p + 0))) / SCALE;
    msg.setpoint_status = p[2];
    msg.ctrl_mode = p[3];
    msg.main_mode = p[4];
    msg.temp_unit = p[5];
    msg.time_format = p[6];
    msg.adaptive_ctrl = p[7];
    msg.switching_diff =
        static_cast<float>(static_cast<int16_t>(from_be16(p + 8))) / SCALE;
    msg.system_mode = p[10];
    msg.sensor_mode = p[11];
    msg.internal_offset =
        static_cast<float>(static_cast<int16_t>(from_be16(p + 12))) / SCALE;
    msg.floor_limited =
        static_cast<float>(static_cast<int16_t>(from_be16(p + 14))) / SCALE;
    msg.power = from_be32(p + 16);
    msg.power_unit = read_unit(p + 20);
    msg.price = from_be32(p + 28);
    msg.currency_unit = read_unit(p + 32);
    msg.vacation_hold = p[40];
    msg.reserved_41 = p[41];
    msg.fan_status = p[42];
    msg.fan_mode = p[43];
    msg.force_vent = p[44];
    msg.change_over_mode = p[45];
    msg.change_over_temp_heat =
        static_cast<float>(static_cast<int16_t>(from_be16(p + 46))) / SCALE;
    msg.change_over_temp_cool =
        static_cast<float>(static_cast<int16_t>(from_be16(p + 48))) / SCALE;
    msg.switching_diff_cool =
        static_cast<float>(static_cast<int16_t>(from_be16(p + 50))) / SCALE;
    for (size_t i = 0; i < TRAILING_LEN; ++i) {
      msg.trailing[i] = p[52 + i];
    }
    return msg;
  }

private:
  /// Read a NUL-terminated string from a fixed @ref UNIT_LEN-byte slot.
  static std::string read_unit(const uint8_t *bytes) {
    size_t len = 0;
    while (len < UNIT_LEN && bytes[len] != 0) {
      ++len;
    }
    return std::string(reinterpret_cast<const char *>(bytes), len);
  }

  /// Write a string into a fixed @ref UNIT_LEN-byte slot, NUL-padded and
  /// truncated to fit.
  static void write_unit(uint8_t *bytes, const std::string &value) {
    size_t len = value.size() < UNIT_LEN ? value.size() : UNIT_LEN;
    for (size_t i = 0; i < len; ++i) {
      bytes[i] = static_cast<uint8_t>(value[i]);
    }
    for (size_t i = len; i < UNIT_LEN; ++i) {
      bytes[i] = 0;
    }
  }
};

} // namespace unilux::message
