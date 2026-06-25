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

/// @file system_mode.hpp
/// @brief Typed WMMM message carrying the installer system-mode setting.
///
/// The SystemMode message (id @c 0x23, thermostat parameter P02) selects the
/// HVAC plant type in the first byte of a 4-byte payload; the remaining three
/// bytes are zero:
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
/// Unknown byte values are preserved verbatim by @ref decode.

#include "message.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

namespace unilux::message {

/// @brief Installer system-mode selection (WMMM message id @c 0x23).
class SystemMode : public unilux::Message {

public:
  /// WMMM message id for this message.
  static constexpr uint8_t ID = 0x23;
  /// Expected payload length in bytes (one mode byte plus three reserved).
  static constexpr size_t PAYLOAD_SIZE = 4;

  /// On-wire system-mode values (the first payload byte).
  enum class Value : uint8_t {
    TwoPipe = 0x00,     ///< 2-pipe system (2P).
    FourPipe = 0x01,    ///< 4-pipe system (4P).
    TwoPipeAuto = 0x02, ///< 2-pipe with auto changeover (2A).
    TwoPipeHeat = 0x03, ///< 2-pipe heat-only (2H).
  };

  /// The decoded system mode.
  Value value{Value::TwoPipe};

  SystemMode() = default;
  explicit SystemMode(Value value) : value(value) {}

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
    case Value::TwoPipe:
      return "SystemMode(2-pipe)";
    case Value::FourPipe:
      return "SystemMode(4-pipe)";
    case Value::TwoPipeAuto:
      return "SystemMode(2-pipe auto)";
    case Value::TwoPipeHeat:
      return "SystemMode(2-pipe heat)";
    }
    char buf[24];
    std::snprintf(buf, sizeof(buf), "SystemMode(0x%02X)",
                  static_cast<unsigned>(value));
    return buf;
  }

  /// @brief Decode a SystemMode message from a generic WMMM frame.
  /// @param frame The frame to interpret.
  /// @return The decoded message, or @c std::nullopt if the payload is not
  ///         exactly @ref PAYLOAD_SIZE bytes. The mode byte is not validated;
  ///         an unrecognised value is carried through verbatim.
  static std::optional<SystemMode> decode(const Frame &frame) {
    if (frame.payload.size() != PAYLOAD_SIZE) {
      return std::nullopt;
    }

    SystemMode msg;
    msg.value = static_cast<Value>(frame.payload[0]);
    return msg;
  }
};

} // namespace unilux::message
