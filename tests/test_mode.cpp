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
#include <vector>

#include "messages/mode.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::Mode;

namespace {

// Build a WMMM frame for the Mode message id with the given payload.
Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = Mode::ID;
  frame.payload = payload;
  return frame;
}

} // namespace

// --- Identity & polymorphism -----------------------------------------------

TEST(Mode, IdMatchesConstant) {
  EXPECT_EQ(Mode{}.id(), Mode::ID);
  EXPECT_EQ(Mode::ID, 0x5C);
}

TEST(Mode, UsableThroughMessageBase) {
  Mode mode(Mode::Value::Cool);
  Message &msg = mode;
  EXPECT_EQ(msg.id(), Mode::ID);
  EXPECT_EQ(msg.encode().msg_id, Mode::ID);
}

// --- decode ----------------------------------------------------------------

TEST(ModeDecode, ParsesKnownModes) {
  EXPECT_EQ(Mode::decode(makeFrame({0x00, 0x00, 0x00, 0x00}))->value,
            Mode::Value::Heat);
  EXPECT_EQ(Mode::decode(makeFrame({0x01, 0x00, 0x00, 0x00}))->value,
            Mode::Value::Cool);
  EXPECT_EQ(Mode::decode(makeFrame({0x02, 0x00, 0x00, 0x00}))->value,
            Mode::Value::Auto);
}

TEST(ModeDecode, CarriesUnknownValueVerbatim) {
  std::optional<Mode> msg = Mode::decode(makeFrame({0x07, 0x00, 0x00, 0x00}));
  ASSERT_TRUE(msg.has_value());
  EXPECT_EQ(static_cast<uint8_t>(msg->value), 0x07);
}

TEST(ModeDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(Mode::decode(makeFrame({})).has_value());
  EXPECT_FALSE(Mode::decode(makeFrame({0x00})).has_value());
  EXPECT_FALSE(Mode::decode(makeFrame({0x00, 0x00, 0x00})).has_value());
  EXPECT_FALSE(
      Mode::decode(makeFrame({0x00, 0x00, 0x00, 0x00, 0x00})).has_value());
}

// --- encode ----------------------------------------------------------------

TEST(ModeEncode, ProducesIdAndZeroedReserved) {
  Frame frame = Mode(Mode::Value::Cool).encode();
  EXPECT_EQ(frame.msg_id, Mode::ID);
  EXPECT_EQ(frame.reserved[0], 0);
  EXPECT_EQ(frame.reserved[1], 0);
  EXPECT_EQ(frame.reserved[2], 0);
}

TEST(ModeEncode, WritesModeByteThenZeroes) {
  EXPECT_EQ(Mode(Mode::Value::Heat).encode().payload,
            (std::vector<uint8_t>{0x00, 0x00, 0x00, 0x00}));
  EXPECT_EQ(Mode(Mode::Value::Cool).encode().payload,
            (std::vector<uint8_t>{0x01, 0x00, 0x00, 0x00}));
  EXPECT_EQ(Mode(Mode::Value::Auto).encode().payload,
            (std::vector<uint8_t>{0x02, 0x00, 0x00, 0x00}));
}

// --- round trip ------------------------------------------------------------

TEST(ModeRoundTrip, EncodeThenDecodePreservesValue) {
  for (Mode::Value v :
       {Mode::Value::Heat, Mode::Value::Cool, Mode::Value::Auto}) {
    std::optional<Mode> decoded = Mode::decode(Mode(v).encode());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->value, v);
  }
}

// --- to_string -------------------------------------------------------------

TEST(ModeToString, FormatsKnownModes) {
  EXPECT_EQ(Mode(Mode::Value::Heat).to_string(), "Mode(heat)");
  EXPECT_EQ(Mode(Mode::Value::Cool).to_string(), "Mode(cool)");
  EXPECT_EQ(Mode(Mode::Value::Auto).to_string(), "Mode(auto)");
}

TEST(ModeToString, FormatsUnknownAsHex) {
  Mode mode;
  mode.value = static_cast<Mode::Value>(0x07);
  EXPECT_EQ(mode.to_string(), "Mode(0x07)");
}
