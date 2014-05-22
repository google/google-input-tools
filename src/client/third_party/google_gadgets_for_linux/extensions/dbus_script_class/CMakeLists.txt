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

IF(GGL_BUILD_LIBGGADGET_DBUS)

APPLY_CONFIG(DBUS)

SET(SRCS
  dbus_script_class.cc
  scriptable_dbus_object.cc
)
ADD_MODULE(dbus-script-class ${SRCS})
TARGET_LINK_LIBRARIES(dbus-script-class ggadget-dbus${GGL_EPOCH} ${DBUS_LIBRARIES} ggadget${GGL_EPOCH})
OUTPUT_MODULE(dbus-script-class)

ENDIF(GGL_BUILD_LIBGGADGET_DBUS)
