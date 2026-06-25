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

#include "messages/all_config.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::AllConfig;

namespace {

// Build a WMMM frame for the AllConfig message id with the given payload.
Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = AllConfig::ID;
  frame.payload = payload;
  return frame;
}

// A real ALL_CONFIG payload captured from the device (61 bytes).
const std::vector<uint8_t> kCaptured = {
    0x00, 0xC8, 0x00, 0x01, 0x01, 0x00, 0xF3, 0x01, 0x00, 0x14, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x2C, 0x00, 0x00, 0x00, 0x6F, 0x6B, 0x57,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xDE, 0x24,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x01, 0x00, 0xAA, 0x00, 0xDC, 0x00, 0x14, 0x42, 0x69, 0xB4,
    0x00, 0xC8, 0x00, 0x04, 0x07, 0x18};

} // namespace

// --- Identity & polymorphism -----------------------------------------------

TEST(AllConfig, IdMatchesConstant) {
  EXPECT_EQ(AllConfig{}.id(), AllConfig::ID);
  EXPECT_EQ(AllConfig::ID, 0x76);
}

TEST(AllConfig, UsableThroughMessageBase) {
  AllConfig cfg;
  Message &msg = cfg;
  EXPECT_EQ(msg.id(), AllConfig::ID);
  EXPECT_EQ(msg.encode().msg_id, AllConfig::ID);
}

// --- decode ----------------------------------------------------------------

TEST(AllConfigDecode, ParsesCapturedPayload) {
  std::optional<AllConfig> msg = AllConfig::decode(makeFrame(kCaptured));
  ASSERT_TRUE(msg.has_value());

  EXPECT_FLOAT_EQ(msg->setpoint, 20.0f);
  EXPECT_EQ(msg->setpoint_status, 0x00);
  EXPECT_EQ(msg->ctrl_mode, 0x01);
  EXPECT_EQ(msg->main_mode, 0x01);
  EXPECT_EQ(msg->temp_unit, 0x00);
  EXPECT_EQ(msg->time_format, 0xF3);
  EXPECT_EQ(msg->adaptive_ctrl, 0x01);
  EXPECT_FLOAT_EQ(msg->switching_diff, 2.0f);
  EXPECT_EQ(msg->system_mode, 0x00);
  EXPECT_EQ(msg->sensor_mode, 0x00);
  EXPECT_FLOAT_EQ(msg->internal_offset, 0.0f);
  EXPECT_FLOAT_EQ(msg->floor_limited, 30.0f);
  EXPECT_EQ(msg->power, 0x6Fu);
  EXPECT_EQ(msg->power_unit, "kW");
  EXPECT_EQ(msg->price, 0xDEu);
  EXPECT_EQ(msg->currency_unit, "$");
  EXPECT_EQ(msg->vacation_hold, 0x00);
  EXPECT_EQ(msg->fan_status, 0x03);
  EXPECT_EQ(msg->fan_mode, 0x00);
  EXPECT_EQ(msg->force_vent, 0x00);
  EXPECT_EQ(msg->change_over_mode, 0x01);
  EXPECT_FLOAT_EQ(msg->change_over_temp_heat, 17.0f);
  EXPECT_FLOAT_EQ(msg->change_over_temp_cool, 22.0f);
  EXPECT_FLOAT_EQ(msg->switching_diff_cool, 2.0f);

  EXPECT_EQ(msg->reserved_41, 0x00);
  const std::array<uint8_t, 9> expected_tail = {0x42, 0x69, 0xB4, 0x00, 0xC8,
                                                0x00, 0x04, 0x07, 0x18};
  EXPECT_EQ(msg->trailing, expected_tail);
}

TEST(AllConfigDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(AllConfig::decode(makeFrame({})).has_value());
  EXPECT_FALSE(
      AllConfig::decode(makeFrame(std::vector<uint8_t>(60, 0))).has_value());
  EXPECT_FALSE(
      AllConfig::decode(makeFrame(std::vector<uint8_t>(62, 0))).has_value());
}

// --- encode ----------------------------------------------------------------

TEST(AllConfigEncode, ProducesIdAndZeroedReserved) {
  Frame frame = AllConfig{}.encode();
  EXPECT_EQ(frame.msg_id, AllConfig::ID);
  EXPECT_EQ(frame.reserved[0], 0);
  EXPECT_EQ(frame.reserved[1], 0);
  EXPECT_EQ(frame.reserved[2], 0);
  EXPECT_EQ(frame.payload.size(), AllConfig::PAYLOAD_SIZE);
}

// --- round trip ------------------------------------------------------------

TEST(AllConfigRoundTrip, DecodeThenEncodeReproducesCapturedBytes) {
  std::optional<AllConfig> decoded = AllConfig::decode(makeFrame(kCaptured));
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->encode().payload, kCaptured);
}

TEST(AllConfigRoundTrip, EncodeThenDecodePreservesFields) {
  AllConfig original;
  original.setpoint = 21.5f;
  original.main_mode = 2;
  original.switching_diff = 1.5f;
  original.floor_limited = 28.0f;
  original.power = 1234;
  original.power_unit = "kW";
  original.price = 99;
  original.currency_unit = "EUR";
  original.fan_status = 1;
  original.change_over_temp_heat = 19.0f;
  original.change_over_temp_cool = 24.0f;
  original.switching_diff_cool = 2.5f;

  std::optional<AllConfig> decoded = AllConfig::decode(original.encode());
  ASSERT_TRUE(decoded.has_value());
  EXPECT_NEAR(decoded->setpoint, original.setpoint, 0.05f);
  EXPECT_EQ(decoded->main_mode, original.main_mode);
  EXPECT_NEAR(decoded->switching_diff, original.switching_diff, 0.05f);
  EXPECT_NEAR(decoded->floor_limited, original.floor_limited, 0.05f);
  EXPECT_EQ(decoded->power, original.power);
  EXPECT_EQ(decoded->power_unit, original.power_unit);
  EXPECT_EQ(decoded->price, original.price);
  EXPECT_EQ(decoded->currency_unit, original.currency_unit);
  EXPECT_EQ(decoded->fan_status, original.fan_status);
  EXPECT_NEAR(decoded->change_over_temp_heat, original.change_over_temp_heat,
              0.05f);
  EXPECT_NEAR(decoded->change_over_temp_cool, original.change_over_temp_cool,
              0.05f);
  EXPECT_NEAR(decoded->switching_diff_cool, original.switching_diff_cool,
              0.05f);
}

TEST(AllConfigEncode, TruncatesUnitStringsToSlotWidth) {
  AllConfig cfg;
  cfg.power_unit = "0123456789"; // longer than the 8-byte slot
  Frame frame = cfg.encode();
  std::optional<AllConfig> decoded = AllConfig::decode(frame);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->power_unit, "01234567");
}

// --- to_string -------------------------------------------------------------

TEST(AllConfigToString, MentionsKeyFields) {
  std::optional<AllConfig> msg = AllConfig::decode(makeFrame(kCaptured));
  ASSERT_TRUE(msg.has_value());
  std::string s = msg->to_string();
  EXPECT_NE(s.find("AllConfig("), std::string::npos);
  EXPECT_NE(s.find("setpoint=20.0"), std::string::npos);
}
