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

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "messages.hpp"
#include "messages/temperature.hpp"
#include "wmmm.hpp"

using unilux::AnyMessage;
using unilux::decode_message;
using unilux::Frame;
using unilux::message::Temperature;

namespace {

// Build a WMMM frame from a message id and payload.
Frame makeFrame(uint8_t msg_id, const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = msg_id;
  frame.payload = payload;
  return frame;
}

} // namespace

// --- Known id, valid payload -----------------------------------------------

TEST(DecodeMessage, DecodesTemperatureFromMatchingId) {
  // 0x00D7 = 215 -> 21.5 C, 0x00DC = 220 -> 22.0 C.
  std::optional<AnyMessage> msg =
      decode_message(makeFrame(Temperature::ID, {0x00, 0xD7, 0x00, 0xDC}));
  ASSERT_TRUE(msg.has_value());
  ASSERT_TRUE(std::holds_alternative<Temperature>(*msg));
  const Temperature &temp = std::get<Temperature>(*msg);
  EXPECT_FLOAT_EQ(temp.t1, 21.5f);
  EXPECT_FLOAT_EQ(temp.t2, 22.0f);
}

// --- Known id, invalid payload ---------------------------------------------

TEST(DecodeMessage, ReturnsNulloptForKnownIdWithBadPayload) {
  // Right id, but the payload is the wrong length for a Temperature.
  EXPECT_FALSE(
      decode_message(makeFrame(Temperature::ID, {0x00, 0xD7})).has_value());
  EXPECT_FALSE(decode_message(makeFrame(Temperature::ID, {})).has_value());
}

// --- Unknown id ------------------------------------------------------------

TEST(DecodeMessage, ReturnsNulloptForUnknownId) {
  // 0xFF is not a known message id, regardless of payload contents.
  EXPECT_FALSE(
      decode_message(makeFrame(0xFF, {0x00, 0xD7, 0x00, 0xDC})).has_value());
  EXPECT_FALSE(decode_message(makeFrame(0x00, {})).has_value());
}

// --- Dispatch correctness --------------------------------------------------

TEST(DecodeMessage, SelectsAlternativeByMessageId) {
  std::optional<AnyMessage> msg =
      decode_message(makeFrame(Temperature::ID, {0x00, 0x00, 0x00, 0x00}));
  ASSERT_TRUE(msg.has_value());
  // The decoded alternative must be Temperature's slot in the variant.
  EXPECT_EQ(msg->index(), 0u);
  EXPECT_TRUE(std::holds_alternative<Temperature>(*msg));
}

// --- Visit / switch ergonomics ---------------------------------------------

TEST(DecodeMessage, IsSwitchableWithVisit) {
  // Mirrors the intended impl-side usage: switch on the concrete type.
  std::optional<AnyMessage> msg =
      decode_message(makeFrame(Temperature::ID, {0x00, 0xD7, 0x00, 0xDC}));
  ASSERT_TRUE(msg.has_value());

  std::string handled;
  std::visit(
      [&](auto &&m) {
        using T = std::decay_t<decltype(m)>;
        if constexpr (std::is_same_v<T, Temperature>) {
          handled = m.to_string();
        }
      },
      *msg);

  EXPECT_EQ(handled, "Temperature(t1=21.5, t2=22.0)");
}
