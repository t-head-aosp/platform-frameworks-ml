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

#ifndef ANDROID_FRAMEWORKS_ML_NN_RUNTIME_EXECUTION_BUILDER_H
#define ANDROID_FRAMEWORKS_ML_NN_RUNTIME_EXECUTION_BUILDER_H

#include <atomic>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Callbacks.h"
#include "ControlFlow.h"
#include "CpuExecutor.h"
#include "HalInterfaces.h"
#include "Memory.h"
#include "ModelArgumentInfo.h"
#include "ModelBuilder.h"
#include "NeuralNetworks.h"

namespace android {
namespace nn {

class BurstBuilder;
class CompilationBuilder;
class Device;
class DynamicTemporaries;
class ExecutionBurstController;
class ExecutionPlan;
class ExecutionStep;
class ModelBuilder;
class RuntimeMemory;
class RuntimePreparedModel;
class StepExecutor;

class ExecutionBuilder {
    friend class StepExecutor;

   public:
    ExecutionBuilder(const CompilationBuilder* compilation);

    int setInput(uint32_t index, const ANeuralNetworksOperandType* type, const void* buffer,
                 size_t length);
    int setInputFromMemory(uint32_t index, const ANeuralNetworksOperandType* type,
                           const RuntimeMemory* memory, size_t offset, size_t length);
    int setOutput(uint32_t index, const ANeuralNetworksOperandType* type, void* buffer,
                  size_t length);
    int setOutputFromMemory(uint32_t index, const ANeuralNetworksOperandType* type,
                            const RuntimeMemory* memory, size_t offset, size_t length);

    int setMeasureTiming(bool measure);

    int getDuration(int32_t durationCode, uint64_t* duration) const;

    int setTimeoutDuration(uint64_t duration);

    std::optional<uint64_t> getTimeoutDuration() const;

    int setLoopTimeout(uint64_t duration);

    uint64_t getLoopTimeoutDuration() const { return mLoopTimeoutDuration; }

    int computeFenced(const std::vector<int>& wait_for, uint64_t timeoutDurationAfterFence,
                      int* sync_fence);

    int computeAsynchronously(sp<ExecutionCallback>* synchronizationCallback) {
        CHECK(synchronizationCallback != nullptr);
        return compute(synchronizationCallback);
    }
    int computeSynchronously() { return compute(nullptr); }
    int burstCompute(BurstBuilder* burst) { return compute(nullptr, burst); }

    // Initialize output dimensional information from ModelArgumentInfo.
    std::vector<OutputShape> getInitialOutputShapes() const;

    int getOutputOperandDimensions(uint32_t index, uint32_t* dimensions);
    int getOutputOperandRank(uint32_t index, uint32_t* rank);

    // Handshake with lower-level execution support
    bool measureTiming() const { return mMeasureTiming; }
    void reportTimingWithoutFencedExecutionCallback(Timing timing) {
        mTimingWithoutFencedExecutionCallback = timing;
    }

    const CompilationBuilder* getCompilation() const { return mCompilation; }
    const ModelBuilder* getModel() const { return mModel; }
    const ModelBuilder* getSourceModel(uint32_t index) const;
    const Operand& getSourceOperand(const std::pair<uint32_t, uint32_t>& sourceOperandIndex) const {
        return getSourceModel(sourceOperandIndex.first)->getOperand(sourceOperandIndex.second);
    }

    ErrorStatus finishWithoutSyncFence(ErrorStatus error,
                                       const std::vector<OutputShape>& outputShapes);

    // Retrieve a reference to the IFencedExecutionCallback callback.
    const sp<V1_3::IFencedExecutionCallback>& getFencedExecutionCallback() {
        return mFencedExecutionCallback;
    }

    bool inFlight() const { return mStarted && !isFinished(); }

    const ModelArgumentInfo& getInputInfo(uint32_t index) const { return mInputs[index]; }
    const ModelArgumentInfo& getOutputInfo(uint32_t index) const { return mOutputs[index]; }

    std::optional<RunTimePoolInfo> getRunTimePoolInfo(uint32_t poolIndex) const {
        return mMemories[poolIndex]->getRunTimePoolInfo();
    }

   private:
    // If a callback is provided, then this is asynchronous. If a callback is
    // not provided (i.e., is nullptr), then this is synchronous.
    //
    // If burst is provided, then the burst path will be used. If a burst is not
    // provided (i.e., is nullptr), then a synchronous execution will occur.
    //
    // Providing both synchronizationCallback and burstBuilder is an error.
    int compute(sp<ExecutionCallback>* synchronizationCallback,
                BurstBuilder* burstBuilder = nullptr);

    const CompilationBuilder* mCompilation;

    // Update output dimensional information from OutputShape to ModelArgumentInfo.
    bool updateOutputShapes(ErrorStatus status, const std::vector<OutputShape>& outputShapes);

    bool updateMemories();

    bool hasSyncFence() const { return mSyncFenceFd > 0; }

    const ModelBuilder* mModel;
    const ExecutionPlan* mPlan;

    // This is a DeviceManager::kPartitioning* value captured from
    // CompilationBuilder when the ExecutionBuilder is constructed.
    uint32_t mPartitioning;

    // The information we'll send to the driver about the inputs and outputs.
    // Note that we build this in two steps:
    // 1. As the arguments are specified, set the corresponding mInputs or mOutputs element.
    //    If set from a pointer, don't set the location in the Request::Argument but store it
    //    instead in mInputBuffers or mOutputBuffers.
    // 2. Once we have all the inputs and outputs, if needed, allocate shared memory for
    //    the m*Buffers entries.  Copy the input values into the shared memory.
    // We do this to avoid creating a lot of shared memory objects if we have a lot of
    // parameters specified via pointers.  We also avoid copying in the case where
    // some of the nodes will interpreted on the CPU anyway.
    std::vector<ModelArgumentInfo> mInputs;
    std::vector<ModelArgumentInfo> mOutputs;
    MemoryTracker mMemories;

    // Do we ask the driver to measure timing?
    bool mMeasureTiming = false;

    // Timing reported from the driver.  This field is only used if
    // mFencedExecutionCallback is nullptr.
    Timing mTimingWithoutFencedExecutionCallback = {};

    // Amount of time to complete or abort the execution.
    std::optional<uint64_t> mTimeoutDuration;

    // Amount of time to complete or abort a loop.
    uint64_t mLoopTimeoutDuration = operation_while::kTimeoutNsDefault;

    // Properties cannot be set once the execution has started.
    std::atomic_bool mStarted = false;

    // Timing and output shapes can only be queried after the execution is
    // finished.  This field only becomes true if !hasSyncFence().
    // See isFinished().
    std::atomic_bool mFinishedWithoutSyncFence = false;

    bool isFinished() const;

    // With what error status has execution completed?  This field only takes on
    // a meaningful value if !hasSyncFence().
    // See completedWith().
    enum class Completion { NO_ERROR, OUTPUT_INSUFFICIENT_SIZE, OTHER_ERROR };
    Completion mCompletionWithoutSyncFence = Completion::OTHER_ERROR;

    // With what error status has execution completed?  Must only be called if
    // isFinished().
    Completion completedWith() const;

    // The sync fence fd that is created in the computeFenced call, if any.
    // (Sometimes no sync fence fd will be created.)
    int mSyncFenceFd = -1;

    // The callback used to query execution related info in the case of fenced
    // execution; otherwise, nullptr.  If the execution plan has multiple steps,
    // this is the callback associated with the last step.  If the last step
    // doesn't support fenced execution (e.g., the driver is too old), or if the
    // launch of execution on the driver fails, then this callback will be
    // nullptr.
    sp<V1_3::IFencedExecutionCallback> mFencedExecutionCallback;
};

// class StepExecutor is used to execute a single "step" in a
// potentially multiple step execution process.  The graph associated
// with that step is executed in its entirety on a single device (or
// on the CPU).
class StepExecutor {
   public:
    // executionBuilder
    //     Describes the full (possibly multiple-"step") execution.
    // model
    //     The model to be executed by the executor.  Possibly a single
    //     "step" model of a multiple-"step" executionBuilder.
    // driver, preparedModel
    //     The device on which to execute the "step", and the prepared
    //     model to execute on that device.  (Both are nullptr in the
    //     case of CPU.)
    // step
    //     Contains the output index mapping from the excerpted "step" model to
    //     main model if the execution has multiple "steps". Must be nullptr
    //     otherwise.
    //     (step == nullptr) == (dynamicTemporaries == nullptr)
    // dynamicTemporaries
    //     If the execution has multiple "steps", describes the temporaries
    //     of source models that do not have fully specified types and are outputs
    //     of "step" models. Must be nullptr otherwise.
    //     (step == nullptr) == (dynamicTemporaries == nullptr)
    StepExecutor(ExecutionBuilder* executionBuilder, const ModelBuilder* model,
                 std::shared_ptr<Device> device,
                 std::shared_ptr<RuntimePreparedModel> preparedModel,
                 const ExecutionStep* step = nullptr,
                 DynamicTemporaries* dynamicTemporaries = nullptr);

    // Map inputs and outputs from ExecutionBuilder to StepExecutor,
    // in the case where we have a single-"step" execution (i.e., the executor
    // is executing the entire model from the ExecutionBuilder).
    void mapInputsAndOutputsTrivially();

    // Update output shapes with shapes returned from execution.
    struct UpdateOutputShapes {
        // These fields are meaningless unless updateOutputShapes() returns true
        bool updatedDynamicTemporary;  // did shape (dimensions, size) information change for at
                                       // least one dynamic temporary?
        bool mainOutputInsufficient;  // is at least one main model output written by this execution
                                      // marked !isSufficient?
        bool zeroSizedInput;  // is at least one output of this execution step a zero-sized tensor
                              // that needs to be read by some other step of the same execution?
    };
    bool updateOutputShapes(int executionResultCode, const std::vector<OutputShape>& from,
                            std::vector<OutputShape>* to, UpdateOutputShapes* update);

    // Map inputs and outputs from ExecutionBuilder to StepExecutor,
    // one at a time.  Note that these are input/output indexes, not
    // operand indexes.
    //
    // For mapOutputToInput(), outputDimensions may be nullptr if the input
    // operand has fully specified dimensions.
    void mapInput(uint32_t builderIndex, uint32_t executorIndex) {
        mapInputOrOutput(mExecutionBuilder->mInputs[builderIndex], &mInputs[executorIndex]);
    }
    void mapOutput(uint32_t builderIndex, uint32_t executorIndex) {
        mapInputOrOutput(mExecutionBuilder->mOutputs[builderIndex], &mOutputs[executorIndex]);
    }
    void mapOutputToInput(uint32_t builderIndex, uint32_t executorIndex,
                          const Dimensions* outputDimensions) {
        mapInputOrOutput(mExecutionBuilder->mOutputs[builderIndex], &mInputs[executorIndex],
                         outputDimensions);
    }

    // If no length is provided, the input or output is assumed to have the length
    // of the operand.  dimensions must either have zero rank or must be
    // consistent with and at least as well specified as operand dimensions
    // (i.e., either rank must match, or operand rank must be zero; and for each
    // individual dimension, either dimension must match, or operand dimension
    // must be zero).
    int setInputFromMemory(uint32_t inputIndex, const RuntimeMemory* memory, uint32_t offset,
                           const Dimensions& dimensions = {},
                           std::optional<uint32_t> length = std::nullopt) {
        return setInputOrOutputFromMemory(mModel->getInputOperand(inputIndex), memory, offset,
                                          dimensions, length, &mInputs.at(inputIndex));
    }
    int setOutputFromMemory(uint32_t outputIndex, const RuntimeMemory* memory, uint32_t offset,
                            const Dimensions& dimensions = {},
                            std::optional<uint32_t> length = std::nullopt) {
        return setInputOrOutputFromMemory(mModel->getOutputOperand(outputIndex), memory, offset,
                                          dimensions, length, &mOutputs.at(outputIndex));
    }

    // Executes using the (driver, preparedModel) specified at construction time.
    std::tuple<int, std::vector<OutputShape>, Timing> compute(
            const std::optional<Deadline>& deadline,
            const std::shared_ptr<ExecutionBurstController>& burstController = nullptr);

    // Re-compiles and executes using the CPU, regardless of the (driver,
    // preparedModel) specified at construction time.
    std::tuple<int, std::vector<OutputShape>, Timing> computeOnCpuFallback();

    bool isCpu() const;

    // Perform fenced execution and return error_code, sync_fence_fd and a
    // callback.
    std::tuple<int, int, sp<V1_3::IFencedExecutionCallback>> computeFenced(
            const std::vector<int>& wait_for, uint64_t timeoutDurationAfterFence,
            const std::optional<Deadline>& deadline);

    // Do the dynamic temporaries defined by this step have valid allocations?
    // (true if there are no dynamic temporaries defined by this step.)
    bool areDynamicTemporariesAllocated() const;

   private:
    // builderDimensions may be nullptr if executorInputOrOutput has fully
    // specified dimensions.
    void mapInputOrOutput(const ModelArgumentInfo& builderInputOrOutput,
                          ModelArgumentInfo* executorInputOrOutput,
                          const Dimensions* builderDimensions = nullptr);

    // If no length is provided, the input or output is assumed to have the length
    // of the corresponding operand.  dimensions must either have zero rank or
    // must be consistent with and at least as well specified as operand
    // dimensions (i.e., either rank must match, or operand rank must be zero;
    // and for each individual dimension, either dimension must match, or
    // operand dimension must be zero).
    int setInputOrOutputFromMemory(const Operand& inputOrOutputOperand, const RuntimeMemory* memory,
                                   uint32_t offset, const Dimensions& dimensions,
                                   std::optional<uint32_t> length,
                                   ModelArgumentInfo* inputOrOutputInfo);

    std::tuple<int, std::vector<OutputShape>, Timing> computeWithMemories(
            const std::optional<Deadline>& deadline,
            const std::vector<const RuntimeMemory*>& memories,
            const std::shared_ptr<ExecutionBurstController>& burstController = nullptr);

    // describes the full (possibly multiple-"step") execution
    ExecutionBuilder* mExecutionBuilder;

    // describes the single execution step
    const ExecutionStep* mExecutionStep;

    // describes the dynamic temporaries
    DynamicTemporaries* mDynamicTemporaries;

    // model to be executed on the executor, in both original and
    // compiled forms; and device on which to execute it
    const ModelBuilder* mModel;
    std::shared_ptr<Device> mDevice;
    std::shared_ptr<RuntimePreparedModel> mPreparedModel;

    // The information we'll send to the driver about the inputs and outputs.
    // Note that we build this in two steps:
    // 1. As the arguments are specified, set the corresponding mInputs or mOutputs element.
    //    If set from a pointer, don't set the location in the Request::Argument but store it
    //    instead in mInputBuffers or mOutputBuffers.
    // 2. Once we have all the inputs and outputs, if needed, allocate shared memory for
    //    the m*Buffers entries.  Copy the input values into the shared memory.
    // We do this to avoid creating a lot of shared memory objects if we have a lot of
    // parameters specified via pointers.  We also avoid copying in the case where
    // some of the nodes will interpreted on the CPU anyway.
    std::vector<ModelArgumentInfo> mInputs;
    std::vector<ModelArgumentInfo> mOutputs;
    MemoryTracker mMemories;
};

std::string toString(StepExecutor::UpdateOutputShapes updateOutputShapes);

}  // namespace nn
}  // namespace android

#endif  // ANDROID_FRAMEWORKS_ML_NN_RUNTIME_EXECUTION_BUILDER_H
