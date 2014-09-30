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

IF(GGL_BUILD_LINUX_SYSTEM_FRAMEWORK)

APPLY_CONFIG(X11)
APPLY_CONFIG(DBUS)
APPLY_CONFIG(NETWORK_MANAGER)

ADD_SUBDIRECTORY(tests)

SET(SRCS
  file_system.cc
  runtime.cc
  memory.cc
  perfmon.cc
  process.cc
)
IF(GGL_BUILD_LIBGGADGET_DBUS)
LIST(APPEND SRCS
  machine.cc
  power.cc
  user.cc
)
ENDIF(GGL_BUILD_LIBGGADGET_DBUS)

IF(GGL_HAVE_NETWORK_MANAGER)
LIST(APPEND SRCS
  network.cc
  wireless.cc
)
ENDIF(GGL_HAVE_NETWORK_MANAGER)

ADD_LIBRARY(ggadget-linux ${SRCS})
TARGET_LINK_LIBRARIES(ggadget-linux
  ggadget${GGL_EPOCH}
  ggadget-xdg${GGL_EPOCH}
  ${X11_LIBRARIES}
)

IF(GGL_BUILD_LIBGGADGET_DBUS)
TARGET_LINK_LIBRARIES(ggadget-linux
  ggadget-dbus${GGL_EPOCH}
)
ENDIF(GGL_BUILD_LIBGGADGET_DBUS)

ADD_MODULE(linux-system-framework linux_system_framework.cc)
TARGET_LINK_LIBRARIES(linux-system-framework
  ggadget-linux
)

OUTPUT_MODULE(linux-system-framework)

ENDIF(GGL_BUILD_LINUX_SYSTEM_FRAMEWORK)
