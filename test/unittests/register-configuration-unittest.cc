// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/register-configuration.h"
#include "testing/gtest-support.h"

namespace v8 {
namespace internal {

const MachineRepresentation kFloat32 = MachineRepresentation::kFloat32;
const MachineRepresentation kFloat64 = MachineRepresentation::kFloat64;

class RegisterConfigurationUnitTest : public ::testing::Test {
 public:
  RegisterConfigurationUnitTest() {}
  virtual ~RegisterConfigurationUnitTest() {}

 private:
};

TEST_F(RegisterConfigurationUnitTest, BasicProperties) {
  const int kNumGeneralRegs = 3;
  const int kNumDoubleRegs = 4;
  const int kNumAllocatableGeneralRegs = 2;
  const int kNumAllocatableDoubleRegs = 2;
  int general_codes[kNumAllocatableGeneralRegs] = {1, 2};
  int double_codes[kNumAllocatableDoubleRegs] = {2, 3};

  RegisterConfiguration test(
      kNumGeneralRegs, kNumDoubleRegs, kNumAllocatableGeneralRegs,
      kNumAllocatableDoubleRegs, RegisterConfiguration::OVERLAP, general_codes,
      double_codes, nullptr, nullptr, nullptr);

  EXPECT_EQ(test.num_general_registers(), kNumGeneralRegs);
  EXPECT_EQ(test.num_double_registers(), kNumDoubleRegs);
  EXPECT_EQ(test.num_allocatable_general_registers(),
            kNumAllocatableGeneralRegs);
  EXPECT_EQ(test.num_allocatable_double_registers(), kNumAllocatableDoubleRegs);
  EXPECT_EQ(test.num_allocatable_float_registers(), kNumAllocatableDoubleRegs);

  EXPECT_EQ(test.allocatable_general_codes_mask(),
            (1 << general_codes[0]) | (1 << general_codes[1]));
  EXPECT_EQ(test.GetAllocatableGeneralCode(0), general_codes[0]);
  EXPECT_EQ(test.GetAllocatableGeneralCode(1), general_codes[1]);
  EXPECT_EQ(test.allocatable_double_codes_mask(),
            (1 << double_codes[0]) | (1 << double_codes[1]));
  EXPECT_EQ(test.GetAllocatableDoubleCode(0), double_codes[0]);
  EXPECT_EQ(test.GetAllocatableDoubleCode(1), double_codes[1]);
}

TEST_F(RegisterConfigurationUnitTest, Aliasing) {
  const int kNumGeneralRegs = 3;
  const int kNumDoubleRegs = 4;
  const int kNumAllocatableGeneralRegs = 2;
  const int kNumAllocatableDoubleRegs = 3;
  int general_codes[] = {1, 2};
  int double_codes[] = {2, 3, 16};  // reg 16 should not alias registers 32, 33.

  RegisterConfiguration test(
      kNumGeneralRegs, kNumDoubleRegs, kNumAllocatableGeneralRegs,
      kNumAllocatableDoubleRegs, RegisterConfiguration::COMBINE, general_codes,
      double_codes, nullptr, nullptr, nullptr);

  // There are 3 allocatable double regs, but only 2 can alias float regs.
  EXPECT_EQ(test.num_allocatable_float_registers(), 4);

  // Test that float registers combine in pairs to form double registers.
  EXPECT_EQ(test.GetAllocatableFloatCode(0), double_codes[0] * 2);
  EXPECT_EQ(test.GetAllocatableFloatCode(1), double_codes[0] * 2 + 1);
  EXPECT_EQ(test.GetAllocatableFloatCode(2), double_codes[1] * 2);
  EXPECT_EQ(test.GetAllocatableFloatCode(3), double_codes[1] * 2 + 1);

  // Registers alias themselves.
  EXPECT_TRUE(test.AreAliases(kFloat32, 0, kFloat32, 0));
  EXPECT_TRUE(test.AreAliases(kFloat64, 0, kFloat64, 0));
  // Registers don't alias other registers of the same size.
  EXPECT_FALSE(test.AreAliases(kFloat32, 1, kFloat32, 0));
  EXPECT_FALSE(test.AreAliases(kFloat64, 1, kFloat64, 0));
  // Float registers combine in pairs and alias double registers.
  EXPECT_TRUE(test.AreAliases(kFloat32, 0, kFloat64, 0));
  EXPECT_TRUE(test.AreAliases(kFloat32, 1, kFloat64, 0));
  EXPECT_TRUE(test.AreAliases(kFloat64, 0, kFloat32, 0));
  EXPECT_TRUE(test.AreAliases(kFloat64, 0, kFloat32, 1));

  EXPECT_FALSE(test.AreAliases(kFloat32, 0, kFloat64, 1));
  EXPECT_FALSE(test.AreAliases(kFloat32, 1, kFloat64, 1));

  EXPECT_TRUE(test.AreAliases(kFloat64, 0, kFloat32, 1));
  EXPECT_TRUE(test.AreAliases(kFloat64, 1, kFloat32, 2));
  EXPECT_TRUE(test.AreAliases(kFloat64, 1, kFloat32, 3));
  EXPECT_TRUE(test.AreAliases(kFloat64, 2, kFloat32, 4));
  EXPECT_TRUE(test.AreAliases(kFloat64, 2, kFloat32, 5));

  int alias_base_index = -1;
  EXPECT_EQ(test.GetAliases(kFloat32, 0, kFloat32, &alias_base_index), 1);
  EXPECT_EQ(alias_base_index, 0);
  EXPECT_EQ(test.GetAliases(kFloat64, 1, kFloat64, &alias_base_index), 1);
  EXPECT_EQ(alias_base_index, 1);
  EXPECT_EQ(test.GetAliases(kFloat32, 0, kFloat64, &alias_base_index), 1);
  EXPECT_EQ(alias_base_index, 0);
  EXPECT_EQ(test.GetAliases(kFloat32, 1, kFloat64, &alias_base_index), 1);
  EXPECT_EQ(test.GetAliases(kFloat32, 2, kFloat64, &alias_base_index), 1);
  EXPECT_EQ(alias_base_index, 1);
  EXPECT_EQ(test.GetAliases(kFloat32, 3, kFloat64, &alias_base_index), 1);
  EXPECT_EQ(alias_base_index, 1);
  EXPECT_EQ(test.GetAliases(kFloat64, 0, kFloat32, &alias_base_index), 2);
  EXPECT_EQ(alias_base_index, 0);
  EXPECT_EQ(test.GetAliases(kFloat64, 1, kFloat32, &alias_base_index), 2);
  EXPECT_EQ(alias_base_index, 2);

  // Non-allocatable codes still alias.
  EXPECT_EQ(test.GetAliases(kFloat64, 2, kFloat32, &alias_base_index), 2);
  EXPECT_EQ(alias_base_index, 4);
  // High numbered double registers don't alias nonexistent single registers.
  EXPECT_EQ(
      test.GetAliases(kFloat64, RegisterConfiguration::kMaxFPRegisters / 2,
                      kFloat32, &alias_base_index),
      0);
  EXPECT_EQ(
      test.GetAliases(kFloat64, RegisterConfiguration::kMaxFPRegisters / 2 + 1,
                      kFloat32, &alias_base_index),
      0);
  EXPECT_EQ(test.GetAliases(kFloat64, RegisterConfiguration::kMaxFPRegisters,
                            kFloat32, &alias_base_index),
            0);
}

}  // namespace internal
}  // namespace v8
