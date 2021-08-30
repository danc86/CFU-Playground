/*
 * Copyright 2021 The CFU-Playground Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "blocks_test.h"

#include <cstdio>

#include "blocks.h"

using hps_accel::multiply_accumulate;
using hps_accel::Vector16;

namespace {

constexpr size_t BYTES_PER_WORD = 4;
constexpr size_t BYTES_PER_VECTOR = 4 * BYTES_PER_WORD;

void print(Vector16 m) {
  for (size_t y = 0; y < 4; y++) {
    for (size_t x = 0; x < 4; x++) {
      printf(" %4d", m.get(y * 4 + x));
    }
    printf("\n");
  }
}

bool check_vector_same(Vector16 actual, Vector16 expected, size_t n) {
  if (actual.same_values(expected)) {
    return true;
  }

  printf("\n FAIL\nexpected:\n");
  print(expected);
  printf("actual:\n");
  print(actual);
  printf("case: %4u\n", n);
  return false;
}

bool test_multiply(Vector16 input, Vector16 filter, int32_t input_offset,
                   int32_t expected) {
  printf(".");
  hps_accel::LoadInputOffset(input_offset);
  int32_t actual = multiply_accumulate(input, filter);
  if (actual == expected) {
    return true;
  }

  printf("\n FAIL\ninput:\n");
  print(input);
  printf("filter:\n");
  print(filter);
  printf("input_offset: %4ld\n", input_offset);
  printf("actual:       %4ld\n", actual);
  printf("expected:     %4ld\n", expected);
  return false;
}

// 1474 bytes of Shakespeare
char const* const DATA =
    "SHYLOCK\n"
    "This kindness will I show.\n"
    "Go with me to a notary, seal me there\n"
    "Your single bond; and, in a merry sport,\n"
    "If you repay me not on such a day,\n"
    "In such a place, such sum or sums as are\n"
    "Express'd in the condition, let the forfeit\n"
    "Be nominated for an equal pound\n"
    "Of your fair flesh, to be cut off and taken\n"
    "In what part of your body pleaseth me.\n"
    "ANTONIO\n"
    "Content, i' faith: I'll seal to such a bond\n"
    "And say there is much kindness in the Jew.\n"
    "BASSANIO\n"
    "You shall not seal to such a bond for me:\n"
    "I'll rather dwell in my necessity.\n"
    "ANTONIO\n"
    "Why, fear not, man; I will not forfeit it:\n"
    "Within these two months, that's a month before\n"
    "This bond expires, I do expect return\n"
    "Of thrice three times the value of this bond.\n"
    "SHYLOCK\n"
    "O father Abram, what these Christians are,\n"
    "Whose own hard dealings teaches them suspect\n"
    "The thoughts of others! Pray you, tell me this;\n"
    "If he should break his day, what should I gain\n"
    "By the exaction of the forfeiture?\n"
    "A pound of man's flesh taken from a man\n"
    "Is not so estimable, profitable neither,\n"
    "As flesh of muttons, beefs, or goats. I say,\n"
    "To buy his favour, I extend this friendship:\n"
    "If he will take it, so; if not, adieu;\n"
    "And, for my love, I pray you wrong me not.\n"
    "ANTONIO\n"
    "Yes Shylock, I will seal unto this bond.\n"
    "SHYLOCK\n"
    "Then meet me forthwith at the notary's;\n"
    "Give him direction for this merry bond,\n"
    "And I will go and purse the ducats straight,\n"
    "See to my house, left in the fearful guard\n"
    "Of an unthrifty knave, and presently\n"
    "I will be with you.\n";

void check_filters(size_t in_channels, size_t out_channels) {
  const int8_t* data_int8 = reinterpret_cast<const int8_t*>(DATA);
  hps_accel::LoadFilter(in_channels, out_channels, data_int8);

  const size_t num_vectors = in_channels * out_channels *
                             hps_accel::FILTER_WIDTH *
                             hps_accel::FILTER_HEIGHT / BYTES_PER_VECTOR;
  size_t cases = 0;
  size_t failures = 0;

  // Read filters 3 times to ensure wrap-around works
  for (size_t i = 0; i < num_vectors * 3; i++) {
    cases++;
    const int8_t* p = data_int8 + ((i % num_vectors) * 16);
    Vector16 expected =
        Vector16::build(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8],
                        p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    if (!check_vector_same(hps_accel::GetFilter(), expected, i)) failures++;
    hps_accel::AdvanceFilterInput();
  }
  printf("%s %2u,%2u: %3d cases with %1d failures\n", failures ? "FAIL" : "OK",
         in_channels, out_channels, cases, failures);
}

void check_inputs(size_t width, size_t in_channels) {
  const int8_t* data_int8 = reinterpret_cast<const int8_t*>(DATA);
  hps_accel::LoadInput(width, in_channels, data_int8);
  const size_t num_vectors = 16 * in_channels / 16;
  const size_t row_offset = width * in_channels;

  size_t cases = 0;
  size_t failures = 0;
  for (size_t i = 0; i < num_vectors * 3; i++) {
    cases++;

    // Find location of expected data and build a vector from it
    const int8_t* p = data_int8;
    const size_t v = i % num_vectors;
    Vector16 expected;
    if (in_channels == 1) {
      const size_t r = row_offset;
      expected = Vector16::build(
          p[r * 0 + 0], p[r * 0 + 1], p[r * 0 + 2], p[r * 0 + 3],  //
          p[r * 1 + 0], p[r * 1 + 1], p[r * 1 + 2], p[r * 1 + 3],  //
          p[r * 2 + 0], p[r * 2 + 1], p[r * 2 + 2], p[r * 2 + 3],  //
          p[r * 3 + 0], p[r * 3 + 1], p[r * 3 + 2], p[r * 3 + 3]);
    } else {
      const size_t vectors_per_row = num_vectors / 4;
      p += row_offset * (v / vectors_per_row);
      p += 16 * (v % vectors_per_row);
      expected =
          Vector16::build(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8],
                          p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    }
    if (!check_vector_same(hps_accel::GetInput(), expected, i)) failures++;
    hps_accel::AdvanceFilterInput();
  }
  printf("%s %2u,%2u: %3d cases with %1d failures\n", failures ? "FAIL" : "OK",
         width, in_channels, cases, failures);
}

};  // anonymous namespace

extern "C" void do_test_blocks_multiply_accumulate(void) {
  struct {
    Vector16 input;
    Vector16 filter;
    int32_t input_offset;
    int32_t expected;
  } multiply_cases[]{
      // Sum 0^2..15^2
      {Vector16::build(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
       Vector16::build(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15), 0,
       1240},
      // Introduce input offset
      {Vector16::build(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
       Vector16::build(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15),
       12, 2680},
      // Arbitary numbers: case 1
      {Vector16::build(-64, -94, -28, -5, -50, 113, -38, -104, 15, -103, 101,
                       -30, -40, -48, -88, -40),
       Vector16::build(-123, -77, 2, 100, 118, -45, 3, 97, 66, -16, -88, 0, 49,
                       94, 68, 116),
       22, -21971},
      // Arbitary numbers: case 2
      {Vector16::build(-93, 46, -5, -32, -28, 79, 84, 57, 124, 114, 47, -75,
                       -77, -91, 7, 92),
       Vector16::build(-86, 39, 120, -60, 10, -7, 2, -88, -23, -69, 106, 36,
                       -52, -36, -80, -31),
       93, -19504},
  };

  size_t cases = 0;
  size_t failures = 0;
  for (auto m : multiply_cases) {
    cases++;
    if (!test_multiply(m.input, m.filter, m.input_offset, m.expected))
      failures++;
  }
  printf("\n%s: %d cases with %d failures\n", failures ? "FAIL" : "OK", cases,
         failures);
}

extern "C" void do_test_blocks_filter(void) {
  check_filters(16, 8);
  check_filters(4, 4);
  check_filters(32, 4);
  check_filters(1, 4);
  check_filters(1, 20);
  check_filters(1, 32);
}

extern "C" void do_test_blocks_input(void) {
  check_inputs(9, 8);
  check_inputs(10, 12);
  check_inputs(99, 1);
  check_inputs(49, 4);
}

extern "C" void do_test_blocks_all(void) {
  printf("multiply_accumulate\n");
  do_test_blocks_multiply_accumulate();
  printf("filter\n");
  do_test_blocks_filter();
  printf("input\n");
  do_test_blocks_input();
}
