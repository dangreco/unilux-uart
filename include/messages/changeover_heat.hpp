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

/// @file changeover_heat.hpp
/// @brief Typed WMMM message carrying the auto-mode heating changeover temp.
///
/// The ChangeoverHeat message (id @c 0x5D, thermostat parameter P16) carries a
/// single temperature in a 4-byte payload: a big-endian 16-bit fixed-point
/// value in tenths of a degree Celsius (bytes 0..1), then two zero bytes. This
/// is the temperature at which auto mode switches to heating. Typical range is
/// 10.0..25.0 °C.
///
/// @code
// clang-format off
/// +----------------+----------------+
/// | value (BE 16)  | reserved       |
/// |    2 bytes     |    2 bytes     |
/// +----------------+----------------+
// clang-format on
/// @endcode

#include "message.hpp"
#include "utils.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

namespace unilux::message {

/// @brief Auto-mode heating changeover temperature (WMMM message id @c 0x5D).
class ChangeoverHeat : public unilux::Message {

public:
  /// WMMM message id for this message.
  static constexpr uint8_t ID = 0x5D;
  /// Expected payload length in bytes (one big-endian 16-bit value, padded).
  static constexpr size_t PAYLOAD_SIZE = 4;
  /// Fixed-point scale: the on-wire value is the temperature times this factor.
  static constexpr float SCALE = 10.0f;

  /// The changeover temperature, in degrees Celsius.
  float value{};

  ChangeoverHeat() = default;
  explicit ChangeoverHeat(float value) : value(value) {}

  uint8_t id() const override { return ID; }

  Frame encode() const override {
    Frame frame;
    frame.msg_id = ID;
    frame.reserved[0] = 0;
    frame.reserved[1] = 0;
    frame.reserved[2] = 0;
    frame.payload.assign(PAYLOAD_SIZE, 0);
    to_be16(frame.payload.data(),
            static_cast<uint16_t>(static_cast<int16_t>(value * SCALE)));
    return frame;
  }

  std::string to_string() const override {
    char buf[40];
    std::snprintf(buf, sizeof(buf), "ChangeoverHeat(%.1f)",
                  static_cast<double>(value));
    return buf;
  }

  /// @brief Decode a ChangeoverHeat message from a generic WMMM frame.
  /// @param frame The frame to interpret.
  /// @return The decoded message, or @c std::nullopt if the payload is not
  ///         exactly @ref PAYLOAD_SIZE bytes.
  static std::optional<ChangeoverHeat> decode(const Frame &frame) {
    if (frame.payload.size() != PAYLOAD_SIZE) {
      return std::nullopt;
    }

    ChangeoverHeat msg;
    msg.value = static_cast<float>(
                    static_cast<int16_t>(from_be16(frame.payload.data()))) /
                SCALE;
    return msg;
  }
};

} // namespace unilux::message
