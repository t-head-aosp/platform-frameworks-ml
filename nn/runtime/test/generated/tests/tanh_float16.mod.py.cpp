// clang-format off
// Generated file (from: tanh_float16.mod.py). Do not edit
#include "../../TestGenerated.h"

namespace tanh_float16 {
// Generated tanh_float16 test
#include "generated/examples/tanh_float16.example.cpp"
// Generated model constructor
#include "generated/models/tanh_float16.model.cpp"
} // namespace tanh_float16

TEST_F(GeneratedTests, tanh_float16) {
    execute(tanh_float16::CreateModel,
            tanh_float16::is_ignored,
            tanh_float16::examples);
}

