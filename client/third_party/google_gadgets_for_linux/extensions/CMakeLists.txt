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

ADD_SUBDIRECTORY(curl_xml_http_request)
ADD_SUBDIRECTORY(qt_xml_http_request)
ADD_SUBDIRECTORY(dbus_script_class)
ADD_SUBDIRECTORY(default_framework)
ADD_SUBDIRECTORY(gst_audio_framework)
ADD_SUBDIRECTORY(gst_video_element)
ADD_SUBDIRECTORY(gtk_edit_element)
ADD_SUBDIRECTORY(qt_edit_element)
ADD_SUBDIRECTORY(gtk_system_framework)
ADD_SUBDIRECTORY(qt_system_framework)
ADD_SUBDIRECTORY(qtwebkit_browser_element)
ADD_SUBDIRECTORY(gtkmoz_browser_element)
ADD_SUBDIRECTORY(libxml2_xml_parser)
IF(NOT APPLE)
  ADD_SUBDIRECTORY(linux_system_framework)
ENDIF(NOT APPLE)
ADD_SUBDIRECTORY(smjs_script_runtime)
ADD_SUBDIRECTORY(qt_script_runtime)
ADD_SUBDIRECTORY(default_options)
ADD_SUBDIRECTORY(google_gadget_manager)
ADD_SUBDIRECTORY(gtk_flash_element)
ADD_SUBDIRECTORY(analytics_usage_collector)
ADD_SUBDIRECTORY(webkit_script_runtime)
ADD_SUBDIRECTORY(gtkwebkit_browser_element)
ADD_SUBDIRECTORY(html_flash_element)
ADD_SUBDIRECTORY(soup_xml_http_request)
