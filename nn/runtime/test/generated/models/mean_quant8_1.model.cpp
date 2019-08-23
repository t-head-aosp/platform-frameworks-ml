// Generated from mean_quant8_1.mod.py
// DO NOT EDIT
// clang-format off
#include "TestGenerated.h"

namespace generated_tests::mean_quant8_1 {

void CreateModel(Model *model) {
  OperandType type0(Type::TENSOR_QUANT8_ASYMM, {4, 3, 2}, 0.8f, 5);
  OperandType type1(Type::TENSOR_INT32, {4});
  OperandType type2(Type::INT32, {});
  OperandType type3(Type::TENSOR_QUANT8_ASYMM, {2}, 0.8f, 5);
  // Phase 1, operands
  auto input = model->addOperand(&type0);
  auto axis = model->addOperand(&type1);
  auto keepDims = model->addOperand(&type2);
  auto output = model->addOperand(&type3);
  // Phase 2, operations
  static int32_t axis_init[] = {1, 0, -3, -3};
  model->setOperandValue(axis, axis_init, sizeof(int32_t) * 4);
  static int32_t keepDims_init[] = {0};
  model->setOperandValue(keepDims, keepDims_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_MEAN, {input, axis, keepDims}, {output});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {input},
    {output});
  assert(model->isValid());
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::mean_quant8_1
namespace generated_tests::mean_quant8_1 {

void CreateModel_dynamic_output_shape(Model *model) {
  OperandType type0(Type::TENSOR_QUANT8_ASYMM, {4, 3, 2}, 0.8f, 5);
  OperandType type1(Type::TENSOR_INT32, {4});
  OperandType type2(Type::INT32, {});
  OperandType type4(Type::TENSOR_QUANT8_ASYMM, {0}, 0.8f, 5);
  // Phase 1, operands
  auto input = model->addOperand(&type0);
  auto axis = model->addOperand(&type1);
  auto keepDims = model->addOperand(&type2);
  auto output = model->addOperand(&type4);
  // Phase 2, operations
  static int32_t axis_init[] = {1, 0, -3, -3};
  model->setOperandValue(axis, axis_init, sizeof(int32_t) * 4);
  static int32_t keepDims_init[] = {0};
  model->setOperandValue(keepDims, keepDims_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_MEAN, {input, axis, keepDims}, {output});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {input},
    {output});
  assert(model->isValid());
}

bool is_ignored_dynamic_output_shape(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}

} // namespace generated_tests::mean_quant8_1