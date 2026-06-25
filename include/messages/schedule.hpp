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

/// @file schedule.hpp
/// @brief Typed WMMM message carrying the weekly program (schedule).
///
/// The Schedule message (id @c 0x78, firmware name @c WMMM_ALL_PRG) carries the
/// weekly setpoint program in a 116-byte payload: a one-byte program mode,
/// three reserved bytes, then seven day-blocks (one per day of the week) of
/// four entries each. Each entry is a switch time and target setpoint:
///
/// @code
// clang-format off
/// +---------+----------+------------------------------------------+
/// | prgMode | reserved | 7 days x 4 entries                       |
/// | 1 byte  | 3 bytes  | 7 x (4 x 4 bytes)                        |
/// +---------+----------+------------------------------------------+
///                       entry = [hour u8][minute u8][setpoint i16 BE /10]
// clang-format on
/// @endcode
///
/// Setpoints are tenths of a degree Celsius on the wire (÷10, as for
/// @ref Temperature). The day index (0..6) and entry index (0..3) are
/// positional and not stored on the wire.

#include "message.hpp"
#include "utils.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

namespace unilux::message {

/// @brief Weekly setpoint program (WMMM message id @c 0x78).
class Schedule : public unilux::Message {

public:
  /// WMMM message id for this message.
  static constexpr uint8_t ID = 0x78;
  /// Number of day-blocks (one per day of the week).
  static constexpr size_t DAYS = 7;
  /// Number of switch entries per day.
  static constexpr size_t ENTRIES_PER_DAY = 4;
  /// Size of one entry on the wire, in bytes.
  static constexpr size_t ENTRY_SIZE = 4;
  /// Expected payload length: 1 mode + 3 reserved + 7 x 4 x 4-byte entries.
  static constexpr size_t PAYLOAD_SIZE =
      4 + DAYS * ENTRIES_PER_DAY * ENTRY_SIZE;
  /// Fixed-point scale for setpoints (on-wire value = value * 10).
  static constexpr float SCALE = 10.0f;

  /// A single scheduled switch point.
  struct Entry {
    uint8_t hour{};   ///< Hour of day (0..23).
    uint8_t minute{}; ///< Minute of hour (0..59).
    float setpoint{}; ///< Target temperature, °C (/10 on the wire).
  };

  /// Program mode (payload offset 0).
  uint8_t prg_mode{};
  /// Reserved header bytes (offsets 1..3), carried verbatim.
  std::array<uint8_t, 3> reserved{};
  /// Schedule entries, indexed [day][entry].
  std::array<std::array<Entry, ENTRIES_PER_DAY>, DAYS> days{};

  Schedule() = default;

  uint8_t id() const override { return ID; }

  Frame encode() const override {
    Frame frame;
    frame.msg_id = ID;
    frame.reserved[0] = 0;
    frame.reserved[1] = 0;
    frame.reserved[2] = 0;
    frame.payload.assign(PAYLOAD_SIZE, 0);
    uint8_t *p = frame.payload.data();

    p[0] = prg_mode;
    p[1] = reserved[0];
    p[2] = reserved[1];
    p[3] = reserved[2];
    for (size_t d = 0; d < DAYS; ++d) {
      for (size_t e = 0; e < ENTRIES_PER_DAY; ++e) {
        uint8_t *entry = p + 4 + (d * ENTRIES_PER_DAY + e) * ENTRY_SIZE;
        const Entry &src = days[d][e];
        entry[0] = src.hour;
        entry[1] = src.minute;
        to_be16(entry + 2, static_cast<uint16_t>(src.setpoint * SCALE));
      }
    }
    return frame;
  }

  std::string to_string() const override {
    char buf[64];
    std::snprintf(buf, sizeof(buf),
                  "Schedule(prgMode=%u, %u days x %u entries)",
                  static_cast<unsigned>(prg_mode), static_cast<unsigned>(DAYS),
                  static_cast<unsigned>(ENTRIES_PER_DAY));
    return buf;
  }

  /// @brief Decode a Schedule message from a generic WMMM frame.
  /// @param frame The frame to interpret.
  /// @return The decoded message, or @c std::nullopt if the payload is not
  ///         exactly @ref PAYLOAD_SIZE bytes.
  static std::optional<Schedule> decode(const Frame &frame) {
    if (frame.payload.size() != PAYLOAD_SIZE) {
      return std::nullopt;
    }
    const uint8_t *p = frame.payload.data();

    Schedule msg;
    msg.prg_mode = p[0];
    msg.reserved = {p[1], p[2], p[3]};
    for (size_t d = 0; d < DAYS; ++d) {
      for (size_t e = 0; e < ENTRIES_PER_DAY; ++e) {
        const uint8_t *entry = p + 4 + (d * ENTRIES_PER_DAY + e) * ENTRY_SIZE;
        Entry &dst = msg.days[d][e];
        dst.hour = entry[0];
        dst.minute = entry[1];
        dst.setpoint =
            static_cast<float>(static_cast<int16_t>(from_be16(entry + 2))) /
            SCALE;
      }
    }
    return msg;
  }
};

} // namespace unilux::message
