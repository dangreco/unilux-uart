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

#include "messages/switching_diff_heat.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::SwitchingDiffHeat;

namespace {

Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = SwitchingDiffHeat::ID;
  frame.payload = payload;
  return frame;
}

} // namespace

TEST(SwitchingDiffHeat, IdMatchesConstant) {
  EXPECT_EQ(SwitchingDiffHeat{}.id(), SwitchingDiffHeat::ID);
  EXPECT_EQ(SwitchingDiffHeat::ID, 0x58);
}

TEST(SwitchingDiffHeat, UsableThroughMessageBase) {
  SwitchingDiffHeat d(2.0f);
  Message &msg = d;
  EXPECT_EQ(msg.id(), SwitchingDiffHeat::ID);
  EXPECT_EQ(msg.encode().msg_id, SwitchingDiffHeat::ID);
}

TEST(SwitchingDiffHeatDecode, ParsesBigEndianTenthsOfADegree) {
  EXPECT_FLOAT_EQ(
      SwitchingDiffHeat::decode(makeFrame({0x00, 0x14, 0x00, 0x00}))->value,
      2.0f);
  EXPECT_FLOAT_EQ(
      SwitchingDiffHeat::decode(makeFrame({0x00, 0x19, 0x00, 0x00}))->value,
      2.5f);
  EXPECT_FLOAT_EQ(
      SwitchingDiffHeat::decode(makeFrame({0x00, 0x1E, 0x00, 0x00}))->value,
      3.0f);
}

TEST(SwitchingDiffHeatDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(SwitchingDiffHeat::decode(makeFrame({})).has_value());
  EXPECT_FALSE(SwitchingDiffHeat::decode(makeFrame({0x00, 0x14})).has_value());
  EXPECT_FALSE(
      SwitchingDiffHeat::decode(makeFrame({0x00, 0x14, 0x00, 0x00, 0x00}))
          .has_value());
}

TEST(SwitchingDiffHeatEncode, WritesBigEndianTenthsOfADegree) {
  Frame frame = SwitchingDiffHeat(2.0f).encode();
  EXPECT_EQ(frame.msg_id, SwitchingDiffHeat::ID);
  EXPECT_EQ(frame.payload, (std::vector<uint8_t>{0x00, 0x14, 0x00, 0x00}));
}

TEST(SwitchingDiffHeatRoundTrip, EncodeThenDecodePreservesValue) {
  for (float v : {2.0f, 2.5f, 3.0f, 4.0f}) {
    std::optional<SwitchingDiffHeat> decoded =
        SwitchingDiffHeat::decode(SwitchingDiffHeat(v).encode());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_NEAR(decoded->value, v, 0.05f);
  }
}

TEST(SwitchingDiffHeatToString, FormatsValue) {
  EXPECT_EQ(SwitchingDiffHeat(2.5f).to_string(), "SwitchingDiffHeat(2.5)");
}
