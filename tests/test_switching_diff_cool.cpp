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

#include "messages/switching_diff_cool.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::SwitchingDiffCool;

namespace {

Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = SwitchingDiffCool::ID;
  frame.payload = payload;
  return frame;
}

} // namespace

TEST(SwitchingDiffCool, IdMatchesConstant) {
  EXPECT_EQ(SwitchingDiffCool{}.id(), SwitchingDiffCool::ID);
  EXPECT_EQ(SwitchingDiffCool::ID, 0x59);
}

TEST(SwitchingDiffCool, UsableThroughMessageBase) {
  SwitchingDiffCool d(2.0f);
  Message &msg = d;
  EXPECT_EQ(msg.id(), SwitchingDiffCool::ID);
  EXPECT_EQ(msg.encode().msg_id, SwitchingDiffCool::ID);
}

TEST(SwitchingDiffCoolDecode, ParsesBigEndianTenthsOfADegree) {
  EXPECT_FLOAT_EQ(
      SwitchingDiffCool::decode(makeFrame({0x00, 0x14, 0x00, 0x00}))->value,
      2.0f);
  EXPECT_FLOAT_EQ(
      SwitchingDiffCool::decode(makeFrame({0x00, 0x1E, 0x00, 0x00}))->value,
      3.0f);
}

TEST(SwitchingDiffCoolDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(SwitchingDiffCool::decode(makeFrame({})).has_value());
  EXPECT_FALSE(SwitchingDiffCool::decode(makeFrame({0x00, 0x14})).has_value());
  EXPECT_FALSE(
      SwitchingDiffCool::decode(makeFrame({0x00, 0x14, 0x00, 0x00, 0x00}))
          .has_value());
}

TEST(SwitchingDiffCoolEncode, WritesBigEndianTenthsOfADegree) {
  Frame frame = SwitchingDiffCool(3.0f).encode();
  EXPECT_EQ(frame.msg_id, SwitchingDiffCool::ID);
  EXPECT_EQ(frame.payload, (std::vector<uint8_t>{0x00, 0x1E, 0x00, 0x00}));
}

TEST(SwitchingDiffCoolRoundTrip, EncodeThenDecodePreservesValue) {
  for (float v : {2.0f, 2.5f, 3.0f, 4.0f}) {
    std::optional<SwitchingDiffCool> decoded =
        SwitchingDiffCool::decode(SwitchingDiffCool(v).encode());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_NEAR(decoded->value, v, 0.05f);
  }
}

TEST(SwitchingDiffCoolToString, FormatsValue) {
  EXPECT_EQ(SwitchingDiffCool(2.5f).to_string(), "SwitchingDiffCool(2.5)");
}
