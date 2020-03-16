#
# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Model: given n, produces [fib(n), fib(n + 1)].
#
# fib = [1, 1]
# i = 1
# while i < n:
#     fib = matmul(fib, [0 1;
#                        1 1])
#     i = i + 1

FibType = ["TENSOR_FLOAT32", [1, 2]]
CounterType = ["TENSOR_INT32", [1]]
BoolType = ["TENSOR_BOOL8", [1]]

def MakeConditionModel():
  fib = Input("fib", FibType)
  i = Input("i", CounterType)
  n = Input("n", CounterType)
  out = Output("out", BoolType)
  model = Model()
  model.IdentifyInputs(fib, i, n)
  model.IdentifyOutputs(out)
  model.Operation("LESS", i, n).To(out)
  return model

def MakeBodyModel():
  fib = Input("fib", FibType)
  i = Input("i", CounterType)
  n = Input("n", CounterType)
  fib_out = Output("fib_out", FibType)
  i_out = Output("i_out", CounterType)
  matrix = Parameter("matrix", ["TENSOR_FLOAT32", [2, 2]], [0, 1, 1, 1])
  zero_bias = Parameter("zero_bias", ["TENSOR_FLOAT32", [2, 1]], [0, 0])
  model = Model()
  model.IdentifyInputs(fib, i, n)
  model.IdentifyOutputs(fib_out, i_out)
  model.Operation("ADD", i, [1], 0).To(i_out)
  model.Operation("FULLY_CONNECTED", fib, matrix, zero_bias, 0).To(fib_out)
  return model

n = Input("n", CounterType)
fib_out = Output("fib_out", FibType)
cond = MakeConditionModel()
body = MakeBodyModel()
fib_init = Parameter("fib_init", FibType, [1, 1])
i_init = [1]
model = Model().Operation("WHILE", cond, body, fib_init, i_init, n).To(fib_out)

# Fibonacci numbers: 1 1 2 3 5 8
Example({n: [1], fib_out: [1, 1]}, name="1", model=model)
Example({n: [2], fib_out: [1, 2]}, name="2", model=model)
Example({n: [3], fib_out: [2, 3]}, name="3", model=model)
Example({n: [4], fib_out: [3, 5]}, name="4", model=model)
Example({n: [5], fib_out: [5, 8]}, name="5", model=model)
