// DO NOT EDIT;
// Generated by ml/nn/runtime/test/specs/generate_test.sh
#include "../../TestGenerated.h"

namespace reshape {
std::vector<MixedTypedExample> examples = {
// Generated reshape test
#include "generated/examples/reshape.example.cpp"
};
// Generated model constructor
#include "generated/models/reshape.model.cpp"
} // namespace reshape
TEST_F(GeneratedTests, reshape) {
    execute(reshape::CreateModel,
            reshape::is_ignored,
            reshape::examples);
}
