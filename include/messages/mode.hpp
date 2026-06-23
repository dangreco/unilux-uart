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

/// @file mode.hpp
/// @brief Typed WMMM message carrying the thermostat's HVAC mode.
///
/// The Mode message (id @c 0x5C) carries the operating mode in the first byte
/// of a 4-byte payload; the remaining three bytes are zero:
///
/// @code
// clang-format off
/// +--------+--------------------------+
/// | mode   | reserved                 |
/// | 1 byte |        3 bytes           |
/// +--------+--------------------------+
// clang-format on
/// @endcode
///
/// The mode byte is one of @ref Mode::Value (heat, cool, auto). Unknown byte
/// values are preserved verbatim by @ref decode and surfaced to the caller, who
/// decides how to handle them.

#include "message.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

namespace unilux::message {

/// @brief Thermostat HVAC mode (WMMM message id @c 0x5C).
///
/// Decode an incoming frame with @ref decode and serialise an outgoing one with
/// @ref encode; the two are exact inverses.
class Mode : public unilux::Message {

public:
  /// WMMM message id for this message.
  static constexpr uint8_t ID = 0x5C;
  /// Expected payload length in bytes (one mode byte plus three reserved).
  static constexpr size_t PAYLOAD_SIZE = 4;

  /// On-wire HVAC mode values (the first payload byte).
  enum class Value : uint8_t {
    Heat = 0x00, ///< Heating to the target temperature.
    Cool = 0x01, ///< Cooling to the target temperature.
    Auto = 0x02, ///< Heating or cooling to reach the target temperature.
  };

  /// The decoded mode value.
  Value value{Value::Heat};

  Mode() = default;
  explicit Mode(Value value) : value(value) {}

  uint8_t id() const override { return ID; }

  Frame encode() const override {
    Frame frame;
    frame.msg_id = ID;
    frame.reserved[0] = 0;
    frame.reserved[1] = 0;
    frame.reserved[2] = 0;
    frame.payload.assign(PAYLOAD_SIZE, 0);
    frame.payload[0] = static_cast<uint8_t>(value);
    return frame;
  }

  std::string to_string() const override {
    switch (value) {
    case Value::Heat:
      return "Mode(heat)";
    case Value::Cool:
      return "Mode(cool)";
    case Value::Auto:
      return "Mode(auto)";
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "Mode(0x%02X)",
                  static_cast<unsigned>(value));
    return buf;
  }

  /// @brief Decode a Mode message from a generic WMMM frame.
  /// @param frame The frame to interpret.
  /// @return The decoded message, or @c std::nullopt if the payload is not
  ///         exactly @ref PAYLOAD_SIZE bytes. The mode byte is not validated;
  ///         an unrecognised value is carried through verbatim.
  static std::optional<Mode> decode(const Frame &frame) {
    if (frame.payload.size() != PAYLOAD_SIZE) {
      return std::nullopt;
    }

    Mode msg;
    msg.value = static_cast<Value>(frame.payload[0]);
    return msg;
  }
};

} // namespace unilux::message
