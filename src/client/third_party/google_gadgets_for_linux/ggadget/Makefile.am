#
# Copyright 2011 Google Inc.
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

EXTRA_DIST = CMakeLists.txt

# Putting . into SBUDIRS list to force building current directory first.
SUBDIRS = . tests js xdg npapi

if GGL_BUILD_LIBGGADGET_GTK
SUBDIRS += gtk
endif

if GGL_BUILD_LIBGGADGET_QT
SUBDIRS += qt
endif

if GGL_BUILD_LIBGGADGET_DBUS
SUBDIRS += dbus
endif

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

gglincludedir		= $(GGL_INCLUDE_DIR)/ggadget
gglsysdepsincludedir	= $(GGL_SYSDEPS_INCLUDE_DIR)/ggadget

gglinclude_HEADERS	= anchor_element.h \
			  audioclip_interface.h \
			  backoff.h \
			  basic_element.h \
			  build_config.h \
			  button_element.h \
			  canvas_interface.h \
			  canvas_utils.h \
			  checkbox_element.h \
			  clip_region.h \
			  color.h \
			  common.h \
			  combobox_element.h \
			  content_item.h \
			  contentarea_element.h \
			  copy_element.h \
			  decorated_view_host.h \
			  details_view_data.h \
			  details_view_decorator.h \
			  digest_utils.h \
			  dir_file_manager.h \
			  display_window.h \
			  div_element.h \
			  docked_main_view_decorator.h \
			  edit_element_base.h \
			  element_factory.h \
			  elements.h \
			  encryptor_interface.h \
			  event.h \
			  extension_manager.h \
			  file_manager_factory.h \
			  file_manager_interface.h \
			  file_manager_wrapper.h \
			  file_system_interface.h \
			  floating_main_view_decorator.h \
			  font_interface.h \
			  format_macros.h \
			  framed_view_decorator_base.h \
			  framework_interface.h \
			  gadget.h \
			  gadget_base.h \
			  gadget_consts.h \
			  gadget_interface.h \
			  gadget_manager_interface.h \
			  graphics_interface.h \
			  host_interface.h \
			  image_cache.h \
			  image_interface.h \
			  img_element.h \
			  item_element.h \
			  label_element.h \
			  light_map.h \
			  linear_element.h \
			  listbox_element.h \
			  locales.h \
			  localized_file_manager.h \
			  logger.h \
			  main_loop_interface.h \
			  main_view_decorator_base.h \
			  math_utils.h \
			  memory_options.h \
			  menu_interface.h \
			  messages.h \
			  module.h \
			  object_element.h \
			  object_videoplayer.h \
			  options_interface.h \
			  permissions.h \
			  popout_main_view_decorator.h \
			  progressbar_element.h \
			  registerable_interface.h \
			  run_once.h \
			  scoped_ptr.h \
			  script_context_interface.h \
			  script_runtime_interface.h \
			  script_runtime_manager.h \
			  scriptable_array.h \
			  scriptable_binary_data.h \
			  scriptable_enumerator.h \
			  scriptable_event.h \
			  scriptable_file_system.h \
			  scriptable_framework.h \
			  scriptable_function.h \
			  scriptable_helper.h \
			  scriptable_holder.h \
			  scriptable_image.h \
			  scriptable_interface.h \
			  scriptable_map.h \
			  scriptable_menu.h \
			  scriptable_options.h \
			  scriptable_view.h \
			  scrollbar_element.h \
			  scrolling_element.h \
			  sidebar.h \
			  signals.h \
			  slot.h \
			  small_object.h \
			  string_utils.h \
			  system_file_functions.h \
			  system_utils.h \
			  host_utils.h \
			  text_formats_decl.h \
			  text_formats.h \
			  text_frame.h \
			  texture.h \
			  unicode_utils.h \
			  usage_collector_interface.h \
			  uuid.h \
			  variant.h \
			  video_element_base.h \
			  view.h \
			  view_decorator_base.h \
			  view_element.h \
			  view_host_interface.h \
			  view_interface.h \
			  xml_dom_interface.h \
			  xml_dom.h \
			  xml_http_request_interface.h \
			  xml_http_request_utils.h \
			  xml_parser_interface.h \
			  xml_utils.h \
			  zip_file_manager.h

nodist_gglsysdepsinclude_HEADERS= sysdeps.h
BUILT_SOURCES=$(gglsysdepsinclude_HEADERS)

lib_LTLIBRARIES		= libggadget@GGL_EPOCH@.la

libggadget@GGL_EPOCH@_la_SOURCES = \
			  anchor_element.cc \
			  backoff.cc \
			  basic_element.cc \
			  button_element.cc \
			  canvas_utils.cc \
			  checkbox_element.cc \
			  clip_region.cc \
			  color.cc \
			  combobox_element.cc \
			  content_item.cc \
			  contentarea_element.cc \
			  copy_element.cc \
			  decorated_view_host.cc \
			  details_view_data.cc \
			  details_view_decorator.cc \
			  digest_utils.cc \
			  dir_file_manager.cc \
			  display_window.cc \
			  div_element.cc \
			  docked_main_view_decorator.cc \
			  edit_element_base.cc \
			  element_factory.cc \
			  elements.cc \
			  encryptor.cc \
			  extension_manager.cc \
			  file_manager_factory.cc \
			  file_manager_wrapper.cc \
			  floating_main_view_decorator.cc \
			  framed_view_decorator_base.cc \
			  gadget.cc \
			  gadget_base.cc \
			  gadget_manager.cc \
			  image_cache.cc \
			  img_element.cc \
			  item_element.cc \
			  label_element.cc \
			  linear_element.cc \
			  listbox_element.cc \
			  locales.cc \
			  localized_file_manager.cc \
			  logger.cc \
			  main_loop.cc \
			  main_view_decorator_base.cc \
			  math_utils.cc \
			  memory_options.cc \
			  messages.cc \
			  module.cc \
			  object_element.cc \
			  object_videoplayer.cc \
			  options_factory.cc \
			  permissions.cc \
			  popout_main_view_decorator.cc \
			  progressbar_element.cc \
			  run_once.cc \
			  script_runtime_manager.cc \
			  scriptable_array.cc \
			  scriptable_event.cc \
			  scriptable_file_system.cc \
			  scriptable_framework.cc \
			  scriptable_helper.cc \
			  scriptable_image.cc \
			  scriptable_menu.cc \
			  scriptable_options.cc \
			  scriptable_view.cc \
			  scrollbar_element.cc \
			  scrolling_element.cc \
			  sidebar.cc \
			  signals.cc \
			  slot.cc \
			  small_object.cc \
			  string_utils.cc \
			  system_file_functions_posix.cc \
			  system_utils.cc \
			  host_utils.cc \
			  text_formats.cc \
			  text_frame.cc \
			  texture.cc \
			  unicode_utils.cc \
			  usage_collector_factory.cc \
			  uuid.cc \
			  variant.cc \
			  video_element_base.cc \
			  view.cc \
			  view_decorator_base.cc \
			  view_element.cc \
			  xml_dom.cc \
			  xml_http_request_factory.cc \
			  xml_http_request_utils.cc \
			  xml_parser.cc \
			  xml_utils.cc \
			  zip_file_manager.cc

libggadget@GGL_EPOCH@_la_CPPFLAGS = \
			  $(LTDLINCL) \
			  $(PREDEFINED_MACROS)

libggadget@GGL_EPOCH@_la_CXXFLAGS = \
			  $(DEFAULT_COMPILE_FLAGS)

libggadget@GGL_EPOCH@_la_LIBADD = \
			  $(top_builddir)/third_party/unzip/libunzip.la

libggadget@GGL_EPOCH@_la_LDFLAGS = \
			  -version-info $(LIBGGADGET_VERSION) \
			  -no-undefined \
			  -export-dynamic \
			  $(LIBLTDL)

pkgconfigdir= $(libdir)/pkgconfig
pkgconfig_DATA= libggadget@GGL_EPOCH@.pc
