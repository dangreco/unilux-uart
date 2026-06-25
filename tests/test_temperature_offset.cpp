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

#include "messages/temperature_offset.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::TemperatureOffset;

namespace {

Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = TemperatureOffset::ID;
  frame.payload = payload;
  return frame;
}

} // namespace

TEST(TemperatureOffset, IdMatchesConstant) {
  EXPECT_EQ(TemperatureOffset{}.id(), TemperatureOffset::ID);
  EXPECT_EQ(TemperatureOffset::ID, 0x56);
}

TEST(TemperatureOffset, UsableThroughMessageBase) {
  TemperatureOffset off(1.0f);
  Message &msg = off;
  EXPECT_EQ(msg.id(), TemperatureOffset::ID);
  EXPECT_EQ(msg.encode().msg_id, TemperatureOffset::ID);
}

TEST(TemperatureOffsetDecode, ParsesBigEndianTenthsOfADegree) {
  EXPECT_FLOAT_EQ(
      TemperatureOffset::decode(makeFrame({0x00, 0x00, 0x00, 0x00}))->value,
      0.0f);
  EXPECT_FLOAT_EQ(
      TemperatureOffset::decode(makeFrame({0x00, 0x05, 0x00, 0x00}))->value,
      0.5f);
  EXPECT_FLOAT_EQ(
      TemperatureOffset::decode(makeFrame({0x00, 0x0A, 0x00, 0x00}))->value,
      1.0f);
}

TEST(TemperatureOffsetDecode, ParsesNegativeValues) {
  // -2.5 C -> -25 -> 0xFFE7 big-endian.
  std::optional<TemperatureOffset> msg =
      TemperatureOffset::decode(makeFrame({0xFF, 0xE7, 0x00, 0x00}));
  ASSERT_TRUE(msg.has_value());
  EXPECT_FLOAT_EQ(msg->value, -2.5f);
}

TEST(TemperatureOffsetDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(TemperatureOffset::decode(makeFrame({})).has_value());
  EXPECT_FALSE(TemperatureOffset::decode(makeFrame({0x00, 0x05})).has_value());
  EXPECT_FALSE(
      TemperatureOffset::decode(makeFrame({0x00, 0x05, 0x00})).has_value());
  EXPECT_FALSE(
      TemperatureOffset::decode(makeFrame({0x00, 0x05, 0x00, 0x00, 0x00}))
          .has_value());
}

TEST(TemperatureOffsetEncode, ProducesIdAndZeroedReserved) {
  Frame frame = TemperatureOffset(1.0f).encode();
  EXPECT_EQ(frame.msg_id, TemperatureOffset::ID);
  EXPECT_EQ(frame.reserved[0], 0);
  EXPECT_EQ(frame.reserved[1], 0);
  EXPECT_EQ(frame.reserved[2], 0);
}

TEST(TemperatureOffsetEncode, WritesBigEndianTenthsOfADegree) {
  EXPECT_EQ(TemperatureOffset(1.0f).encode().payload,
            (std::vector<uint8_t>{0x00, 0x0A, 0x00, 0x00}));
  EXPECT_EQ(TemperatureOffset(-2.5f).encode().payload,
            (std::vector<uint8_t>{0xFF, 0xE7, 0x00, 0x00}));
}

TEST(TemperatureOffsetRoundTrip, EncodeThenDecodePreservesValue) {
  for (float v : {-5.0f, -0.5f, 0.0f, 0.5f, 5.0f}) {
    std::optional<TemperatureOffset> decoded =
        TemperatureOffset::decode(TemperatureOffset(v).encode());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_NEAR(decoded->value, v, 0.05f);
  }
}

TEST(TemperatureOffsetToString, FormatsValue) {
  EXPECT_EQ(TemperatureOffset(1.0f).to_string(), "TemperatureOffset(1.0)");
  EXPECT_EQ(TemperatureOffset(-2.5f).to_string(), "TemperatureOffset(-2.5)");
}
