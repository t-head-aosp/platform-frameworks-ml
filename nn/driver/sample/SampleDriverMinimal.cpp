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

#define LOG_TAG "SampleDriverMinimal"

#include <android-base/logging.h>

#include <thread>
#include <vector>

#include "HalInterfaces.h"
#include "NeuralNetworksOEM.h"
#include "SampleDriverPartial.h"
#include "Utils.h"
#include "ValidateHal.h"

namespace android {
namespace nn {
namespace sample_driver {

class SampleDriverMinimal : public SampleDriverPartial {
   public:
    SampleDriverMinimal() : SampleDriverPartial("nnapi-sample_minimal") {}
    hardware::Return<void> getCapabilities_1_3(getCapabilities_1_3_cb cb) override;

   private:
    std::vector<bool> getSupportedOperationsImpl(const V1_3::Model& model) const override;
};

hardware::Return<void> SampleDriverMinimal::getCapabilities_1_3(getCapabilities_1_3_cb cb) {
    android::nn::initVLogMask();
    VLOG(DRIVER) << "getCapabilities()";

    V1_3::Capabilities capabilities = {
            .relaxedFloat32toFloat16PerformanceScalar = {.execTime = 0.4f, .powerUsage = 0.5f},
            .relaxedFloat32toFloat16PerformanceTensor = {.execTime = 0.4f, .powerUsage = 0.5f},
            .operandPerformance = nonExtensionOperandPerformance<HalVersion::V1_3>({1.0f, 1.0f}),
            .ifPerformance = {.execTime = 1.0f, .powerUsage = 1.0f},
            .whilePerformance = {.execTime = 1.0f, .powerUsage = 1.0f}};
    update(&capabilities.operandPerformance, V1_3::OperandType::TENSOR_FLOAT32,
           {.execTime = 0.4f, .powerUsage = 0.5f});
    update(&capabilities.operandPerformance, V1_3::OperandType::FLOAT32,
           {.execTime = 0.4f, .powerUsage = 0.5f});

    cb(V1_3::ErrorStatus::NONE, capabilities);
    return hardware::Void();
}

std::vector<bool> SampleDriverMinimal::getSupportedOperationsImpl(const V1_3::Model& model) const {
    const size_t count = model.main.operations.size();
    std::vector<bool> supported(count);
    // Simulate supporting just a few ops
    for (size_t i = 0; i < count; i++) {
        supported[i] = false;
        const V1_3::Operation& operation = model.main.operations[i];
        switch (operation.type) {
            case V1_3::OperationType::ADD:
            case V1_3::OperationType::CONCATENATION:
            case V1_3::OperationType::CONV_2D: {
                const V1_3::Operand& firstOperand = model.main.operands[operation.inputs[0]];
                if (firstOperand.type == V1_3::OperandType::TENSOR_FLOAT32) {
                    supported[i] = true;
                }
                break;
            }
            default:
                break;
        }
    }
    return supported;
}

}  // namespace sample_driver
}  // namespace nn
}  // namespace android

using android::sp;
using android::nn::sample_driver::SampleDriverMinimal;

int main() {
    sp<SampleDriverMinimal> driver(new SampleDriverMinimal());
    return driver->run();
}
