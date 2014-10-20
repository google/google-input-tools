#!/bin/bash
# Copyright 2014 The Cloud Input Tools Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS-IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

src=/usr/local/google/home/wuyingbing/opensource/google-input-tools/src/cloud
closure=/usr/local/google/home/wuyingbing/opensource/closure
$closure/closure/bin/calcdeps.py -i $src/keyboard/standalone.js \
  -p $closure -o compiled -p $src \
  -c $closure/compiler.jar \
  -f "--compilation_level=ADVANCED_OPTIMIZATIONS" \
  > kbd.js
