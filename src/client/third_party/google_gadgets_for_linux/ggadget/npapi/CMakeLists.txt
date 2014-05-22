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

IF(GGL_BUILD_LIBGGADGET_NPAPI)

APPLY_CONFIG(X11)
APPLY_CONFIG(XT)

SET(SRCS
  npapi_plugin.cc
  npapi_plugin_script.cc
  npapi_utils.cc
)

ADD_SUBDIRECTORY(tests)

ADD_LIBRARY(ggadget-npapi${GGL_EPOCH} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(ggadget-npapi${GGL_EPOCH}
  ggadget${GGL_EPOCH}
  dl
  ${X11_LIBRARIES}
  ${XT_LIBRARIES}
)
OUTPUT_LIBRARY(ggadget-npapi${GGL_EPOCH})

INSTALL(FILES
  npapi_plugin.h
  DESTINATION ${GGL_INCLUDE_DIR}/ggadget/npapi COMPONENT Devel)

INSTALL_PKG_CONFIG(libggadget-npapi)

ENDIF(GGL_BUILD_LIBGGADGET_NPAPI)
