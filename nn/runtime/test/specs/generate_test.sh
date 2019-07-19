#!/bin/bash

#
# Copyright (C) 2017 The Android Open Source Project
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

set -Eeuo pipefail

NNAPI_VERSIONS="V1_0 V1_1 V1_2"

# Process one test spec, and optionally provide the log file argument
# for the slicing tool. The first argument is the test spec file; the
# second optional argument specifies the log file this test should dump
# results into. Only used by the test slicing tool to collect reference
# outputs from the CPU. Also, it outputs the right #includes in the
# test harness so the test would be invoked by TestGenerated.cpp
#
# This function shouldn't be directly called from other scripts. Use
# generate_wrapper below for generating models and examples and updating the
# test framework in one shot.

export NNAPI_BASE=$ANDROID_BUILD_TOP/frameworks/ml/nn
: ${TEST_DIR:=frameworks/ml/nn/runtime/test}
: ${FORCE:=""}

function generate_one_testcase {
  # Generate one testcase
  local LOGFILE=$2
  if [ -n "$2" ]; then
    local LOGFILE="-l $2"
  fi
  local BASENAME=`basename -s .mod.py $1`
  local MODEL="-m $ANDROID_BUILD_TOP/$TEST_DIR/generated/models/$BASENAME.model.cpp"
  local EXAMPLE="-e $ANDROID_BUILD_TOP/$TEST_DIR/generated/examples/$BASENAME.example.cpp"
  local TEST="-t $ANDROID_BUILD_TOP/$TEST_DIR/generated/tests/$(basename $1).cpp"

  $NNAPI_BASE/tools/test_generator/cts_generator.py $FORCE ./`basename $1` \
    $MODEL $EXAMPLE $TEST $LOGFILE
  ret=$?
  return $ret
}

# Driver for generate_one_testcase. Append the output of generate_one_testcase
# (which are C++ snippets that invokes the test harness) to the
# all_generated_tests.cpp
# Optionally, the "LOG" file ($2), only used by the slicing tool, would be
# passed to generate_one_testcase.
#
# This function should be called to process one test spec from other scripts.
function generate_wrapper {
  local LOGFILE=""
  if [ $1 = "log" ]; then
    local LOGFILE=$2
    shift
    shift
  fi
  cd $ANDROID_BUILD_TOP/$TEST_DIR/specs
  FOUND=0

  for ver in $NNAPI_VERSIONS;
  do
    VER_DIR=$ANDROID_BUILD_TOP/$TEST_DIR/specs/$ver
    [ ! -d $VER_DIR ] && continue
    pushd $VER_DIR > /dev/null
    for f in $@;
    do
      if [ -f $(basename $f) ]; then
        generate_one_testcase $f "$LOGFILE"
        if [ $? -ne 0 ]; then
          echo "Failed processing $f"
          return $?
        fi
        FOUND=1
      fi
    done
    popd > /dev/null
  done
  if [[ $FOUND -eq 0 ]]; then
    echo did not find any files for $@
    exit 1
  fi
  return $?
}

# Process all test spec directory specified by NNAPI_VERSIONS.
function generate_spec_dirs {
  cd $ANDROID_BUILD_TOP/$TEST_DIR/specs

  for ver in $NNAPI_VERSIONS;
  do
    VER_DIR=$ANDROID_BUILD_TOP/$TEST_DIR/specs/$ver
    [ ! -d $VER_DIR ] && continue
    pushd $VER_DIR > /dev/null

    TARGET_MODEL_DIR="-m $ANDROID_BUILD_TOP/$TEST_DIR/generated/models"
    TARGET_EXAMPLE_DIR="-e $ANDROID_BUILD_TOP/$TEST_DIR/generated/examples"
    TARGET_TEST_DIR="-t $ANDROID_BUILD_TOP/$TEST_DIR/generated/tests"

    $NNAPI_BASE/tools/test_generator/cts_generator.py $FORCE $VER_DIR \
        $TARGET_MODEL_DIR $TARGET_EXAMPLE_DIR $TARGET_TEST_DIR
    if [ $? -ne 0 ]; then
        echo "Failed processing $VER_DIR"
        return $?
    fi
    popd > /dev/null
  done
  return $?
}

# Only run the following when not sourced by another script
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  set -eu
  if [ $# -gt 0 ]; then
    if  [ $1 = "-f" ] || [ $1 = "--force" ]; then
      FORCE="-f"
      shift
    fi
  fi
  if [ $# -eq 0 ]; then
    generate_spec_dirs $FORCE
  else
    FILES="$@"
    generate_wrapper $FILES
  fi
  if [ $? -ne 0 ]; then
    exit $?
  fi
fi # [[ "${BASH_SOURCE[0]}" == "${0}" ]]

