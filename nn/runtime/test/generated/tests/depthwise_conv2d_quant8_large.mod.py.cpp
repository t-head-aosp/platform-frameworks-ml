// Generated from depthwise_conv2d_quant8_large.mod.py
// DO NOT EDIT
// clang-format off
#include "TestGenerated.h"

namespace generated_tests::depthwise_conv2d_quant8_large {

void CreateModel(Model *model);
bool is_ignored(int);
std::vector<::test_helper::MixedTypedExample>& get_examples();

TEST_F(GeneratedTests, depthwise_conv2d_quant8_large) {
    execute(CreateModel,
            is_ignored,
            get_examples());
}

} // namespace generated_tests::depthwise_conv2d_quant8_large
TEST_AVAILABLE_SINCE(V1_0, depthwise_conv2d_quant8_large, generated_tests::depthwise_conv2d_quant8_large::CreateModel)

namespace generated_tests::depthwise_conv2d_quant8_large {

void CreateModel_dynamic_output_shape(Model *model);
bool is_ignored_dynamic_output_shape(int);
std::vector<::test_helper::MixedTypedExample>& get_examples_dynamic_output_shape();

TEST_F(DynamicOutputShapeTest, depthwise_conv2d_quant8_large_dynamic_output_shape) {
    execute(CreateModel_dynamic_output_shape,
            is_ignored_dynamic_output_shape,
            get_examples_dynamic_output_shape());
}

} // namespace generated_tests::depthwise_conv2d_quant8_large
