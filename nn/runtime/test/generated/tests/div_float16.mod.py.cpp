// clang-format off
// Generated file (from: div_float16.mod.py). Do not edit
#include "../../TestGenerated.h"

namespace div_float16 {
// Generated div_float16 test
#include "generated/examples/div_float16.example.cpp"
// Generated model constructor
#include "generated/models/div_float16.model.cpp"
} // namespace div_float16

TEST_F(GeneratedTests, div_float16) {
    execute(div_float16::CreateModel,
            div_float16::is_ignored,
            div_float16::examples);
}

