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

SET(SRCS
  desktop_entry.cc
  icon_theme.cc
  utilities.cc
)

ADD_SUBDIRECTORY(tests)

ADD_DEFINITIONS(-DXDG_PREFIX=_ggl_xdg)
ADD_LIBRARY(ggadget-xdg${GGL_EPOCH} SHARED ${SRCS})

IF(GGL_ENABLE_XDGMIME)
  ADD_DEFINITIONS(-DGGL_ENABLE_XDGMIME)
  TARGET_LINK_LIBRARIES(ggadget-xdg${GGL_EPOCH}
    ggadget${GGL_EPOCH}
    xdgmime
  )
ELSE(GGL_ENABLE_XDGMIME)
  TARGET_LINK_LIBRARIES(ggadget-xdg${GGL_EPOCH}
    ggadget${GGL_EPOCH}
  )
ENDIF(GGL_ENABLE_XDGMIME)

OUTPUT_LIBRARY(ggadget-xdg${GGL_EPOCH})

INSTALL(FILES
  desktop_entry.h
  utilities.h
  DESTINATION ${GGL_INCLUDE_DIR}/ggadget/xdg COMPONENT Devel)
INSTALL_PKG_CONFIG(libggadget-xdg)
