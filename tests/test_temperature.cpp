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

#include "messages/temperature.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::Temperature;

namespace {

// Build a WMMM frame for the Temperature message id with the given payload.
Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = Temperature::ID;
  frame.payload = payload;
  return frame;
}

} // namespace

// --- Identity & polymorphism -----------------------------------------------

TEST(Temperature, IdMatchesConstant) {
  EXPECT_EQ(Temperature{}.id(), Temperature::ID);
  EXPECT_EQ(Temperature::ID, 0x02);
}

TEST(Temperature, UsableThroughMessageBase) {
  // Public inheritance: a Temperature must be addressable as a Message.
  Temperature temp(21.5f, 22.0f);
  Message &msg = temp;
  EXPECT_EQ(msg.id(), Temperature::ID);
  EXPECT_EQ(msg.encode().msg_id, Temperature::ID);
}

// --- decode ----------------------------------------------------------------

TEST(TemperatureDecode, ParsesBigEndianTenthsOfADegree) {
  // 0x00D7 = 215 -> 21.5 C, 0x00DC = 220 -> 22.0 C.
  std::optional<Temperature> msg =
      Temperature::decode(makeFrame({0x00, 0xD7, 0x00, 0xDC}));
  ASSERT_TRUE(msg.has_value());
  EXPECT_FLOAT_EQ(msg->t1, 21.5f);
  EXPECT_FLOAT_EQ(msg->t2, 22.0f);
}

TEST(TemperatureDecode, ZeroPayloadDecodesToZero) {
  std::optional<Temperature> msg =
      Temperature::decode(makeFrame({0x00, 0x00, 0x00, 0x00}));
  ASSERT_TRUE(msg.has_value());
  EXPECT_FLOAT_EQ(msg->t1, 0.0f);
  EXPECT_FLOAT_EQ(msg->t2, 0.0f);
}

TEST(TemperatureDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(Temperature::decode(makeFrame({})).has_value());
  EXPECT_FALSE(Temperature::decode(makeFrame({0x00, 0xD7})).has_value());
  EXPECT_FALSE(Temperature::decode(makeFrame({0x00, 0xD7, 0x00})).has_value());
  EXPECT_FALSE(Temperature::decode(makeFrame({0x00, 0xD7, 0x00, 0xDC, 0x00}))
                   .has_value());
}

// --- encode ----------------------------------------------------------------

TEST(TemperatureEncode, ProducesIdAndZeroedReserved) {
  Frame frame = Temperature(21.5f, 22.0f).encode();
  EXPECT_EQ(frame.msg_id, Temperature::ID);
  EXPECT_EQ(frame.reserved[0], 0);
  EXPECT_EQ(frame.reserved[1], 0);
  EXPECT_EQ(frame.reserved[2], 0);
}

TEST(TemperatureEncode, WritesBigEndianTenthsOfADegree) {
  Frame frame = Temperature(21.5f, 22.0f).encode();
  EXPECT_EQ(frame.payload, (std::vector<uint8_t>{0x00, 0xD7, 0x00, 0xDC}));
}

// --- round trip ------------------------------------------------------------

TEST(TemperatureRoundTrip, EncodeThenDecodePreservesValues) {
  Temperature original(18.3f, 24.9f);
  std::optional<Temperature> decoded = Temperature::decode(original.encode());
  ASSERT_TRUE(decoded.has_value());
  EXPECT_NEAR(decoded->t1, original.t1, 0.05f);
  EXPECT_NEAR(decoded->t2, original.t2, 0.05f);
}

// --- to_string -------------------------------------------------------------

TEST(TemperatureToString, FormatsBothChannels) {
  EXPECT_EQ(Temperature(21.5f, 22.0f).to_string(),
            "Temperature(t1=21.5, t2=22.0)");
}
