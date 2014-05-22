#!/bin/sh
#
# Copyright 2007 Google Inc.
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

set -e
here=`pwd`
cd `dirname $0`/..
srcdir=`pwd`
mkdir -p build/debug_kw
cd build/debug_kw
cmake -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_CXX_COMPILER=$srcdir/utils/kwwrap-g++.sh" "$srcdir"
make
kwinject -t kwinject.trace -o kwinject.out
kwbuildproject kwinject.out --incremental --tables-directory tables_dir "$@"
if kwadmin create-project ggl --language cxx; then echo created ggl; fi
kwadmin load ggl tables_dir
firefox http://localhost:8070
