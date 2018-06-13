// Generated file (from: fully_connected_quant8_large_weights_as_inputs.mod.py). Do not edit
void CreateModel(Model *model) {
  OperandType type3(Type::INT32, {});
  OperandType type1(Type::TENSOR_INT32, {1}, 0.04, 0);
  OperandType type2(Type::TENSOR_QUANT8_ASYMM, {1, 1}, 1.f, 0);
  OperandType type0(Type::TENSOR_QUANT8_ASYMM, {1, 5}, 0.2, 0);
  // Phase 1, operands
  auto op1 = model->addOperand(&type0);
  auto op2 = model->addOperand(&type0);
  auto b0 = model->addOperand(&type1);
  auto op3 = model->addOperand(&type2);
  auto act = model->addOperand(&type3);
  // Phase 2, operations
  static int32_t act_init[] = {0};
  model->setOperandValue(act, act_init, sizeof(int32_t) * 1);
  model->addOperation(ANEURALNETWORKS_FULLY_CONNECTED, {op1, op2, b0, act}, {op3});
  // Phase 3, inputs and outputs
  model->identifyInputsAndOutputs(
    {op1, op2, b0},
    {op3});
  assert(model->isValid());
  model->finish();
}

bool is_ignored(int i) {
  static std::set<int> ignore = {};
  return ignore.find(i) != ignore.end();
}
