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

#include "messages/changeover_cool.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::ChangeoverCool;

namespace {

Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = ChangeoverCool::ID;
  frame.payload = payload;
  return frame;
}

} // namespace

TEST(ChangeoverCool, IdMatchesConstant) {
  EXPECT_EQ(ChangeoverCool{}.id(), ChangeoverCool::ID);
  EXPECT_EQ(ChangeoverCool::ID, 0x5E);
}

TEST(ChangeoverCool, UsableThroughMessageBase) {
  ChangeoverCool c(22.0f);
  Message &msg = c;
  EXPECT_EQ(msg.id(), ChangeoverCool::ID);
  EXPECT_EQ(msg.encode().msg_id, ChangeoverCool::ID);
}

TEST(ChangeoverCoolDecode, ParsesBigEndianTenthsOfADegree) {
  EXPECT_FLOAT_EQ(
      ChangeoverCool::decode(makeFrame({0x00, 0xDC, 0x00, 0x00}))->value,
      22.0f);
  EXPECT_FLOAT_EQ(
      ChangeoverCool::decode(makeFrame({0x00, 0xE1, 0x00, 0x00}))->value,
      22.5f);
}

TEST(ChangeoverCoolDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(ChangeoverCool::decode(makeFrame({})).has_value());
  EXPECT_FALSE(ChangeoverCool::decode(makeFrame({0x00, 0xDC})).has_value());
  EXPECT_FALSE(ChangeoverCool::decode(makeFrame({0x00, 0xDC, 0x00, 0x00, 0x00}))
                   .has_value());
}

TEST(ChangeoverCoolEncode, WritesBigEndianTenthsOfADegree) {
  Frame frame = ChangeoverCool(22.5f).encode();
  EXPECT_EQ(frame.msg_id, ChangeoverCool::ID);
  EXPECT_EQ(frame.payload, (std::vector<uint8_t>{0x00, 0xE1, 0x00, 0x00}));
}

TEST(ChangeoverCoolRoundTrip, EncodeThenDecodePreservesValue) {
  for (float v : {20.0f, 22.5f, 40.0f}) {
    std::optional<ChangeoverCool> decoded =
        ChangeoverCool::decode(ChangeoverCool(v).encode());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_NEAR(decoded->value, v, 0.05f);
  }
}

TEST(ChangeoverCoolToString, FormatsValue) {
  EXPECT_EQ(ChangeoverCool(22.5f).to_string(), "ChangeoverCool(22.5)");
}
