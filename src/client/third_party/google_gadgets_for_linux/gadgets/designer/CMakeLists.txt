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

IF(GGL_BUILD_GTK_HOST)
# Generates ggl-gtk.desktop file
INSTALL_BINARY_DESKTOP(ggl-designer)
ENDIF(GGL_BUILD_GTK_HOST)

ADD_MODULE(designer-script-utils designer_script_utils.cc)
TARGET_LINK_LIBRARIES(designer-script-utils ggadget${GGL_EPOCH})

ADD_DIR_TO_ZIP(blank_gadget blank-gadget.gg)
ADD_DIR_TO_ZIP(gadget ../../output/bin/designer.gg)
ADD_FILE_TO_ZIP_ABS(${CMAKE_CURRENT_BINARY_DIR}/blank-gadget.gg
  ${CMAKE_BINARY_DIR}/output/bin/designer.gg)
ADD_TARGET_TO_ZIP(designer-script-utils ../../output/bin/designer.gg)

INSTALL_GADGET(designer.gg)
