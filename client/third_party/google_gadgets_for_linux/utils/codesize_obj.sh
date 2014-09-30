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

# Usage: cd to the root of the build directory and run this script.

CODESIGHS=third_party/codesighs
if [ ! -d $CODESIGHS ]; then
  # In case of in cmake's output directory.
  CODESIGHS=../third_party/codesighs
fi
if [ ! -d $CODESIGHS ]; then
  echo Error: no third_party/codesighs directory found.
fi

ALLFILES=`find . -type f -name "*.o" | grep -v /tests/ | grep -v /codesighs/`
TSVFILE=/tmp/tsv.$$
LASTTSVFILE=last_obj.tsv
NMRESULTS=/tmp/nmresults.$$

nm --format=bsd --size-sort --print-file-name --demangle $ALLFILES >$NMRESULTS 2>/dev/null
$CODESIGHS/nm2tsv --input $NMRESULTS | sort -r >$TSVFILE
$CODESIGHS/codesighs --modules --input $TSVFILE
if [ -f $LASTTSVFILE ]; then
  echo
  echo "================= DIFF ================="
  DIFFFILE=/tmp/tsvdiff.$$
  diff $LASTTSVFILE $TSVFILE >$DIFFFILE
  $CODESIGHS/maptsvdifftool --input $DIFFFILE
fi
cp $TSVFILE $LASTTSVFILE

