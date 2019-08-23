// Generated from minimum.mod.py
// DO NOT EDIT
// clang-format off
#include "TestGenerated.h"

namespace generated_tests::minimum {

void CreateModel(Model *model);
bool is_ignored(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple();

TEST_F(GeneratedTests, minimum_simple) {
    execute(CreateModel,
            is_ignored,
            get_examples_simple());
}

} // namespace generated_tests::minimum
TEST_AVAILABLE_SINCE(V1_2, minimum_simple, generated_tests::minimum::CreateModel)

namespace generated_tests::minimum {

void CreateModel_relaxed(Model *model);
bool is_ignored_relaxed(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple_relaxed();

TEST_F(GeneratedTests, minimum_simple_relaxed) {
    execute(CreateModel_relaxed,
            is_ignored_relaxed,
            get_examples_simple_relaxed());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_float16(Model *model);
bool is_ignored_float16(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple_float16();

TEST_F(GeneratedTests, minimum_simple_float16) {
    execute(CreateModel_float16,
            is_ignored_float16,
            get_examples_simple_float16());
}

} // namespace generated_tests::minimum
TEST_AVAILABLE_SINCE(V1_2, minimum_simple_float16, generated_tests::minimum::CreateModel_float16)

namespace generated_tests::minimum {

void CreateModel_int32(Model *model);
bool is_ignored_int32(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple_int32();

TEST_F(GeneratedTests, minimum_simple_int32) {
    execute(CreateModel_int32,
            is_ignored_int32,
            get_examples_simple_int32());
}

} // namespace generated_tests::minimum
TEST_AVAILABLE_SINCE(V1_2, minimum_simple_int32, generated_tests::minimum::CreateModel_int32)

namespace generated_tests::minimum {

void CreateModel_quant8(Model *model);
bool is_ignored_quant8(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple_quant8();

TEST_F(GeneratedTests, minimum_simple_quant8) {
    execute(CreateModel_quant8,
            is_ignored_quant8,
            get_examples_simple_quant8());
}

} // namespace generated_tests::minimum
TEST_AVAILABLE_SINCE(V1_2, minimum_simple_quant8, generated_tests::minimum::CreateModel_quant8)

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape(Model *model);
bool is_ignored_dynamic_output_shape(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple_dynamic_output_shape();

TEST_F(DynamicOutputShapeTest, minimum_simple_dynamic_output_shape) {
    execute(CreateModel_dynamic_output_shape,
            is_ignored_dynamic_output_shape,
            get_examples_simple_dynamic_output_shape());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_relaxed(Model *model);
bool is_ignored_dynamic_output_shape_relaxed(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple_dynamic_output_shape_relaxed();

TEST_F(DynamicOutputShapeTest, minimum_simple_dynamic_output_shape_relaxed) {
    execute(CreateModel_dynamic_output_shape_relaxed,
            is_ignored_dynamic_output_shape_relaxed,
            get_examples_simple_dynamic_output_shape_relaxed());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_float16(Model *model);
bool is_ignored_dynamic_output_shape_float16(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple_dynamic_output_shape_float16();

TEST_F(DynamicOutputShapeTest, minimum_simple_dynamic_output_shape_float16) {
    execute(CreateModel_dynamic_output_shape_float16,
            is_ignored_dynamic_output_shape_float16,
            get_examples_simple_dynamic_output_shape_float16());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_int32(Model *model);
bool is_ignored_dynamic_output_shape_int32(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple_dynamic_output_shape_int32();

TEST_F(DynamicOutputShapeTest, minimum_simple_dynamic_output_shape_int32) {
    execute(CreateModel_dynamic_output_shape_int32,
            is_ignored_dynamic_output_shape_int32,
            get_examples_simple_dynamic_output_shape_int32());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_quant8(Model *model);
bool is_ignored_dynamic_output_shape_quant8(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_simple_dynamic_output_shape_quant8();

TEST_F(DynamicOutputShapeTest, minimum_simple_dynamic_output_shape_quant8) {
    execute(CreateModel_dynamic_output_shape_quant8,
            is_ignored_dynamic_output_shape_quant8,
            get_examples_simple_dynamic_output_shape_quant8());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_2(Model *model);
bool is_ignored_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast();

TEST_F(GeneratedTests, minimum_broadcast) {
    execute(CreateModel_2,
            is_ignored_2,
            get_examples_broadcast());
}

} // namespace generated_tests::minimum
TEST_AVAILABLE_SINCE(V1_2, minimum_broadcast, generated_tests::minimum::CreateModel_2)

namespace generated_tests::minimum {

void CreateModel_relaxed_2(Model *model);
bool is_ignored_relaxed_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast_relaxed();

TEST_F(GeneratedTests, minimum_broadcast_relaxed) {
    execute(CreateModel_relaxed_2,
            is_ignored_relaxed_2,
            get_examples_broadcast_relaxed());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_float16_2(Model *model);
bool is_ignored_float16_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast_float16();

TEST_F(GeneratedTests, minimum_broadcast_float16) {
    execute(CreateModel_float16_2,
            is_ignored_float16_2,
            get_examples_broadcast_float16());
}

} // namespace generated_tests::minimum
TEST_AVAILABLE_SINCE(V1_2, minimum_broadcast_float16, generated_tests::minimum::CreateModel_float16_2)

namespace generated_tests::minimum {

void CreateModel_int32_2(Model *model);
bool is_ignored_int32_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast_int32();

TEST_F(GeneratedTests, minimum_broadcast_int32) {
    execute(CreateModel_int32_2,
            is_ignored_int32_2,
            get_examples_broadcast_int32());
}

} // namespace generated_tests::minimum
TEST_AVAILABLE_SINCE(V1_2, minimum_broadcast_int32, generated_tests::minimum::CreateModel_int32_2)

namespace generated_tests::minimum {

void CreateModel_quant8_2(Model *model);
bool is_ignored_quant8_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast_quant8();

TEST_F(GeneratedTests, minimum_broadcast_quant8) {
    execute(CreateModel_quant8_2,
            is_ignored_quant8_2,
            get_examples_broadcast_quant8());
}

} // namespace generated_tests::minimum
TEST_AVAILABLE_SINCE(V1_2, minimum_broadcast_quant8, generated_tests::minimum::CreateModel_quant8_2)

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_2(Model *model);
bool is_ignored_dynamic_output_shape_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast_dynamic_output_shape();

TEST_F(DynamicOutputShapeTest, minimum_broadcast_dynamic_output_shape) {
    execute(CreateModel_dynamic_output_shape_2,
            is_ignored_dynamic_output_shape_2,
            get_examples_broadcast_dynamic_output_shape());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_relaxed_2(Model *model);
bool is_ignored_dynamic_output_shape_relaxed_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast_dynamic_output_shape_relaxed();

TEST_F(DynamicOutputShapeTest, minimum_broadcast_dynamic_output_shape_relaxed) {
    execute(CreateModel_dynamic_output_shape_relaxed_2,
            is_ignored_dynamic_output_shape_relaxed_2,
            get_examples_broadcast_dynamic_output_shape_relaxed());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_float16_2(Model *model);
bool is_ignored_dynamic_output_shape_float16_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast_dynamic_output_shape_float16();

TEST_F(DynamicOutputShapeTest, minimum_broadcast_dynamic_output_shape_float16) {
    execute(CreateModel_dynamic_output_shape_float16_2,
            is_ignored_dynamic_output_shape_float16_2,
            get_examples_broadcast_dynamic_output_shape_float16());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_int32_2(Model *model);
bool is_ignored_dynamic_output_shape_int32_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast_dynamic_output_shape_int32();

TEST_F(DynamicOutputShapeTest, minimum_broadcast_dynamic_output_shape_int32) {
    execute(CreateModel_dynamic_output_shape_int32_2,
            is_ignored_dynamic_output_shape_int32_2,
            get_examples_broadcast_dynamic_output_shape_int32());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_quant8_2(Model *model);
bool is_ignored_dynamic_output_shape_quant8_2(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_broadcast_dynamic_output_shape_quant8();

TEST_F(DynamicOutputShapeTest, minimum_broadcast_dynamic_output_shape_quant8) {
    execute(CreateModel_dynamic_output_shape_quant8_2,
            is_ignored_dynamic_output_shape_quant8_2,
            get_examples_broadcast_dynamic_output_shape_quant8());
}

} // namespace generated_tests::minimum

namespace generated_tests::minimum {

void CreateModel_3(Model *model);
bool is_ignored_3(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_overflow();

TEST_F(GeneratedTests, minimum_overflow) {
    execute(CreateModel_3,
            is_ignored_3,
            get_examples_overflow());
}

} // namespace generated_tests::minimum
TEST_AVAILABLE_SINCE(V1_2, minimum_overflow, generated_tests::minimum::CreateModel_3)

namespace generated_tests::minimum {

void CreateModel_dynamic_output_shape_3(Model *model);
bool is_ignored_dynamic_output_shape_3(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_overflow_dynamic_output_shape();

TEST_F(DynamicOutputShapeTest, minimum_overflow_dynamic_output_shape) {
    execute(CreateModel_dynamic_output_shape_3,
            is_ignored_dynamic_output_shape_3,
            get_examples_overflow_dynamic_output_shape());
}

} // namespace generated_tests::minimum
