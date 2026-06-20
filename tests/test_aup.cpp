/*
 * wmmm -- a clean-room C++ library for the AUP/WMMM UART protocol.
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

#include "aup.hpp"

using namespace wmmm::aup;

namespace {

// Feed bytes that should NOT complete a frame; every byte must yield nullopt.
void feedNone(AupParser &parser, const std::vector<uint8_t> &bytes) {
  for (uint8_t byte : bytes) {
    EXPECT_FALSE(parser.consume(byte).has_value());
  }
}

// Feed a complete frame: every byte but the last yields nullopt, and the last
// byte yields the assembled frame. Returns the frame.
Aup feedFrame(AupParser &parser, const std::vector<uint8_t> &bytes) {
  EXPECT_FALSE(bytes.empty());
  for (size_t i = 0; i + 1 < bytes.size(); ++i) {
    EXPECT_FALSE(parser.consume(bytes[i]).has_value())
        << "frame completed early at byte index " << i;
  }
  std::optional<Aup> frame = parser.consume(bytes.back());
  EXPECT_TRUE(frame.has_value()) << "frame did not complete on final byte";
  return frame.value_or(Aup{});
}

// Build a well-formed AUP frame from header fields + payload.
std::vector<uint8_t> makeFrame(uint8_t checksum, uint8_t flag, uint8_t type,
                               uint8_t command,
                               const std::vector<uint8_t> &payload) {
  uint16_t length = static_cast<uint16_t>(payload.size());
  std::vector<uint8_t> bytes = {
      AUP_MAGIC1,
      AUP_MAGIC2,
      checksum,
      flag,
      type,
      command,
      static_cast<uint8_t>(length >> 8),
      static_cast<uint8_t>(length & 0xFF),
  };
  bytes.insert(bytes.end(), payload.begin(), payload.end());
  return bytes;
}

} // namespace

// --- Happy path: payload sizes ---------------------------------------------

TEST(Aup, ZeroLengthPayload) {
  AupParser parser;
  Aup frame = feedFrame(
      parser, {AUP_MAGIC1, AUP_MAGIC2, 0x11, 0x22, 0x33, 0x44, 0x00, 0x00});
  EXPECT_EQ(frame.checksum, 0x11);
  EXPECT_EQ(frame.flag, 0x22);
  EXPECT_EQ(frame.type, 0x33);
  EXPECT_EQ(frame.command, 0x44);
  EXPECT_EQ(frame.length, 0u);
  EXPECT_TRUE(frame.payload.empty());
}

TEST(Aup, SingleBytePayload) {
  AupParser parser;
  Aup frame = feedFrame(parser, makeFrame(0x01, 0x02, 0x03, 0x04, {0xAB}));
  ASSERT_EQ(frame.payload.size(), 1u);
  EXPECT_EQ(frame.payload[0], 0xAB);
}

TEST(Aup, MultiBytePayload) {
  AupParser parser;
  std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF};
  Aup frame = feedFrame(parser, makeFrame(0x10, 0x20, 0x30, 0x40, payload));
  EXPECT_EQ(frame.payload, payload);
}

// --- Header field parsing ---------------------------------------------------

TEST(Aup, FieldsParsedCorrectly) {
  AupParser parser;
  std::vector<uint8_t> payload(0x0102, 0x7A); // 258 bytes
  Aup frame = feedFrame(parser, makeFrame(0xAA, 0xBB, 0xCC, 0xDD, payload));
  EXPECT_EQ(frame.checksum, 0xAA);
  EXPECT_EQ(frame.flag, 0xBB);
  EXPECT_EQ(frame.type, 0xCC);
  EXPECT_EQ(frame.command, 0xDD);
  EXPECT_EQ(frame.length, 0x0102u);
  EXPECT_EQ(frame.payload.size(), 0x0102u);
  EXPECT_EQ(frame.length, frame.payload.size());
}

TEST(Aup, HeaderFieldsMayEqualMagic) {
  // Bytes equal to the magic markers in header positions are consumed
  // literally, not treated as the start of a new frame.
  AupParser parser;
  Aup frame = feedFrame(parser, makeFrame(AUP_MAGIC1, AUP_MAGIC2, AUP_MAGIC1,
                                          AUP_MAGIC2, {0x01}));
  EXPECT_EQ(frame.checksum, AUP_MAGIC1);
  EXPECT_EQ(frame.flag, AUP_MAGIC2);
  EXPECT_EQ(frame.type, AUP_MAGIC1);
  EXPECT_EQ(frame.command, AUP_MAGIC2);
  ASSERT_EQ(frame.payload.size(), 1u);
  EXPECT_EQ(frame.payload[0], 0x01);
}

// --- Length field (big-endian) ---------------------------------------------

TEST(Aup, BigEndianLength) {
  // length = 0x0102 = 258; the frame must complete on exactly the 258th payload
  // byte, proving MSB << 8 | LSB ordering.
  AupParser parser;
  std::vector<uint8_t> payload(258, 0x5C);
  std::vector<uint8_t> bytes = makeFrame(0, 0, 0, 0, payload);
  EXPECT_EQ(bytes[6], 0x01); // len_hi
  EXPECT_EQ(bytes[7], 0x02); // len_lo
  Aup frame = feedFrame(parser, bytes);
  EXPECT_EQ(frame.length, 0x0102u);
  EXPECT_EQ(frame.payload.size(), 258u);
}

TEST(Aup, LengthHighByteOnly) {
  // length = 0x0100 = 256; a zero LSB must not be mistaken for a zero-length
  // (no-payload) frame.
  AupParser parser;
  std::vector<uint8_t> payload(256, 0x42);
  std::vector<uint8_t> bytes = makeFrame(0, 0, 0, 0, payload);
  EXPECT_EQ(bytes[6], 0x01); // len_hi
  EXPECT_EQ(bytes[7], 0x00); // len_lo
  Aup frame = feedFrame(parser, bytes);
  EXPECT_EQ(frame.length, 0x0100u);
  EXPECT_EQ(frame.payload.size(), 256u);
}

// --- Incremental delivery ---------------------------------------------------

TEST(Aup, NoFrameUntilComplete) {
  AupParser parser;
  std::vector<uint8_t> bytes = makeFrame(0x01, 0x02, 0x03, 0x04, {0xAA, 0xBB});
  for (size_t i = 0; i + 1 < bytes.size(); ++i) {
    EXPECT_FALSE(parser.consume(bytes[i]).has_value()) << "early at " << i;
  }
  EXPECT_TRUE(parser.consume(bytes.back()).has_value());
}

// --- Resync / garbage handling ---------------------------------------------

TEST(Aup, GarbageBeforeFrame) {
  AupParser parser;
  feedNone(parser, {0x00, 0x01, 0x99, 0xFF, 0x12}); // never see MAGIC1
  Aup frame = feedFrame(parser, makeFrame(0x55, 0x66, 0x77, 0x88, {0xCD}));
  EXPECT_EQ(frame.checksum, 0x55);
  ASSERT_EQ(frame.payload.size(), 1u);
  EXPECT_EQ(frame.payload[0], 0xCD);
}

TEST(Aup, Magic1ResetOnBadByte) {
  // MAGIC1 then a non-magic byte resets; the parser must still parse the next
  // valid frame.
  AupParser parser;
  feedNone(parser, {AUP_MAGIC1, 0x00});
  Aup frame = feedFrame(parser, makeFrame(0x01, 0x02, 0x03, 0x04, {0xEE}));
  ASSERT_EQ(frame.payload.size(), 1u);
  EXPECT_EQ(frame.payload[0], 0xEE);
}

TEST(Aup, Magic2BadByteResets) {
  // MAGIC1 then a byte that is neither MAGIC2 nor MAGIC1 resets to MAGIC1.
  AupParser parser;
  feedNone(parser, {AUP_MAGIC1, 0x12});
  Aup frame = feedFrame(parser, makeFrame(0x09, 0x08, 0x07, 0x06, {0x5A}));
  EXPECT_EQ(frame.checksum, 0x09);
  ASSERT_EQ(frame.payload.size(), 1u);
  EXPECT_EQ(frame.payload[0], 0x5A);
}

TEST(Aup, DoubleMagic1) {
  // 5A 5A 9E ... -- a second MAGIC1 keeps the parser in MAGIC2.
  AupParser parser;
  std::vector<uint8_t> bytes = {AUP_MAGIC1, AUP_MAGIC1, AUP_MAGIC2, 0x01, 0x02,
                                0x03,       0x04,       0x00,       0x00};
  Aup frame = feedFrame(parser, bytes);
  EXPECT_EQ(frame.checksum, 0x01);
  EXPECT_TRUE(frame.payload.empty());
}

TEST(Aup, ManyMagic1) {
  AupParser parser;
  std::vector<uint8_t> bytes = {AUP_MAGIC1, AUP_MAGIC1, AUP_MAGIC1, AUP_MAGIC1,
                                AUP_MAGIC2, 0xAA,       0xBB,       0xCC,
                                0xDD,       0x00,       0x01,       0x7E};
  Aup frame = feedFrame(parser, bytes);
  EXPECT_EQ(frame.checksum, 0xAA);
  ASSERT_EQ(frame.payload.size(), 1u);
  EXPECT_EQ(frame.payload[0], 0x7E);
}

TEST(Aup, PartialThenResync) {
  // A partial header is abandoned by a bad byte in MAGIC2, then a full valid
  // frame is parsed.
  AupParser parser;
  feedNone(parser, {AUP_MAGIC1, 0x01}); // bad MAGIC2 -> reset
  Aup frame =
      feedFrame(parser, makeFrame(0x11, 0x22, 0x33, 0x44, {0x99, 0x88}));
  EXPECT_EQ(frame.payload, (std::vector<uint8_t>{0x99, 0x88}));
}

// --- Magic bytes inside the body -------------------------------------------

TEST(Aup, PayloadContainsMagicBytes) {
  // Once past the header, MAGIC1/MAGIC2 bytes in the payload are pure data;
  // they must not trigger a resync.
  AupParser parser;
  std::vector<uint8_t> payload = {AUP_MAGIC1, AUP_MAGIC2, AUP_MAGIC1,
                                  AUP_MAGIC2, 0x00,       AUP_MAGIC1};
  Aup frame = feedFrame(parser, makeFrame(0x01, 0x02, 0x03, 0x04, payload));
  EXPECT_EQ(frame.payload, payload);
}

// --- Stream reuse / reset ---------------------------------------------------

TEST(Aup, BackToBackFrames) {
  AupParser parser;
  Aup first =
      feedFrame(parser, makeFrame(0x01, 0x02, 0x03, 0x04, {0x10, 0x11}));
  EXPECT_EQ(first.payload, (std::vector<uint8_t>{0x10, 0x11}));

  Aup second =
      feedFrame(parser, makeFrame(0xA1, 0xA2, 0xA3, 0xA4, {0x20, 0x21, 0x22}));
  EXPECT_EQ(second.checksum, 0xA1);
  EXPECT_EQ(second.payload, (std::vector<uint8_t>{0x20, 0x21, 0x22}));
}

TEST(Aup, ReuseLargeThenZero) {
  // A large-payload frame followed by a zero-payload frame: reset() must clear
  // the previous payload so no stale bytes leak into the second frame.
  AupParser parser;
  std::vector<uint8_t> big(64, 0xC3);
  Aup first = feedFrame(parser, makeFrame(0x01, 0x02, 0x03, 0x04, big));
  EXPECT_EQ(first.payload.size(), 64u);

  Aup second = feedFrame(parser, makeFrame(0x05, 0x06, 0x07, 0x08, {}));
  EXPECT_EQ(second.checksum, 0x05);
  EXPECT_TRUE(second.payload.empty());
}

// --- Fresh parser determinism (depends on state_ init in aup.hpp) ----------

TEST(Aup, FreshParserStartsAtMagic1) {
  AupParser parser;
  EXPECT_FALSE(
      parser.consume(0x00).has_value()); // ignored as pre-frame garbage
  Aup frame = feedFrame(parser, makeFrame(0x01, 0x02, 0x03, 0x04, {0x7F}));
  ASSERT_EQ(frame.payload.size(), 1u);
  EXPECT_EQ(frame.payload[0], 0x7F);
}
