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

/// @file temperature.hpp
/// @brief Typed WMMM message carrying two temperature readings.
///
/// The Temperature message (id @c 0x02) carries two temperatures in a 4-byte
/// payload, each a big-endian 16-bit fixed-point value in tenths of a degree
/// Celsius (the on-wire value is the temperature times ten):
///
/// @code
// clang-format off
/// +----------------+----------------+
/// | t1 (BE 16)     | t2 (BE 16)     |
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

/// @brief Two-channel temperature reading (WMMM message id @c 0x02).
///
/// Decode an incoming frame with @ref decode and serialise an outgoing one with
/// @ref encode; the two are inverses up to the fixed-point resolution of one
/// tenth of a degree.
class Temperature : public unilux::Message {

public:
  /// WMMM message id for this message.
  static constexpr uint8_t ID = 0x02;
  /// Expected payload length in bytes (two big-endian 16-bit values).
  static constexpr size_t PAYLOAD_SIZE = 4;
  /// Fixed-point scale: the on-wire value is the temperature times this factor.
  static constexpr float SCALE = 10.0f;

  /// First temperature channel, in degrees Celsius.
  float t1{};
  /// Second temperature channel, in degrees Celsius.
  float t2{};

  Temperature() = default;
  Temperature(float t1, float t2) : t1(t1), t2(t2) {}

  uint8_t id() const override { return ID; }

  Frame encode() const override {
    Frame frame;
    frame.msg_id = ID;
    frame.reserved[0] = 0;
    frame.reserved[1] = 0;
    frame.reserved[2] = 0;
    frame.payload.resize(PAYLOAD_SIZE);
    to_be16(frame.payload.data(), static_cast<uint16_t>(t1 * SCALE));
    to_be16(frame.payload.data() + 2, static_cast<uint16_t>(t2 * SCALE));
    return frame;
  }

  std::string to_string() const override {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Temperature(t1=%.1f, t2=%.1f)",
                  static_cast<double>(t1), static_cast<double>(t2));
    return buf;
  }

  /// @brief Decode a Temperature message from a generic WMMM frame.
  /// @param frame The frame to interpret.
  /// @return The decoded message, or @c std::nullopt if the payload is not
  ///         exactly @ref PAYLOAD_SIZE bytes.
  static std::optional<Temperature> decode(const Frame &frame) {
    if (frame.payload.size() != PAYLOAD_SIZE) {
      return std::nullopt;
    }

    Temperature msg;
    msg.t1 = static_cast<float>(from_be16(frame.payload.data())) / SCALE;
    msg.t2 = static_cast<float>(from_be16(frame.payload.data() + 2)) / SCALE;
    return msg;
  }
};

} // namespace unilux::message
