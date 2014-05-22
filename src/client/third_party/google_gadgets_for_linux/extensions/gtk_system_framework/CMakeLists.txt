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

IF(GGL_BUILD_LIBGGADGET_GTK)

APPLY_CONFIG(GTK2)

IF(RSVG_FOUND)
  APPLY_CONFIG(RSVG)
ENDIF(RSVG_FOUND)

ADD_MODULE(gtk-system-framework gtk_system_framework.cc)
TARGET_LINK_LIBRARIES(gtk-system-framework
  ggadget-gtk${GGL_EPOCH}
  ggadget-xdg${GGL_EPOCH}
  ${GTK_LIBRARIES}
)
OUTPUT_MODULE(gtk-system-framework)

ENDIF(GGL_BUILD_LIBGGADGET_GTK)
