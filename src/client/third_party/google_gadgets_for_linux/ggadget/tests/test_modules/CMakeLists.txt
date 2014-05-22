#
# Copyright 2008 Google Inc.
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

MACRO(OUTPUT_TEST_MODULE _target_name)
  GET_TARGET_PROPERTY(target_location ${_target_name} LOCATION)
  GET_FILENAME_COMPONENT(target_dir ${target_location} PATH)
  COPY_TARGET(${_target_name} ${target_dir}/..)
ENDMACRO(OUTPUT_TEST_MODULE _target_name _output_name)

SET(SRCS
  ../../test_module.cc
)

ADD_SUBDIRECTORY(foo_module)
ADD_SUBDIRECTORY(bar_module)
ADD_SUBDIRECTORY(fake_module)
ADD_SUBDIRECTORY(tux_module)
