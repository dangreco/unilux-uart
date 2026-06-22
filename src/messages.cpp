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

/// @file messages.cpp
/// @brief Implementation of frame-to-typed-message dispatch.

#include "messages.hpp"

#include <optional>
#include <variant>

namespace unilux {

namespace {

/// @brief Decode @p frame as message type @c M if its id matches.
/// @return @c true when @c frame.msg_id equals @c M::ID (whether or not the
///         payload decoded successfully), which stops the fold since message
///         ids are unique; @c false to let the next type be tried.
template <typename M>
bool try_decode(const Frame &frame, std::optional<AnyMessage> &out) {
  if (frame.msg_id != M::ID) {
    return false;
  }
  if (auto msg = M::decode(frame)) {
    out = AnyMessage{*msg};
  }
  return true;
}

/// @brief Fold over every alternative of @ref AnyMessage, dispatching by id.
///
/// The trailing pointer parameter is unused at runtime; it only carries the
/// variant's alternative types (@c Ms...) so they can be expanded in the fold.
template <typename... Ms>
std::optional<AnyMessage> decode_variant(const Frame &frame,
                                         std::variant<Ms...> *) {
  std::optional<AnyMessage> out;
  (try_decode<Ms>(frame, out) || ...);
  return out;
}

} // namespace

std::optional<AnyMessage> decode_message(const Frame &frame) {
  return decode_variant(frame, static_cast<AnyMessage *>(nullptr));
}

} // namespace unilux
