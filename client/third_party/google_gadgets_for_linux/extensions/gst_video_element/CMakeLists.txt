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

IF(GGL_BUILD_GST_VIDEO_ELEMENT)

APPLY_CONFIG(GSTREAMER)
APPLY_CONFIG(GSTREAMER_PLUGINS_BASE)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion")

SET(SRCS
  gadget_videosink.cc
  gst_video_element.cc
)
ADD_MODULE(gst-video-element ${SRCS})
TARGET_LINK_LIBRARIES(gst-video-element
  ${GSTREAMER_VIDEO_LIBRARIES} ggadget${GGL_EPOCH})

OUTPUT_MODULE(gst-video-element)

ENDIF(GGL_BUILD_GST_VIDEO_ELEMENT)
