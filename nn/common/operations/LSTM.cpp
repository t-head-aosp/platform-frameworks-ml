/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "LSTM.h"

#include "CpuExecutor.h"
#include "CpuOperationUtils.h"
#include "HalInterfaces.h"

#include "Tracing.h"

namespace android {
namespace nn {

namespace {

template <typename T>
inline T* GetBuffer(RunTimeOperandInfo* operand) {
    return reinterpret_cast<T*>(operand->buffer);
}

template <typename T>
inline const T* GetBuffer(const RunTimeOperandInfo* operand) {
    return reinterpret_cast<const T*>(operand->buffer);
}

}  // anonymous namespace

LSTMCell::LSTMCell(const Operation& operation, std::vector<RunTimeOperandInfo>& operands) {
    input_ = GetInput(operation, operands, kInputTensor);

    input_to_input_weights_ =
            GetInput(operation, operands, kInputToInputWeightsTensor);  // optional
    input_to_forget_weights_ = GetInput(operation, operands, kInputToForgetWeightsTensor);
    input_to_cell_weights_ = GetInput(operation, operands, kInputToCellWeightsTensor);
    input_to_output_weights_ = GetInput(operation, operands, kInputToOutputWeightsTensor);

    recurrent_to_input_weights_ =
            GetInput(operation, operands, kRecurrentToInputWeightsTensor);  // optional
    recurrent_to_forget_weights_ = GetInput(operation, operands, kRecurrentToForgetWeightsTensor);
    recurrent_to_cell_weights_ = GetInput(operation, operands, kRecurrentToCellWeightsTensor);
    recurrent_to_output_weights_ = GetInput(operation, operands, kRecurrentToOutputWeightsTensor);

    cell_to_input_weights_ = GetInput(operation, operands, kCellToInputWeightsTensor);  // optional
    cell_to_forget_weights_ =
            GetInput(operation, operands, kCellToForgetWeightsTensor);  // optional
    cell_to_output_weights_ =
            GetInput(operation, operands, kCellToOutputWeightsTensor);  // optional

    input_gate_bias_ = GetInput(operation, operands, kInputGateBiasTensor);
    forget_gate_bias_ = GetInput(operation, operands, kForgetGateBiasTensor);
    cell_bias_ = GetInput(operation, operands, kCellGateBiasTensor);
    output_gate_bias_ = GetInput(operation, operands, kOutputGateBiasTensor);

    projection_weights_ = GetInput(operation, operands, kProjectionWeightsTensor);  // optional
    projection_bias_ = GetInput(operation, operands, kProjectionBiasTensor);        // optional

    output_state_in_ = GetInput(operation, operands, kOutputStateInTensor);
    cell_state_in_ = GetInput(operation, operands, kCellStateInTensor);

    params_.activation_ = static_cast<TfLiteFusedActivation>(
            getScalarData<int32_t>(*GetInput(operation, operands, kActivationParam)));
    if (input_->type == OperandType::TENSOR_FLOAT32) {
        params_.cell_clip_ = getScalarData<float>(*GetInput(operation, operands, kCellClipParam));
        params_.proj_clip_ = getScalarData<float>(*GetInput(operation, operands, kProjClipParam));
    } else {
        params_.cell_clip_ = static_cast<float>(
                getScalarData<_Float16>(*GetInput(operation, operands, kCellClipParam)));
        params_.proj_clip_ = static_cast<float>(
                getScalarData<_Float16>(*GetInput(operation, operands, kProjClipParam)));
    }

    // We check the version of LSTM by checking the number of the inputs to the
    // op. For LSTM version 1.0 there were 23 inputs and for 1.2 there are 27.
    if (operation.inputs.size() == 27) {
        input_layer_norm_weights_ = GetInput(operation, operands, kInputLayerNormWeightsTensor);
        forget_layer_norm_weights_ = GetInput(operation, operands, kForgetLayerNormWeightsTensor);
        cell_layer_norm_weights_ = GetInput(operation, operands, kCellLayerNormWeightsTensor);
        output_layer_norm_weights_ = GetInput(operation, operands, kOutputLayerNormWeightsTensor);
    } else {
        // For LSTM from HAL v1.0 assign operands with no values
        static RunTimeOperandInfo no_value;
        no_value.lifetime = OperandLifeTime::NO_VALUE;

        input_layer_norm_weights_ = &no_value;
        forget_layer_norm_weights_ = &no_value;
        cell_layer_norm_weights_ = &no_value;
        output_layer_norm_weights_ = &no_value;
    }

    output_state_out_ = GetOutput(operation, operands, kOutputStateOutTensor);
    cell_state_out_ = GetOutput(operation, operands, kCellStateOutTensor);
    output_ = GetOutput(operation, operands, kOutputTensor);

    scratch_buffer_ = GetOutput(operation, operands, kScratchBufferTensor);
}

bool LSTMCell::CheckInputTensorDimensions(const Operation& operation,
                                          std::vector<RunTimeOperandInfo>& operands,
                                          uint32_t n_input, uint32_t n_output, uint32_t n_cell) {
    // Making sure clipping parameters have valid values.
    // == 0 means no clipping
    //  > 0 means clipping
    NN_CHECK(params_.cell_clip_ >= 0);
    NN_CHECK(params_.proj_clip_ >= 0);

    if (!IsNullInput(input_to_input_weights_)) {
        NN_CHECK_EQ(NumDimensions(input_to_input_weights_), 2);
        NN_CHECK_EQ(SizeOfDimension(input_to_input_weights_, 0), n_cell);
        NN_CHECK_EQ(SizeOfDimension(input_to_input_weights_, 1), n_input);
    }

    NN_CHECK_EQ(NumDimensions(input_to_forget_weights_), 2);
    NN_CHECK_EQ(SizeOfDimension(input_to_forget_weights_, 0), n_cell);
    NN_CHECK_EQ(SizeOfDimension(input_to_forget_weights_, 1), n_input);

    NN_CHECK_EQ(NumDimensions(input_to_cell_weights_), 2);
    NN_CHECK_EQ(SizeOfDimension(input_to_cell_weights_, 0), n_cell);
    NN_CHECK_EQ(SizeOfDimension(input_to_cell_weights_, 1), n_input);

    if (!IsNullInput(recurrent_to_input_weights_)) {
        NN_CHECK_EQ(NumDimensions(recurrent_to_input_weights_), 2);
        NN_CHECK_EQ(SizeOfDimension(recurrent_to_input_weights_, 0), n_cell);
        NN_CHECK_EQ(SizeOfDimension(recurrent_to_input_weights_, 1), n_output);
    }

    NN_CHECK_EQ(NumDimensions(recurrent_to_forget_weights_), 2);
    NN_CHECK_EQ(SizeOfDimension(recurrent_to_forget_weights_, 0), n_cell);
    NN_CHECK_EQ(SizeOfDimension(recurrent_to_forget_weights_, 1), n_output);

    NN_CHECK_EQ(NumDimensions(recurrent_to_cell_weights_), 2);
    NN_CHECK_EQ(SizeOfDimension(recurrent_to_cell_weights_, 0), n_cell);
    NN_CHECK_EQ(SizeOfDimension(recurrent_to_cell_weights_, 1), n_output);

    // We make sure the input-gate's parameters are either both present (regular
    // LSTM) or not at all (CIFG-LSTM).
    const bool cifg_weights_all_or_none =
            (!IsNullInput(input_to_input_weights_) && !IsNullInput(recurrent_to_input_weights_)) ||
            (IsNullInput(input_to_input_weights_) && IsNullInput(recurrent_to_input_weights_));
    NN_CHECK(cifg_weights_all_or_none);

    if (!IsNullInput(cell_to_input_weights_)) {
        NN_CHECK_EQ(NumDimensions(cell_to_input_weights_), 1);
        NN_CHECK_EQ(SizeOfDimension(cell_to_input_weights_, 0), n_cell);
    }

    if (!IsNullInput(cell_to_forget_weights_)) {
        NN_CHECK_EQ(NumDimensions(cell_to_forget_weights_), 1);
        NN_CHECK_EQ(SizeOfDimension(cell_to_forget_weights_, 0), n_cell);
    }

    if (!IsNullInput(cell_to_output_weights_)) {
        NN_CHECK_EQ(NumDimensions(cell_to_output_weights_), 1);
        NN_CHECK_EQ(SizeOfDimension(cell_to_output_weights_, 0), n_cell);
    }

    // Making sure the peephole weights are there all or none.
    const bool use_cifg = IsNullInput(input_to_input_weights_);
    const bool peephole_weights_all_or_none =
            ((!IsNullInput(cell_to_input_weights_) || use_cifg) &&
             !IsNullInput(cell_to_forget_weights_) && !IsNullInput(cell_to_output_weights_)) ||
            (IsNullInput(cell_to_input_weights_) && IsNullInput(cell_to_forget_weights_) &&
             IsNullInput(cell_to_output_weights_));
    NN_CHECK(peephole_weights_all_or_none);

    // Make sure the input gate bias is present only when not a CIFG-LSTM.
    if (use_cifg) {
        NN_CHECK(IsNullInput(input_gate_bias_));
    } else {
        NN_CHECK_EQ(NumDimensions(input_gate_bias_), 1);
        NN_CHECK_EQ(SizeOfDimension(input_gate_bias_, 0), n_cell);
    }

    NN_CHECK_EQ(NumDimensions(forget_gate_bias_), 1);
    NN_CHECK_EQ(SizeOfDimension(forget_gate_bias_, 0), n_cell);

    NN_CHECK_EQ(NumDimensions(cell_bias_), 1);
    NN_CHECK_EQ(SizeOfDimension(cell_bias_, 0), n_cell);

    NN_CHECK_EQ(NumDimensions(output_gate_bias_), 1);
    NN_CHECK_EQ(SizeOfDimension(output_gate_bias_, 0), n_cell);

    if (!IsNullInput(projection_weights_)) {
        NN_CHECK_EQ(NumDimensions(projection_weights_), 2);
        NN_CHECK_EQ(SizeOfDimension(projection_weights_, 0), n_output);
        NN_CHECK_EQ(SizeOfDimension(projection_weights_, 1), n_cell);
    }

    if (!IsNullInput(projection_bias_)) {
        NN_CHECK_EQ(NumDimensions(projection_bias_), 1);
        NN_CHECK_EQ(SizeOfDimension(projection_bias_, 0), n_output);
    }

    // Making sure the projection tensors are consistent:
    // 1) If projection weight is not present, then projection bias should not be
    // present.
    // 2) If projection weight is present, then projection bias is optional.
    // TODO: make sure this is correct.
    const bool projecton_tensors_consistent =
            (!IsNullInput(projection_weights_) || IsNullInput(projection_bias_));
    NN_CHECK(projecton_tensors_consistent == true);

    if (!IsNullInput(input_layer_norm_weights_)) {
        NN_CHECK_EQ(NumDimensions(input_layer_norm_weights_), 1);
        NN_CHECK_EQ(SizeOfDimension(input_layer_norm_weights_, 0), n_cell);
    }
    if (!IsNullInput(forget_layer_norm_weights_)) {
        NN_CHECK_EQ(NumDimensions(forget_layer_norm_weights_), 1);
        NN_CHECK_EQ(SizeOfDimension(forget_layer_norm_weights_, 0), n_cell);
    }
    if (!IsNullInput(cell_layer_norm_weights_)) {
        NN_CHECK_EQ(NumDimensions(cell_layer_norm_weights_), 1);
        NN_CHECK_EQ(SizeOfDimension(cell_layer_norm_weights_, 0), n_cell);
    }
    if (!IsNullInput(output_layer_norm_weights_)) {
        NN_CHECK_EQ(NumDimensions(output_layer_norm_weights_), 1);
        NN_CHECK_EQ(SizeOfDimension(output_layer_norm_weights_, 0), n_cell);
    }

    const bool layer_norm_weights_all_or_none =
            (IsNullInput(input_layer_norm_weights_) && IsNullInput(forget_layer_norm_weights_) &&
             IsNullInput(cell_layer_norm_weights_) && IsNullInput(input_layer_norm_weights_)) ||
            (!IsNullInput(input_layer_norm_weights_) && !IsNullInput(forget_layer_norm_weights_) &&
             !IsNullInput(cell_layer_norm_weights_) && !IsNullInput(input_layer_norm_weights_));
    NN_CHECK(layer_norm_weights_all_or_none);

    return true;
}

bool LSTMCell::Prepare(const Operation& operation, std::vector<RunTimeOperandInfo>& operands,
                       Shape* scratchShape, Shape* outputStateShape, Shape* cellStateShape,
                       Shape* outputShape) {
    // Check we have all the inputs and outputs we need.
    NN_CHECK(NumInputsWithValues(operation, operands) >= 15 &&
             NumInputsWithValues(operation, operands) <= 27);
    NN_CHECK_EQ(NumOutputs(operation), 4);

    // Inferring batch size, number of outputs and number of cells from the
    // input tensors.
    NN_CHECK(NumDimensions(input_) > 1);
    const uint32_t n_batch = SizeOfDimension(input_, 0);
    const uint32_t n_input = SizeOfDimension(input_, 1);

    const uint32_t n_cell = SizeOfDimension(input_to_output_weights_, 0);
    NN_CHECK_EQ(NumDimensions(input_to_output_weights_), 2);
    NN_CHECK_EQ(SizeOfDimension(input_to_output_weights_, 1), n_input);

    NN_CHECK_EQ(NumDimensions(recurrent_to_output_weights_), 2);
    NN_CHECK_EQ(SizeOfDimension(recurrent_to_output_weights_, 0), n_cell);
    const uint32_t n_output = SizeOfDimension(recurrent_to_output_weights_, 1);

    // Check that input tensor dimensions matches with each other.
    if (!CheckInputTensorDimensions(operation, operands, n_input, n_output, n_cell)) {
        return false;
    }

    // Resize the output and output_state tensors.
    const Shape& inputShape = input_->shape();

    outputShape->type = inputShape.type;
    outputShape->dimensions = {n_batch, n_output};
    outputShape->offset = inputShape.offset;
    outputShape->scale = inputShape.scale;

    outputStateShape->type = inputShape.type;
    outputStateShape->dimensions = {n_batch, n_output};
    outputStateShape->offset = inputShape.offset;
    outputStateShape->scale = inputShape.scale;

    cellStateShape->type = inputShape.type;
    cellStateShape->dimensions = {n_batch, n_cell};
    cellStateShape->offset = inputShape.offset;
    cellStateShape->scale = inputShape.scale;

    const bool use_cifg = IsNullInput(input_to_input_weights_);
    if (use_cifg) {
        // Reserving space for Cell, Forget, Output gates
        scratchShape->dimensions = {n_batch, n_cell * 3};
    } else {
        // Reserving space for Input, Cell, Forget, Output gates
        scratchShape->dimensions = {n_batch, n_cell * 4};
    }
    scratchShape->type = inputShape.type;
    scratchShape->offset = inputShape.offset;
    scratchShape->scale = inputShape.scale;

    return true;
}

bool LSTMCell::EvalFloat32(
        const float* input_buffer, const float* input_to_input_weights_buffer,
        const float* input_to_forget_weights_buffer, const float* input_to_cell_weights_buffer,
        const float* input_to_output_weights_buffer, const float* recurrent_to_input_weights_buffer,
        const float* recurrent_to_forget_weights_buffer,
        const float* recurrent_to_cell_weights_buffer,
        const float* recurrent_to_output_weights_buffer, const float* cell_to_input_weights_buffer,
        const float* cell_to_forget_weights_buffer, const float* cell_to_output_weights_buffer,
        const float* input_gate_bias_buffer, const float* forget_gate_bias_buffer,
        const float* cell_bias_buffer, const float* output_gate_bias_buffer,
        const float* projection_weights_buffer, const float* projection_bias_buffer,
        const float* output_state_in_buffer, const float* cell_state_in_buffer,
        const float* input_layer_norm_weights_buffer, const float* forget_layer_norm_weights_buffer,
        const float* cell_layer_norm_weights_buffer, const float* output_layer_norm_weights_buffer,
        float* output_state_out_buffer, float* cell_state_out_buffer, float* output_buffer,
        float* scratch_buffer_buffer) {
    NNTRACE_COMP("LSTMCell::Eval");

    const uint32_t n_batch = input_->shape().dimensions[0];
    const uint32_t n_input = input_->shape().dimensions[1];
    // n_cell and n_output will be the same size when there is no projection.
    const uint32_t n_cell = input_to_output_weights_->shape().dimensions[0];
    const uint32_t n_output = recurrent_to_output_weights_->shape().dimensions[1];

    // Since we have already checked that weights are all there or none, we can
    // check the existence of only one to the get the condition.
    const bool use_cifg = IsNullInput(input_to_input_weights_);
    const bool use_peephole = !IsNullInput(cell_to_output_weights_);
    const bool use_layer_norm = !IsNullInput(input_layer_norm_weights_);

    // Index the scratch buffers pointers to the global scratch buffer.
    float* input_gate_scratch = nullptr;
    float* cell_scratch = nullptr;
    float* forget_gate_scratch = nullptr;
    float* output_gate_scratch = nullptr;
    if (use_cifg) {
        cell_scratch = scratch_buffer_buffer;
        forget_gate_scratch = cell_scratch + n_cell * n_batch;
        output_gate_scratch = cell_scratch + 2 * n_cell * n_batch;
    } else {
        input_gate_scratch = scratch_buffer_buffer;
        cell_scratch = input_gate_scratch + n_cell * n_batch;
        forget_gate_scratch = input_gate_scratch + 2 * n_cell * n_batch;
        output_gate_scratch = input_gate_scratch + 3 * n_cell * n_batch;
    }

    if (!use_layer_norm) {
        // Initialize scratch buffers with bias.
        if (!use_cifg) {
            tflite::tensor_utils::VectorBatchVectorAssign(input_gate_bias_buffer, n_cell, n_batch,
                                                          input_gate_scratch);
        }
        tflite::tensor_utils::VectorBatchVectorAssign(forget_gate_bias_buffer, n_cell, n_batch,
                                                      forget_gate_scratch);
        tflite::tensor_utils::VectorBatchVectorAssign(cell_bias_buffer, n_cell, n_batch,
                                                      cell_scratch);
        tflite::tensor_utils::VectorBatchVectorAssign(output_gate_bias_buffer, n_cell, n_batch,
                                                      output_gate_scratch);
    } else {
        // Initialize scratch buffers with zeroes.
        if (!use_cifg) {
            tflite::tensor_utils::ZeroVector(input_gate_scratch, n_cell * n_batch);
        }
        tflite::tensor_utils::ZeroVector(forget_gate_scratch, n_cell * n_batch);
        tflite::tensor_utils::ZeroVector(cell_scratch, n_cell * n_batch);
        tflite::tensor_utils::ZeroVector(output_gate_scratch, n_cell * n_batch);
    }

    // For each batch and cell: compute input_weight * input.
    if (!use_cifg) {
        tflite::tensor_utils::MatrixBatchVectorMultiplyAccumulate(
                input_to_input_weights_buffer, n_cell, n_input, input_buffer, n_batch,
                input_gate_scratch, /*result_stride*/ 1);
    }
    tflite::tensor_utils::MatrixBatchVectorMultiplyAccumulate(
            input_to_forget_weights_buffer, n_cell, n_input, input_buffer, n_batch,
            forget_gate_scratch, /*result_stride*/ 1);
    tflite::tensor_utils::MatrixBatchVectorMultiplyAccumulate(input_to_cell_weights_buffer, n_cell,
                                                              n_input, input_buffer, n_batch,
                                                              cell_scratch, /*result_stride*/ 1);
    tflite::tensor_utils::MatrixBatchVectorMultiplyAccumulate(
            input_to_output_weights_buffer, n_cell, n_input, input_buffer, n_batch,
            output_gate_scratch, /*result_stride*/ 1);

    // For each batch and cell: compute recurrent_weight * output_state.
    if (!use_cifg) {
        tflite::tensor_utils::MatrixBatchVectorMultiplyAccumulate(
                recurrent_to_input_weights_buffer, n_cell, n_output, output_state_in_buffer,
                n_batch, input_gate_scratch,
                /*result_stride*/ 1);
    }
    tflite::tensor_utils::MatrixBatchVectorMultiplyAccumulate(
            recurrent_to_forget_weights_buffer, n_cell, n_output, output_state_in_buffer, n_batch,
            forget_gate_scratch, /*result_stride*/ 1);
    tflite::tensor_utils::MatrixBatchVectorMultiplyAccumulate(
            recurrent_to_cell_weights_buffer, n_cell, n_output, output_state_in_buffer, n_batch,
            cell_scratch, /*result_stride*/ 1);
    tflite::tensor_utils::MatrixBatchVectorMultiplyAccumulate(
            recurrent_to_output_weights_buffer, n_cell, n_output, output_state_in_buffer, n_batch,
            output_gate_scratch, /*result_stride*/ 1);

    // For each batch and cell: update input gate.
    if (!use_cifg) {
        if (use_peephole) {
            tflite::tensor_utils::VectorBatchVectorCwiseProductAccumulate(
                    cell_to_input_weights_buffer, n_cell, cell_state_in_buffer, n_batch,
                    input_gate_scratch);
        }
        if (use_layer_norm) {
            tflite::tensor_utils::MeanStddevNormalization(input_gate_scratch, input_gate_scratch,
                                                          n_cell, n_batch, kLayerNormEpsilon);
            tflite::tensor_utils::VectorBatchVectorCwiseProduct(input_layer_norm_weights_buffer,
                                                                n_cell, input_gate_scratch, n_batch,
                                                                input_gate_scratch);
            tflite::tensor_utils::VectorBatchVectorAdd(input_gate_bias_buffer, n_cell, n_batch,
                                                       input_gate_scratch);
        }
        tflite::tensor_utils::ApplySigmoidToVector(input_gate_scratch, n_cell * n_batch,
                                                   input_gate_scratch);
    }

    // For each batch and cell: update forget gate.
    if (use_peephole) {
        tflite::tensor_utils::VectorBatchVectorCwiseProductAccumulate(cell_to_forget_weights_buffer,
                                                                      n_cell, cell_state_in_buffer,
                                                                      n_batch, forget_gate_scratch);
    }
    if (use_layer_norm) {
        tflite::tensor_utils::MeanStddevNormalization(forget_gate_scratch, forget_gate_scratch,
                                                      n_cell, n_batch, kLayerNormEpsilon);
        tflite::tensor_utils::VectorBatchVectorCwiseProduct(forget_layer_norm_weights_buffer,
                                                            n_cell, forget_gate_scratch, n_batch,
                                                            forget_gate_scratch);
        tflite::tensor_utils::VectorBatchVectorAdd(forget_gate_bias_buffer, n_cell, n_batch,
                                                   forget_gate_scratch);
    }
    tflite::tensor_utils::ApplySigmoidToVector(forget_gate_scratch, n_cell * n_batch,
                                               forget_gate_scratch);

    // For each batch and cell: update the cell.
    if (use_layer_norm) {
        tflite::tensor_utils::MeanStddevNormalization(cell_scratch, cell_scratch, n_cell, n_batch,
                                                      kLayerNormEpsilon);
        tflite::tensor_utils::VectorBatchVectorCwiseProduct(cell_layer_norm_weights_buffer, n_cell,
                                                            cell_scratch, n_batch, cell_scratch);
        tflite::tensor_utils::VectorBatchVectorAdd(cell_bias_buffer, n_cell, n_batch, cell_scratch);
    }
    tflite::tensor_utils::VectorVectorCwiseProduct(forget_gate_scratch, cell_state_in_buffer,
                                                   n_batch * n_cell, cell_state_out_buffer);
    tflite::tensor_utils::ApplyActivationToVector(cell_scratch, n_batch * n_cell,
                                                  params_.activation_, cell_scratch);
    if (use_cifg) {
        tflite::tensor_utils::Sub1Vector(forget_gate_scratch, n_batch * n_cell,
                                         forget_gate_scratch);
        tflite::tensor_utils::VectorVectorCwiseProductAccumulate(
                cell_scratch, forget_gate_scratch, n_batch * n_cell, cell_state_out_buffer);
    } else {
        tflite::tensor_utils::VectorVectorCwiseProductAccumulate(
                cell_scratch, input_gate_scratch, n_batch * n_cell, cell_state_out_buffer);
    }
    if (params_.cell_clip_ > 0.0) {
        tflite::tensor_utils::ClipVector(cell_state_out_buffer, n_batch * n_cell,
                                         params_.cell_clip_, cell_state_out_buffer);
    }

    // For each batch and cell: update the output gate.
    if (use_peephole) {
        tflite::tensor_utils::VectorBatchVectorCwiseProductAccumulate(cell_to_output_weights_buffer,
                                                                      n_cell, cell_state_out_buffer,
                                                                      n_batch, output_gate_scratch);
    }
    if (use_layer_norm) {
        tflite::tensor_utils::MeanStddevNormalization(output_gate_scratch, output_gate_scratch,
                                                      n_cell, n_batch, kLayerNormEpsilon);
        tflite::tensor_utils::VectorBatchVectorCwiseProduct(output_layer_norm_weights_buffer,
                                                            n_cell, output_gate_scratch, n_batch,
                                                            output_gate_scratch);
        tflite::tensor_utils::VectorBatchVectorAdd(output_gate_bias_buffer, n_cell, n_batch,
                                                   output_gate_scratch);
    }
    tflite::tensor_utils::ApplySigmoidToVector(output_gate_scratch, n_batch * n_cell,
                                               output_gate_scratch);
    tflite::tensor_utils::ApplyActivationToVector(cell_state_out_buffer, n_batch * n_cell,
                                                  params_.activation_, cell_scratch);
    tflite::tensor_utils::VectorVectorCwiseProduct(output_gate_scratch, cell_scratch,
                                                   n_batch * n_cell, output_gate_scratch);

    // For each batch: update the projection and output_state.
    const bool use_projection_weight = (projection_weights_->lifetime != OperandLifeTime::NO_VALUE);
    const bool use_projection_bias = (projection_bias_->lifetime != OperandLifeTime::NO_VALUE);
    if (use_projection_weight) {
        if (use_projection_bias) {
            tflite::tensor_utils::VectorBatchVectorAssign(projection_bias_buffer, n_output, n_batch,
                                                          output_buffer);
        } else {
            tflite::tensor_utils::ZeroVector(output_buffer, n_batch * n_output);
        }
        tflite::tensor_utils::MatrixBatchVectorMultiplyAccumulate(
                projection_weights_buffer, n_output, n_cell, output_gate_scratch, n_batch,
                output_buffer,
                /*result_stride*/ 1);
        if (params_.proj_clip_ > 0.0) {
            tflite::tensor_utils::ClipVector(output_buffer, n_batch * n_output, params_.proj_clip_,
                                             output_buffer);
        }
    } else {
        tflite::tensor_utils::CopyVector(output_gate_scratch, n_batch * n_output, output_buffer);
    }
    tflite::tensor_utils::CopyVector(output_buffer, n_batch * n_output, output_state_out_buffer);
    return true;
}

bool LSTMCell::Eval() {
    switch (input_->type) {
        case OperandType::TENSOR_FLOAT32: {
            // clang-format off
            EvalFloat32(
                    GetBuffer<const float>(input_),
                    GetBuffer<const float>(input_to_input_weights_),
                    GetBuffer<const float>(input_to_forget_weights_),
                    GetBuffer<const float>(input_to_cell_weights_),
                    GetBuffer<const float>(input_to_output_weights_),
                    GetBuffer<const float>(recurrent_to_input_weights_),
                    GetBuffer<const float>(recurrent_to_forget_weights_),
                    GetBuffer<const float>(recurrent_to_cell_weights_),
                    GetBuffer<const float>(recurrent_to_output_weights_),
                    GetBuffer<const float>(cell_to_input_weights_),
                    GetBuffer<const float>(cell_to_forget_weights_),
                    GetBuffer<const float>(cell_to_output_weights_),
                    GetBuffer<const float>(input_gate_bias_),
                    GetBuffer<const float>(forget_gate_bias_),
                    GetBuffer<const float>(cell_bias_),
                    GetBuffer<const float>(output_gate_bias_),
                    GetBuffer<const float>(projection_weights_),
                    GetBuffer<const float>(projection_bias_),
                    GetBuffer<const float>(output_state_in_),
                    GetBuffer<const float>(cell_state_in_),
                    GetBuffer<const float>(input_layer_norm_weights_),
                    GetBuffer<const float>(forget_layer_norm_weights_),
                    GetBuffer<const float>(cell_layer_norm_weights_),
                    GetBuffer<const float>(output_layer_norm_weights_),
                    GetBuffer<float>(output_state_out_),
                    GetBuffer<float>(cell_state_out_),
                    GetBuffer<float>(output_),
                    GetBuffer<float>(scratch_buffer_));
            // clang-format on
        } break;
        case OperandType::TENSOR_FLOAT16: {
            std::vector<float> input_float32(getNumberOfElements(input_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(input_), &input_float32);
            const float* input_to_input_weights_buffer = nullptr;
            std::vector<float> input_to_input_weights_float32(
                    getNumberOfElements(input_to_input_weights_->shape()));
            if (!IsNullInput(input_to_input_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(input_to_input_weights_),
                                        &input_to_input_weights_float32);
                input_to_input_weights_buffer = input_to_input_weights_float32.data();
            }
            std::vector<float> input_to_forget_weights_float32(
                    getNumberOfElements(input_to_forget_weights_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(input_to_forget_weights_),
                                    &input_to_forget_weights_float32);
            std::vector<float> input_to_cell_weights_float32(
                    getNumberOfElements(input_to_cell_weights_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(input_to_cell_weights_),
                                    &input_to_cell_weights_float32);
            std::vector<float> input_to_output_weights_float32(
                    getNumberOfElements(input_to_output_weights_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(input_to_output_weights_),
                                    &input_to_output_weights_float32);
            const float* recurrent_to_input_weights_buffer = nullptr;
            std::vector<float> recurrent_to_input_weights_float32(
                    getNumberOfElements(recurrent_to_input_weights_->shape()));
            if (!IsNullInput(recurrent_to_input_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(recurrent_to_input_weights_),
                                        &recurrent_to_input_weights_float32);
                recurrent_to_input_weights_buffer = recurrent_to_input_weights_float32.data();
            }
            std::vector<float> recurrent_to_forget_weights_float32(
                    getNumberOfElements(recurrent_to_forget_weights_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(recurrent_to_forget_weights_),
                                    &recurrent_to_forget_weights_float32);
            std::vector<float> recurrent_to_cell_weights_float32(
                    getNumberOfElements(recurrent_to_cell_weights_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(recurrent_to_cell_weights_),
                                    &recurrent_to_cell_weights_float32);
            std::vector<float> recurrent_to_output_weights_float32(
                    getNumberOfElements(recurrent_to_output_weights_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(recurrent_to_output_weights_),
                                    &recurrent_to_output_weights_float32);
            const float* cell_to_input_weights_buffer = nullptr;
            std::vector<float> cell_to_input_weights_float32(
                    getNumberOfElements(cell_to_input_weights_->shape()));
            if (!IsNullInput(cell_to_input_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(cell_to_input_weights_),
                                        &cell_to_input_weights_float32);
                cell_to_input_weights_buffer = cell_to_input_weights_float32.data();
            }
            const float* cell_to_forget_weights_buffer = nullptr;
            std::vector<float> cell_to_forget_weights_float32(
                    getNumberOfElements(cell_to_forget_weights_->shape()));
            if (!IsNullInput(cell_to_forget_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(cell_to_forget_weights_),
                                        &cell_to_forget_weights_float32);
                cell_to_forget_weights_buffer = cell_to_forget_weights_float32.data();
            }
            const float* cell_to_output_weights_buffer = nullptr;
            std::vector<float> cell_to_output_weights_float32(
                    getNumberOfElements(cell_to_output_weights_->shape()));
            if (!IsNullInput(cell_to_output_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(cell_to_output_weights_),
                                        &cell_to_output_weights_float32);
                cell_to_output_weights_buffer = cell_to_output_weights_float32.data();
            }
            const float* input_gate_bias_buffer = nullptr;
            std::vector<float> input_gate_bias_float32(
                    getNumberOfElements(input_gate_bias_->shape()));
            if (!IsNullInput(input_gate_bias_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(input_gate_bias_),
                                        &input_gate_bias_float32);
                input_gate_bias_buffer = input_gate_bias_float32.data();
            }
            std::vector<float> forget_gate_bias_float32(
                    getNumberOfElements(forget_gate_bias_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(forget_gate_bias_),
                                    &forget_gate_bias_float32);
            std::vector<float> cell_bias_float32(getNumberOfElements(cell_bias_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(cell_bias_), &cell_bias_float32);
            std::vector<float> output_gate_bias_float32(
                    getNumberOfElements(output_gate_bias_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(output_gate_bias_),
                                    &output_gate_bias_float32);
            const float* projection_weights_buffer = nullptr;
            std::vector<float> projection_weights_float32(
                    getNumberOfElements(projection_weights_->shape()));
            if (!IsNullInput(projection_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(projection_weights_),
                                        &projection_weights_float32);
                projection_weights_buffer = projection_weights_float32.data();
            }
            const float* projection_bias_buffer = nullptr;
            std::vector<float> projection_bias_float32(
                    getNumberOfElements(projection_bias_->shape()));
            if (!IsNullInput(projection_bias_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(projection_bias_),
                                        &projection_bias_float32);
                projection_bias_buffer = projection_bias_float32.data();
            }
            std::vector<float> output_state_in_float32(
                    getNumberOfElements(output_state_in_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(output_state_in_),
                                    &output_state_in_float32);
            std::vector<float> cell_state_in_float32(getNumberOfElements(cell_state_in_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(cell_state_in_), &cell_state_in_float32);
            const float* input_layer_norm_weights_buffer = nullptr;
            std::vector<float> input_layer_norm_weights_float32(
                    getNumberOfElements(input_layer_norm_weights_->shape()));
            if (!IsNullInput(input_layer_norm_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(input_layer_norm_weights_),
                                        &input_layer_norm_weights_float32);
                input_layer_norm_weights_buffer = input_layer_norm_weights_float32.data();
            }
            const float* forget_layer_norm_weights_buffer = nullptr;
            std::vector<float> forget_layer_norm_weights_float32(
                    getNumberOfElements(forget_layer_norm_weights_->shape()));
            if (!IsNullInput(forget_layer_norm_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(forget_layer_norm_weights_),
                                        &forget_layer_norm_weights_float32);
                forget_layer_norm_weights_buffer = forget_layer_norm_weights_float32.data();
            }
            const float* cell_layer_norm_weights_buffer = nullptr;
            std::vector<float> cell_layer_norm_weights_float32(
                    getNumberOfElements(cell_layer_norm_weights_->shape()));
            if (!IsNullInput(cell_layer_norm_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(cell_layer_norm_weights_),
                                        &cell_layer_norm_weights_float32);
                cell_layer_norm_weights_buffer = cell_layer_norm_weights_float32.data();
            }
            const float* output_layer_norm_weights_buffer = nullptr;
            std::vector<float> output_layer_norm_weights_float32(
                    getNumberOfElements(output_layer_norm_weights_->shape()));
            if (!IsNullInput(output_layer_norm_weights_)) {
                convertFloat16ToFloat32(GetBuffer<_Float16>(output_layer_norm_weights_),
                                        &output_layer_norm_weights_float32);
                output_layer_norm_weights_buffer = output_layer_norm_weights_float32.data();
            }
            std::vector<float> output_state_out_float32(
                    getNumberOfElements(output_state_out_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(output_state_out_),
                                    &output_state_out_float32);
            std::vector<float> cell_state_out_float32(
                    getNumberOfElements(cell_state_out_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(cell_state_out_), &cell_state_out_float32);
            std::vector<float> output_float32(getNumberOfElements(output_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(output_), &output_float32);
            std::vector<float> scratch_buffer_float32(
                    getNumberOfElements(scratch_buffer_->shape()));
            convertFloat16ToFloat32(GetBuffer<_Float16>(scratch_buffer_), &scratch_buffer_float32);

            // clang-format off
            EvalFloat32(
                    input_float32.data(),
                    input_to_input_weights_buffer,
                    input_to_forget_weights_float32.data(),
                    input_to_cell_weights_float32.data(),
                    input_to_output_weights_float32.data(),
                    recurrent_to_input_weights_buffer,
                    recurrent_to_forget_weights_float32.data(),
                    recurrent_to_cell_weights_float32.data(),
                    recurrent_to_output_weights_float32.data(),
                    cell_to_input_weights_buffer,
                    cell_to_forget_weights_buffer,
                    cell_to_output_weights_buffer,
                    input_gate_bias_buffer,
                    forget_gate_bias_float32.data(),
                    cell_bias_float32.data(),
                    output_gate_bias_float32.data(),
                    projection_weights_buffer,
                    projection_bias_buffer,
                    output_state_in_float32.data(),
                    cell_state_in_float32.data(),
                    input_layer_norm_weights_buffer,
                    forget_layer_norm_weights_buffer,
                    cell_layer_norm_weights_buffer,
                    output_layer_norm_weights_buffer,
                    output_state_out_float32.data(),
                    cell_state_out_float32.data(),
                    output_float32.data(),
                    scratch_buffer_float32.data());
            // clang-format on

            convertFloat32ToFloat16(output_state_out_float32,
                                    GetBuffer<_Float16>(output_state_out_));
            convertFloat32ToFloat16(cell_state_out_float32, GetBuffer<_Float16>(cell_state_out_));
            convertFloat32ToFloat16(output_float32, GetBuffer<_Float16>(output_));
            convertFloat32ToFloat16(scratch_buffer_float32, GetBuffer<_Float16>(scratch_buffer_));
        } break;
        default: {
            LOG(ERROR) << "Unsupported data type: " << static_cast<int>(input_->type);
            return false;
        }
    }
    return true;
}

}  // namespace nn
}  // namespace android
