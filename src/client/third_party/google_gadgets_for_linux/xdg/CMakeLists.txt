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

SET(ICONS
  32x32
  48x48
  64x64
  128x128
  192x192
  )

FOREACH(icon ${ICONS})
  INSTALL(FILES "${CMAKE_SOURCE_DIR}/xdg/${icon}.png"
    DESTINATION
    "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/${icon}/apps"
    RENAME google-gadgets.png)
  INSTALL(FILES "${CMAKE_SOURCE_DIR}/xdg/${icon}.png"
    DESTINATION
    "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/${icon}/mimetypes"
    RENAME application-x-google-gadget.png)
ENDFOREACH(icon ${ICONS})

INSTALL(FILES "${CMAKE_SOURCE_DIR}/xdg/128x128.png"
  DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pixmaps"
  RENAME google-gadgets.png
)

INSTALL(FILES "${CMAKE_SOURCE_DIR}/xdg/00-google-gadgets.xml"
  DESTINATION
  "${CMAKE_INSTALL_PREFIX}/share/mime/packages"
)

ADD_EXECUTABLE(intl_desktop_file intl_desktop_file.cc)
TARGET_LINK_LIBRARIES(intl_desktop_file ggadget${GGL_EPOCH})
OUTPUT_EXECUTABLE(intl_desktop_file)

