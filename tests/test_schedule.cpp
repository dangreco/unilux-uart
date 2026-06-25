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

#include "messages/schedule.hpp"
#include "wmmm.hpp"

using unilux::Frame;
using unilux::Message;
using unilux::message::Schedule;

namespace {

// Build a WMMM frame for the Schedule message id with the given payload.
Frame makeFrame(const std::vector<uint8_t> &payload) {
  Frame frame{};
  frame.msg_id = Schedule::ID;
  frame.payload = payload;
  return frame;
}

// A real ALL_PRG payload captured from the device (116 bytes): prgMode + 3
// reserved, then 7 day-blocks of 4 [HH][MM][setpoint BE16] entries.
const std::vector<uint8_t> kCaptured = {
    0x00, 0x00, 0x00, 0x00, // prgMode + reserved
    0x08, 0x00, 0x00, 0xDC, 0x0A, 0x00, 0x00, 0xAA,
    0x12, 0x00, 0x00, 0xDC, 0x17, 0x00, 0x00, 0xAA, // day 0
    0x06, 0x00, 0x00, 0xD2, 0x08, 0x00, 0x00, 0xA0,
    0x12, 0x00, 0x00, 0xD2, 0x16, 0x00, 0x00, 0xA0, // day 1
    0x06, 0x00, 0x00, 0xD2, 0x08, 0x00, 0x00, 0xA0,
    0x12, 0x00, 0x00, 0xD2, 0x16, 0x00, 0x00, 0xA0, // day 2
    0x06, 0x00, 0x00, 0xD2, 0x08, 0x00, 0x00, 0xA0,
    0x12, 0x00, 0x00, 0xD2, 0x16, 0x00, 0x00, 0xA0, // day 3
    0x06, 0x00, 0x00, 0xD2, 0x08, 0x00, 0x00, 0xA0,
    0x12, 0x00, 0x00, 0xD2, 0x16, 0x00, 0x00, 0xA0, // day 4
    0x06, 0x00, 0x00, 0xD2, 0x08, 0x00, 0x00, 0xA0,
    0x12, 0x00, 0x00, 0xD2, 0x16, 0x00, 0x00, 0xA0, // day 5
    0x08, 0x00, 0x00, 0xDC, 0x0A, 0x00, 0x00, 0xAA,
    0x12, 0x00, 0x00, 0xDC, 0x17, 0x00, 0x00, 0xAA}; // day 6

} // namespace

// --- Identity & polymorphism -----------------------------------------------

TEST(Schedule, IdMatchesConstant) {
  EXPECT_EQ(Schedule{}.id(), Schedule::ID);
  EXPECT_EQ(Schedule::ID, 0x78);
}

TEST(Schedule, PayloadSizeIsHeaderPlusGrid) {
  EXPECT_EQ(Schedule::PAYLOAD_SIZE, 116u);
}

TEST(Schedule, UsableThroughMessageBase) {
  Schedule sched;
  Message &msg = sched;
  EXPECT_EQ(msg.id(), Schedule::ID);
  EXPECT_EQ(msg.encode().msg_id, Schedule::ID);
}

// --- decode ----------------------------------------------------------------

TEST(ScheduleDecode, ParsesCapturedPayload) {
  std::optional<Schedule> msg = Schedule::decode(makeFrame(kCaptured));
  ASSERT_TRUE(msg.has_value());
  EXPECT_EQ(msg->prg_mode, 0x00);

  // Day 0 (weekend comfort profile).
  EXPECT_EQ(msg->days[0][0].hour, 8);
  EXPECT_EQ(msg->days[0][0].minute, 0);
  EXPECT_FLOAT_EQ(msg->days[0][0].setpoint, 22.0f);
  EXPECT_EQ(msg->days[0][1].hour, 10);
  EXPECT_FLOAT_EQ(msg->days[0][1].setpoint, 17.0f);
  EXPECT_EQ(msg->days[0][2].hour, 18);
  EXPECT_FLOAT_EQ(msg->days[0][2].setpoint, 22.0f);
  EXPECT_EQ(msg->days[0][3].hour, 23);
  EXPECT_FLOAT_EQ(msg->days[0][3].setpoint, 17.0f);

  // Day 1 (a weekday profile).
  EXPECT_EQ(msg->days[1][0].hour, 6);
  EXPECT_FLOAT_EQ(msg->days[1][0].setpoint, 21.0f);
  EXPECT_EQ(msg->days[1][1].hour, 8);
  EXPECT_FLOAT_EQ(msg->days[1][1].setpoint, 16.0f);
  EXPECT_EQ(msg->days[1][3].hour, 22);
  EXPECT_FLOAT_EQ(msg->days[1][3].setpoint, 16.0f);
}

TEST(ScheduleDecode, RejectsWrongPayloadSize) {
  EXPECT_FALSE(Schedule::decode(makeFrame({})).has_value());
  EXPECT_FALSE(
      Schedule::decode(makeFrame(std::vector<uint8_t>(115, 0))).has_value());
  EXPECT_FALSE(
      Schedule::decode(makeFrame(std::vector<uint8_t>(117, 0))).has_value());
}

// --- encode ----------------------------------------------------------------

TEST(ScheduleEncode, ProducesIdAndZeroedReserved) {
  Frame frame = Schedule{}.encode();
  EXPECT_EQ(frame.msg_id, Schedule::ID);
  EXPECT_EQ(frame.reserved[0], 0);
  EXPECT_EQ(frame.reserved[1], 0);
  EXPECT_EQ(frame.reserved[2], 0);
  EXPECT_EQ(frame.payload.size(), Schedule::PAYLOAD_SIZE);
}

// --- round trip ------------------------------------------------------------

TEST(ScheduleRoundTrip, DecodeThenEncodeReproducesCapturedBytes) {
  std::optional<Schedule> decoded = Schedule::decode(makeFrame(kCaptured));
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->encode().payload, kCaptured);
}

TEST(ScheduleRoundTrip, EncodeThenDecodePreservesEntries) {
  Schedule original;
  original.prg_mode = 2;
  original.days[3][1] = {7, 30, 19.5f};
  original.days[6][2] = {21, 15, 16.5f};

  std::optional<Schedule> decoded = Schedule::decode(original.encode());
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->prg_mode, 2);
  EXPECT_EQ(decoded->days[3][1].hour, 7);
  EXPECT_EQ(decoded->days[3][1].minute, 30);
  EXPECT_NEAR(decoded->days[3][1].setpoint, 19.5f, 0.05f);
  EXPECT_EQ(decoded->days[6][2].hour, 21);
  EXPECT_EQ(decoded->days[6][2].minute, 15);
  EXPECT_NEAR(decoded->days[6][2].setpoint, 16.5f, 0.05f);
}

// --- to_string -------------------------------------------------------------

TEST(ScheduleToString, MentionsModeAndShape) {
  Schedule sched;
  sched.prg_mode = 0;
  EXPECT_EQ(sched.to_string(), "Schedule(prgMode=0, 7 days x 4 entries)");
}
