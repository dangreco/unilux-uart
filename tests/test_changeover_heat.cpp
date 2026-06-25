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

#include "messages/changeover_heat.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::ChangeoverHeat;

namespace {

Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = ChangeoverHeat::ID;
  frame.payload = payload;
  return frame;
}

} // namespace

TEST(ChangeoverHeat, IdMatchesConstant) {
  EXPECT_EQ(ChangeoverHeat{}.id(), ChangeoverHeat::ID);
  EXPECT_EQ(ChangeoverHeat::ID, 0x5D);
}

TEST(ChangeoverHeat, UsableThroughMessageBase) {
  ChangeoverHeat c(17.0f);
  Message &msg = c;
  EXPECT_EQ(msg.id(), ChangeoverHeat::ID);
  EXPECT_EQ(msg.encode().msg_id, ChangeoverHeat::ID);
}

TEST(ChangeoverHeatDecode, ParsesBigEndianTenthsOfADegree) {
  EXPECT_FLOAT_EQ(
      ChangeoverHeat::decode(makeFrame({0x00, 0xAA, 0x00, 0x00}))->value,
      17.0f);
  EXPECT_FLOAT_EQ(
      ChangeoverHeat::decode(makeFrame({0x00, 0xAF, 0x00, 0x00}))->value,
      17.5f);
}

TEST(ChangeoverHeatDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(ChangeoverHeat::decode(makeFrame({})).has_value());
  EXPECT_FALSE(ChangeoverHeat::decode(makeFrame({0x00, 0xAA})).has_value());
  EXPECT_FALSE(ChangeoverHeat::decode(makeFrame({0x00, 0xAA, 0x00, 0x00, 0x00}))
                   .has_value());
}

TEST(ChangeoverHeatEncode, WritesBigEndianTenthsOfADegree) {
  Frame frame = ChangeoverHeat(17.0f).encode();
  EXPECT_EQ(frame.msg_id, ChangeoverHeat::ID);
  EXPECT_EQ(frame.payload, (std::vector<uint8_t>{0x00, 0xAA, 0x00, 0x00}));
}

TEST(ChangeoverHeatRoundTrip, EncodeThenDecodePreservesValue) {
  for (float v : {10.0f, 17.5f, 25.0f}) {
    std::optional<ChangeoverHeat> decoded =
        ChangeoverHeat::decode(ChangeoverHeat(v).encode());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_NEAR(decoded->value, v, 0.05f);
  }
}

TEST(ChangeoverHeatToString, FormatsValue) {
  EXPECT_EQ(ChangeoverHeat(17.0f).to_string(), "ChangeoverHeat(17.0)");
}
