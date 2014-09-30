#
#  Copyright 2008 Google Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

IF(GGL_BUILD_GTKWEBKIT_BROWSER_ELEMENT)

APPLY_CONFIG(GTK2)
APPLY_CONFIG(GTKWEBKIT)

SET(SRCS browser_element.cc)

ADD_MODULE(gtkwebkit-browser-element ${SRCS})
TARGET_LINK_LIBRARIES(gtkwebkit-browser-element
  ggadget${GGL_EPOCH}
  ggadget-webkitjs${GGL_EPOCH}
  ${GTK2_LIBRARIES}
  ${GTKWEBKIT_LIBRARIES}
  dl
)
OUTPUT_MODULE(gtkwebkit-browser-element)

ENDIF(GGL_BUILD_GTKWEBKIT_BROWSER_ELEMENT)
