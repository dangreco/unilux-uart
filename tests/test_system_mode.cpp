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

#include "messages/system_mode.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::SystemMode;

namespace {

Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = SystemMode::ID;
  frame.payload = payload;
  return frame;
}

} // namespace

TEST(SystemMode, IdMatchesConstant) {
  EXPECT_EQ(SystemMode{}.id(), SystemMode::ID);
  EXPECT_EQ(SystemMode::ID, 0x23);
}

TEST(SystemMode, UsableThroughMessageBase) {
  SystemMode m(SystemMode::Value::FourPipe);
  Message &msg = m;
  EXPECT_EQ(msg.id(), SystemMode::ID);
  EXPECT_EQ(msg.encode().msg_id, SystemMode::ID);
}

TEST(SystemModeDecode, ParsesKnownModes) {
  EXPECT_EQ(SystemMode::decode(makeFrame({0x00, 0x00, 0x00, 0x00}))->value,
            SystemMode::Value::TwoPipe);
  EXPECT_EQ(SystemMode::decode(makeFrame({0x01, 0x00, 0x00, 0x00}))->value,
            SystemMode::Value::FourPipe);
  EXPECT_EQ(SystemMode::decode(makeFrame({0x02, 0x00, 0x00, 0x00}))->value,
            SystemMode::Value::TwoPipeAuto);
  EXPECT_EQ(SystemMode::decode(makeFrame({0x03, 0x00, 0x00, 0x00}))->value,
            SystemMode::Value::TwoPipeHeat);
}

TEST(SystemModeDecode, CarriesUnknownValueVerbatim) {
  std::optional<SystemMode> msg =
      SystemMode::decode(makeFrame({0x09, 0x00, 0x00, 0x00}));
  ASSERT_TRUE(msg.has_value());
  EXPECT_EQ(static_cast<uint8_t>(msg->value), 0x09);
}

TEST(SystemModeDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(SystemMode::decode(makeFrame({})).has_value());
  EXPECT_FALSE(SystemMode::decode(makeFrame({0x00, 0x00, 0x00})).has_value());
}

TEST(SystemModeEncode, WritesModeByteThenZeroes) {
  EXPECT_EQ(SystemMode(SystemMode::Value::FourPipe).encode().payload,
            (std::vector<uint8_t>{0x01, 0x00, 0x00, 0x00}));
}

TEST(SystemModeRoundTrip, EncodeThenDecodePreservesValue) {
  for (SystemMode::Value v :
       {SystemMode::Value::TwoPipe, SystemMode::Value::FourPipe,
        SystemMode::Value::TwoPipeAuto, SystemMode::Value::TwoPipeHeat}) {
    std::optional<SystemMode> decoded =
        SystemMode::decode(SystemMode(v).encode());
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->value, v);
  }
}

TEST(SystemModeToString, FormatsKnownAndUnknown) {
  EXPECT_EQ(SystemMode(SystemMode::Value::FourPipe).to_string(),
            "SystemMode(4-pipe)");
  SystemMode m;
  m.value = static_cast<SystemMode::Value>(0x09);
  EXPECT_EQ(m.to_string(), "SystemMode(0x09)");
}
