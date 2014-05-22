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

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit -Wno-pointer-sign -Wno-conversion")

ADD_DEFINITIONS(
  -DSYS_UNIX
  -DHAVE_STDLIB
  -DUNIQUE_HISTORY
  -DUSE_DIRENT
  -DHIDE
  -DHAVE_TCGETATTR
  -DANSI_ARROWS
)

SET(SRCS
  editline.c
  sysunix.c
)

ADD_LIBRARY(editline STATIC ${SRCS})
