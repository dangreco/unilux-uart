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

/// @file message.hpp
/// @brief Abstract base class for typed WMMM messages.
///
/// The @c wmmm.hpp layer turns a byte buffer into a generic @ref unilux::Frame
/// (a message id, three reserved bytes, and an opaque payload). This header
/// sits one level above it: a @ref unilux::Message is a concrete, strongly
/// typed view of a particular message id, with named fields in place of the raw
/// payload.
///
/// Each concrete message (see the @c messages/ directory) derives from
/// @ref unilux::Message and provides:
///   - a @c static @c constexpr @c uint8_t @c ID giving its message id;
///   - a @c static @c decode(const Frame&) factory returning the typed message,
///     or @c std::nullopt when the frame does not match;
///   - @ref encode, the inverse, producing a @ref unilux::Frame.

#include "wmmm.hpp"
#include <cstdint>
#include <string>

namespace unilux {

/// @brief Interface implemented by every typed WMMM message.
///
/// Concrete subclasses pair this interface with a static @c decode factory; the
/// virtual methods here cover the operations that do not depend on the concrete
/// type, so a decoded message can be handled through a @c Message reference.
class Message {

public:
  virtual ~Message() = default;

  /// @brief The WMMM message id this message represents.
  /// @return The id byte; matches the subclass's @c ID constant.
  virtual uint8_t id() const = 0;

  /// @brief Serialise this message into a generic WMMM frame.
  /// @return A @ref Frame whose @c msg_id is @ref id and whose payload encodes
  ///         the message's fields.
  virtual Frame encode() const = 0;

  /// @brief Render the message and its fields as a human-readable string.
  /// @return A description suitable for logging or debugging.
  virtual std::string to_string() const = 0;
};

}; // namespace unilux
