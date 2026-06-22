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

/// @file utils.hpp
/// @brief Big-endian byte-order helpers shared by the message layer.
///
/// WMMM message payloads carry multi-byte integers most-significant byte first
/// (see @c message.hpp). These free functions convert between that on-wire
/// big-endian representation and host byte order. They are kept deliberately
/// small and @c inline so any header may include them without risking a
/// One-Definition-Rule violation.

#include <cstdint>

namespace unilux {

/// @brief Read a big-endian (MSB-first) 16-bit value from a buffer.
/// @param bytes Pointer to at least two readable bytes.
/// @return The decoded value in host byte order.
inline uint16_t from_be16(const uint8_t *bytes) {
  return (static_cast<uint16_t>(bytes[0]) << 8) | bytes[1];
}

/// @brief Write a 16-bit value to a buffer in big-endian (MSB-first) order.
/// @param bytes Pointer to at least two writable bytes.
/// @param value The value to serialise.
inline void to_be16(uint8_t *bytes, uint16_t value) {
  bytes[0] = static_cast<uint8_t>(value >> 8);   // MSB
  bytes[1] = static_cast<uint8_t>(value & 0xFF); // LSB
}

} // namespace unilux
