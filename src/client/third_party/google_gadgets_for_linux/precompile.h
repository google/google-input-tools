/*
  Copyright 2014 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

// Precompile header for common headers in google_gadgets_for_linux.

#ifndef PRECOMPILE_H__
#define PRECOMPILE_H__

#ifndef _ATL_NO_HOSTING
#define _ATL_NO_HOSTING
#endif

// Common windows headers
#include <windows.h>

#ifdef DrawText
#undef DrawText
#endif

// C++ headers
#include <algorithm>
#include <cstdio>
#include <functional>
#include <hash_map>
#include <map>
#include <string>
#include <set>
#include <utility>
#include <vector>

// Common headers.
#include <ggadget/anchor_element.h>
#include <ggadget/audioclip_interface.h>
#include <ggadget/backoff.h>
#include <ggadget/basic_element.h>
#include <ggadget/build_config.h>
#include <ggadget/button_element.h>
#include <ggadget/canvas_interface.h>
#include <ggadget/canvas_utils.h>
#include <ggadget/checkbox_element.h>
#include <ggadget/clip_region.h>
#include <ggadget/color.h>
#include <ggadget/combobox_element.h>
#include <ggadget/common.h>
#include <ggadget/contentarea_element.h>
#include <ggadget/content_item.h>
#include <ggadget/copy_element.h>
#include <ggadget/decorated_view_host.h>
#include <ggadget/details_view_data.h>
#include <ggadget/details_view_decorator.h>
#include <ggadget/digest_utils.h>
#include <ggadget/dir_file_manager.h>
#include <ggadget/display_window.h>
#include <ggadget/div_element.h>
#include <ggadget/docked_main_view_decorator.h>
#include <ggadget/edit_element_base.h>
#include <ggadget/elements.h>
#include <ggadget/element_factory.h>
#include <ggadget/encryptor_interface.h>
#include <ggadget/event.h>
#include <ggadget/extension_manager.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_interface.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/file_system_interface.h>
#include <ggadget/floating_main_view_decorator.h>
#include <ggadget/font_interface.h>
#include <ggadget/format_macros.h>
#include <ggadget/framed_view_decorator_base.h>
#include <ggadget/framework_interface.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_base.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/graphics_interface.h>
#include <ggadget/host_interface.h>
#include <ggadget/host_utils.h>
#include <ggadget/image_cache.h>
#include <ggadget/image_interface.h>
#include <ggadget/img_element.h>
#include <ggadget/item_element.h>
#include <ggadget/label_element.h>
#include <ggadget/light_map.h>
#include <ggadget/linear_element.h>
#include <ggadget/listbox_element.h>
#include <ggadget/locales.h>
#include <ggadget/localized_file_manager.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/main_view_decorator_base.h>
#include <ggadget/math_utils.h>
#include <ggadget/memory_options.h>
#include <ggadget/menu_interface.h>
#include <ggadget/messages.h>
#include <ggadget/module.h>
#include <ggadget/object_element.h>
#include <ggadget/object_videoplayer.h>
#include <ggadget/options_interface.h>
#include <ggadget/permissions.h>
#include <ggadget/popout_main_view_decorator.h>
#include <ggadget/progressbar_element.h>
#include <ggadget/registerable_interface.h>
#include <ggadget/run_once.h>
#include <ggadget/scoped_ptr.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/scriptable_enumerator.h>
#include <ggadget/scriptable_event.h>
#include <ggadget/scriptable_file_system.h>
#include <ggadget/scriptable_framework.h>
#include <ggadget/scriptable_function.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_holder.h>
#include <ggadget/scriptable_image.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/scriptable_map.h>
#include <ggadget/scriptable_menu.h>
#include <ggadget/scriptable_options.h>
#include <ggadget/scriptable_view.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/script_runtime_manager.h>
#include <ggadget/scrollbar_element.h>
#include <ggadget/scrolling_element.h>
#include <ggadget/sidebar.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/small_object.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_file_functions.h>
#include <ggadget/system_utils.h>
#include <ggadget/texture.h>
#include <ggadget/text_frame.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/usage_collector_interface.h>
#include <ggadget/uuid.h>
#include <ggadget/variant.h>
#include <ggadget/video_element_base.h>
#include <ggadget/view.h>
#include <ggadget/view_decorator_base.h>
#include <ggadget/view_element.h>
#include <ggadget/view_host_interface.h>
#include <ggadget/view_interface.h>
#include <ggadget/xml_dom.h>
#include <ggadget/xml_dom_interface.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_http_request_utils.h>
#include <ggadget/xml_parser_interface.h>
#include <ggadget/xml_utils.h>
#include <ggadget/zip_file_manager.h>

#endif  // PRECOMPILE_H__
