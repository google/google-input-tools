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

progname="$0"

here=`/bin/pwd`

cd `dirname "$progname"`
progbase=`basename "$progname"`
progdir=`/bin/pwd`

while [ -h "$progname" ]; do
  progname=`/bin/ls -l "$progbase" | sed -e 's/^.* -> //' `
  cd `dirname "$progname"`
  progbase=`basename "$progname"`
  progdir=`/bin/pwd`
done

if [ -x "$progbase".bin ]; then
  cd "$here"
  export PREFIX=`dirname "$progdir"`
  export LD_LIBRARY_PATH="$PREFIX/lib:$LD_LIBRARY_PATH"
  export DYLD_LIBRARY_PATH="$PREFIX/lib:$DYLD_LIBRARY_PATH"
  export GGL_MODULE_PATH="$PREFIX/modules"
  "$progdir/$progbase.bin" "$@"
  exit $?
else
  cd "$here"
  echo "Cannot find $progbase runtime directory. Exiting."
  exit 2
fi
