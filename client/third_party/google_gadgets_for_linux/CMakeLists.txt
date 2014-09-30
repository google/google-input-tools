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

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
INCLUDE(Configure)

ADD_SUBDIRECTORY(unittest)
ADD_SUBDIRECTORY(ggadget)
ADD_SUBDIRECTORY(hosts)
ADD_SUBDIRECTORY(third_party)
ADD_SUBDIRECTORY(extensions)
ADD_SUBDIRECTORY(gadgets)
ADD_SUBDIRECTORY(xdg)

ADD_DIR_TO_ZIP(resources output/bin/resources.gg)

INSTALL( FILES "${CMAKE_BINARY_DIR}/output/bin/resources.gg"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/google-gadgets"
)
