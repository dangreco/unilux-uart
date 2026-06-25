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

#include "messages/display_unit.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::DisplayUnit;

namespace {

Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = DisplayUnit::ID;
  frame.payload = payload;
  return frame;
}

} // namespace

TEST(DisplayUnit, IdMatchesConstant) {
  EXPECT_EQ(DisplayUnit{}.id(), DisplayUnit::ID);
  EXPECT_EQ(DisplayUnit::ID, 0x2C);
}

TEST(DisplayUnit, UsableThroughMessageBase) {
  DisplayUnit u(DisplayUnit::Value::Fahrenheit);
  Message &msg = u;
  EXPECT_EQ(msg.id(), DisplayUnit::ID);
  EXPECT_EQ(msg.encode().msg_id, DisplayUnit::ID);
}

TEST(DisplayUnitDecode, ParsesKnownUnits) {
  EXPECT_EQ(DisplayUnit::decode(makeFrame({0x02, 0x00, 0x00, 0x00}))->value,
            DisplayUnit::Value::Celsius);
  EXPECT_EQ(DisplayUnit::decode(makeFrame({0x03, 0x00, 0x00, 0x00}))->value,
            DisplayUnit::Value::Fahrenheit);
}

TEST(DisplayUnitDecode, CarriesUnknownValueVerbatim) {
  std::optional<DisplayUnit> msg =
      DisplayUnit::decode(makeFrame({0x00, 0x00, 0x00, 0x00}));
  ASSERT_TRUE(msg.has_value());
  EXPECT_EQ(static_cast<uint8_t>(msg->value), 0x00);
}

TEST(DisplayUnitDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(DisplayUnit::decode(makeFrame({})).has_value());
  EXPECT_FALSE(DisplayUnit::decode(makeFrame({0x02, 0x00})).has_value());
}

TEST(DisplayUnitEncode, WritesUnitByteThenZeroes) {
  EXPECT_EQ(DisplayUnit(DisplayUnit::Value::Fahrenheit).encode().payload,
            (std::vector<uint8_t>{0x03, 0x00, 0x00, 0x00}));
}

TEST(DisplayUnitRoundTrip, EncodeThenDecodePreservesValue) {
  for (DisplayUnit::Value v :
       {DisplayUnit::Value::Celsius, DisplayUnit::Value::Fahrenheit}) {
    std::optional<DisplayUnit> decoded =
        DisplayUnit::decode(DisplayUnit(v).encode());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->value, v);
  }
}

TEST(DisplayUnitToString, FormatsKnownAndUnknown) {
  EXPECT_EQ(DisplayUnit(DisplayUnit::Value::Celsius).to_string(),
            "DisplayUnit(C)");
  EXPECT_EQ(DisplayUnit(DisplayUnit::Value::Fahrenheit).to_string(),
            "DisplayUnit(F)");
  DisplayUnit u;
  u.value = static_cast<DisplayUnit::Value>(0x00);
  EXPECT_EQ(u.to_string(), "DisplayUnit(0x00)");
}
