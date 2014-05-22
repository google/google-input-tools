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

IF(GGL_BUILD_GTK_EDIT_ELEMENT)

APPLY_CONFIG(CAIRO)
APPLY_CONFIG(PANGO)
APPLY_CONFIG(GLIB2)
APPLY_CONFIG(GTK2)

SET(SRCS
  gtk_edit_element.cc
  gtk_edit_impl.cc
)
ADD_MODULE(gtk-edit-element ${SRCS})
TARGET_LINK_LIBRARIES(gtk-edit-element
  ggadget${GGL_EPOCH}
  ggadget-gtk${GGL_EPOCH}
  ${CAIRO_LIBRARIES}
  ${PANGO_LIBRARIES}
  ${GTK2_LIBRARIES}
)
OUTPUT_MODULE(gtk-edit-element)

ENDIF(GGL_BUILD_GTK_EDIT_ELEMENT)
