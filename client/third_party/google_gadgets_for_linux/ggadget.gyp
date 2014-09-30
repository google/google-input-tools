{
  'target_defaults': {
    'defines': [
      'GGL_FOR_GOOPY',
    ],
    'include_dirs': [
      '.',
      '<(SHARED_INTERMEDIATE_DIR)/third_party/google_gadgets_for_linux',
      '<(ZLIB)',
    ],
    'conditions' : [
      ['OS=="win"', {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'UsePrecompiledHeader': '0',           # /Yc
            'PrecompiledHeaderThrough': '',
            'ForcedIncludeFiles': '<(DEPTH)/build/min_max.h',
          },
        },
      },],
    ],
  },
  'variables': {
    'libggadget_binary_version': '1.0.0',
    'ggl_major_version': 0,
    'ggl_minor_version': 11,
    'ggl_micro_version': 2,
    'ggl_version': '<(ggl_major_version).<(ggl_minor_version).<(ggl_micro_version)',
    'ggl_version_timestamp': '100127-000000',
    'ggl_api_major_version': 5,
    'ggl_api_minor_version': 8,
    'ggl_api_version': '<(ggl_api_major_version).<(ggl_api_minor_version).0.0',
    # TODO: set following variables based on architecture
    'ggl_sizeof_long_int': 8,
    'ggl_sizeof_size_t': 8,
  },  # variables
  'conditions': [
    ['OS=="linux"', {
      'variables': {
        'ggl_platform': 'linux',
      },
    },],
    ['OS=="mac"', {
      'variables': {
        'ggl_platform': 'mac',
      },
    },],
  ],  # conditions

  'targets': [
    {
      'target_name': 'ggadget',
      'type': '<(library)',
      'sources': [
        'ggadget/anchor_element.cc',
        'ggadget/anchor_element.h',
        'ggadget/basic_element.cc',
        'ggadget/basic_element.h',
        'ggadget/build_config.h',
        'ggadget/button_element.cc',
        'ggadget/button_element.h',
        'ggadget/canvas_interface.h',
        'ggadget/canvas_utils.cc',
        'ggadget/canvas_utils.h',
        'ggadget/clip_region.cc',
        'ggadget/clip_region.h',
        'ggadget/color.cc',
        'ggadget/color.h',
        'ggadget/common.h',
        'ggadget/copy_element.cc',
        'ggadget/copy_element.h',
        'ggadget/dir_file_manager.cc',
        'ggadget/dir_file_manager.h',
        'ggadget/div_element.cc',
        'ggadget/div_element.h',
        'ggadget/element_factory.cc',
        'ggadget/element_factory.h',
        'ggadget/elements.cc',
        'ggadget/elements.h',
        'ggadget/event.h',
        'ggadget/file_manager_factory.cc',
        'ggadget/file_manager_factory.h',
        'ggadget/file_manager_interface.h',
        'ggadget/file_manager_wrapper.cc',
        'ggadget/file_manager_wrapper.h',
        'ggadget/file_system_interface.h',
        'ggadget/font_interface.h',
        'ggadget/format_macros.h',
        'ggadget/framework_interface.h',
        'ggadget/gadget_base.cc',
        'ggadget/gadget_base.h',
        'ggadget/gadget_consts.h',
        'ggadget/gadget_interface.h',
        'ggadget/graphics_interface.h',
        'ggadget/host_interface.h',
        'ggadget/host_utils.cc',
        'ggadget/host_utils.h',
        'ggadget/image_cache.cc',
        'ggadget/image_cache.h',
        'ggadget/image_interface.h',
        'ggadget/img_element.cc',
        'ggadget/img_element.h',
        'ggadget/label_element.cc',
        'ggadget/label_element.h',
        'ggadget/light_map.h',
        'ggadget/linear_element.cc',
        'ggadget/linear_element.h',
        'ggadget/locales.cc',
        'ggadget/locales.h',
        'ggadget/localized_file_manager.cc',
        'ggadget/localized_file_manager.h',
        'ggadget/logger.cc',
        'ggadget/logger.h',
        'ggadget/main_loop.cc',
        'ggadget/main_loop_interface.h',
        'ggadget/math_utils.cc',
        'ggadget/math_utils.h',
        'ggadget/memory_options.cc',
        'ggadget/memory_options.h',
        'ggadget/menu_interface.h',
        'ggadget/messages.cc',
        'ggadget/messages.h',
        'ggadget/options_factory.cc',
        'ggadget/options_interface.h',
        'ggadget/permissions.cc',
        'ggadget/permissions.h',
        'ggadget/registerable_interface.h',
        'ggadget/scoped_ptr.h',
        'ggadget/script_context_interface.h',
        'ggadget/script_runtime_interface.h',
        'ggadget/script_runtime_manager.cc',
        'ggadget/script_runtime_manager.h',
        'ggadget/scriptable_array.cc',
        'ggadget/scriptable_array.h',
        'ggadget/scriptable_binary_data.h',
        'ggadget/scriptable_enumerator.h',
        'ggadget/scriptable_event.cc',
        'ggadget/scriptable_event.h',
        'ggadget/scriptable_function.h',
        'ggadget/scriptable_helper.cc',
        'ggadget/scriptable_helper.h',
        'ggadget/scriptable_holder.h',
        'ggadget/scriptable_image.cc',
        'ggadget/scriptable_image.h',
        'ggadget/scriptable_interface.h',
        'ggadget/scriptable_map.h',
        'ggadget/scriptable_menu.cc',
        'ggadget/scriptable_menu.h',
        'ggadget/scriptable_options.cc',
        'ggadget/scriptable_options.h',
        'ggadget/scriptable_view.cc',
        'ggadget/scriptable_view.h',
        'ggadget/scrollbar_element.cc',
        'ggadget/scrollbar_element.h',
        'ggadget/scrolling_element.cc',
        'ggadget/scrolling_element.h',
        'ggadget/signals.cc',
        'ggadget/signals.h',
        'ggadget/slot.cc',
        'ggadget/slot.h',
        'ggadget/string_utils.cc',
        'ggadget/string_utils.h',
        'ggadget/system_file_functions.h',
        'ggadget/system_utils.cc',
        'ggadget/system_utils.h',
        'ggadget/text_formats.cc',
        'ggadget/text_formats.h',
        'ggadget/text_formats_decl.h',
        'ggadget/text_frame.cc',
        'ggadget/text_frame.h',
        'ggadget/text_renderer_interface.h',
        'ggadget/texture.cc',
        'ggadget/texture.h',
        'ggadget/unicode_utils.cc',
        'ggadget/unicode_utils.h',
        'ggadget/variant.cc',
        'ggadget/variant.h',
        'ggadget/view.cc',
        'ggadget/view.h',
        'ggadget/view_host_interface.h',
        'ggadget/view_interface.h',
        'ggadget/xml_dom.cc',
        'ggadget/xml_dom.h',
        'ggadget/xml_dom_interface.h',
        'ggadget/xml_http_request_factory.cc',
        'ggadget/xml_http_request_interface.h',
        'ggadget/xml_parser.cc',
        'ggadget/xml_parser_interface.h',
        'ggadget/xml_utils.cc',
        'ggadget/xml_utils.h',
        'ggadget/zip_file_manager.cc',
        'ggadget/zip_file_manager.h',
      ],
      'dependencies' : [
        'unzip',
      ],
      'direct_dependent_settings': {
        'defines': [
          'GGL_FOR_GOOPY',
        ],
        'include_dirs': [
          '.',
          '<(SHARED_INTERMEDIATE_DIR)/third_party/google_gadgets_for_linux',
        ],
      },
      'conditions': [
        ['OS!="win"', {
          'sources': [
            'ggadget/small_object.cc',
            'ggadget/small_object.h',
          ],
        }],
        ['OS=="win"', {
          'sources': [
            'ggadget/system_file_functions_win32.cc',
          ],
          'dependencies': [
            'ggadget_win32',
          ]
        }],
        ['OS=="mac"', {
          'sources': [
            'ggadget/mac/cocoa_main_loop.h',
            'ggadget/mac/cocoa_main_loop.mm',
            'ggadget/mac/content_view.h',
            'ggadget/mac/content_view.mm',
            'ggadget/mac/content_window.h',
            'ggadget/mac/content_window.mm',
            'ggadget/mac/ct_font.h',
            'ggadget/mac/ct_font.mm',
            'ggadget/mac/ct_text_renderer.h',
            'ggadget/mac/ct_text_renderer.mm',
            'ggadget/mac/mac_array.h',
            'ggadget/mac/quartz_canvas.h',
            'ggadget/mac/quartz_canvas.mm',
            'ggadget/mac/quartz_graphics.h',
            'ggadget/mac/quartz_graphics.mm',
            'ggadget/mac/quartz_image.h',
            'ggadget/mac/quartz_image.mm',
            'ggadget/mac/scoped_cftyperef.h',
            'ggadget/mac/single_view_host.h',
            'ggadget/mac/single_view_host.mm',
            'ggadget/mac/utils.h',
            'ggadget/mac/utils.mm',
          ],
          'link_settings': {
            'libraries': [
              '/usr/lib/libz.dylib',
            ],
          }
        },],
        ['OS=="linux"', {
          'sources': [
          ],
          'link_settings': {
            'libraries': [
              '-lz',
            ],
          },
        }],
      ],
    }, # target ggadget

    {
      'target_name': 'unzip',
      'type': '<(library)',
      'sources': [
        'third_party/unzip/ioapi.c',
        'third_party/unzip/mztools.c',
        'third_party/unzip/unzip.c',
        'third_party/unzip/zip.c'
      ],
      'dependencies': [
        '<(DEPTH)/third_party/zlib/zlib.gyp:zlib',
      ],
	  'conditions': [
	    ['OS=="win"', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'UsePrecompiledHeader': '0',           # /Yc
              'PrecompiledHeaderThrough': '',
              'ForcedIncludeFiles': '',
            },
          },
        },],
	  ],
    }, # target unzip
    {
      'target_name': 'ggadget_sysdeps_header',
      'type': 'none',
      'conditions': [
        # Windows doesn't have sed, thus we use a pre-defined sysdeps.h in
        # ggadget/win/
        ['OS!="win"', {
          'actions': [
            {
              'action_name': 'sysdeps_header',
              'inputs': [
                'ggadget/sysdeps.h.in',
              ],
              'outputs': [
                '<(SHARED_INTERMEDIATE_DIR)/third_party/google_gadgets_for_linux/ggadget/sysdeps.h',
              ],
              'action': [
                'tools/cmd_redirection.sh',
                '<@(_outputs)',
                'sed',
                '-e', 's/@GGL_SIZEOF_CHAR@/1/',
                '-e', 's/@GGL_SIZEOF_SHORT_INT@/2/',
                '-e', 's/@GGL_SIZEOF_INT@/4/',
                '-e', 's/@GGL_SIZEOF_LONG_INT@/<(ggl_sizeof_long_int)/',
                '-e', 's/@GGL_SIZEOF_SIZE_T@/<(ggl_sizeof_size_t)/',
                '-e', 's/@GGL_SIZEOF_DOUBLE@/8/',
                '-e', 's/@GGL_SIZEOF_WCHAR_T@/4/',
                '-e', 's/@GGL_PLATFORM@/\"<(ggl_platform)\"/',
                '-e', 's/@GGL_PLATFORM_SHORT@/\"<(ggl_platform)\"/',
                '-e', 's/@LIBGGADGET_BINARY_VERSION@/\"<(libggadget_binary_version)\"/',
                '-e', 's/@GGL_VERSION@/\"<(ggl_version)\"/',
                '-e', 's/@GGL_MAJOR_VERSION@/<(ggl_major_version)/',
                '-e', 's/@GGL_MINOR_VERSION@/<(ggl_minor_version)/',
                '-e', 's/@GGL_MICRO_VERSION@/<(ggl_minor_version)/',
                '-e', 's/@GGL_VERSION_TIMESTAMP@/\"<(ggl_version_timestamp)\"/',
                '-e', 's/@GGL_API_MAJOR_VERSION@/<(ggl_api_major_version)/',
                '-e', 's/@GGL_API_MINOR_VERSION@/<(ggl_api_minor_version)/',
                '-e', 's/@GGL_API_VERSION@/\"<(ggl_api_version)\"/',
                'ggadget/sysdeps.h.in',
              ],
              'message': 'Generating sysdeps header file: <@(_outputs)',
            },
          ],  # actions
        },],
      ],
    },  # target: ggadget_sysdeps_header
  ],  # targets
  'conditions' : [
    ['OS!="win"', {
      'targets' : [
        {
          'target_name': 'extensions',
          'type': '<(library)',
          'dependencies': [
              'ggadget_sysdeps_header',
          ],
          'sources': [
            'extensions/extensions.h',
            'extensions/extensions.cc',
            'extensions/libxml2_xml_parser/libxml2_xml_parser.cc',
          ],
          'conditions': [
            ['OS=="mac"', {
              'include_dirs': [
                '/usr/include/libxml2',
              ],
              'link_settings': {
                'libraries': [
                  '/usr/lib/libxml2.dylib',
                ],
              }
            }],  # OS == mac
            ['OS=="linux"', {
              'cflags': [
                '<!@(pkg-config --cflags libxml-2.0)',
              ],
              'link_settings': {
                'ldflags': [
                  '<!@(pkg-config --libs-only-L --libs-only-other libxml-2.0)',
                ],
                'libraries': [
                  '<!@(pkg-config --libs-only-l libxml-2.0)',
                ],
              },  #link_settings
            }],  #OS == linux
          ], # conditions
        },
      ],  # targets
    },],  # OS != win
    [ 'OS=="win"', {
      'targets' : [
        {
          'target_name': 'ggadget_win32',
          'type': '<(library)',
          'hard_dependency': 1,
          'sources': [
            'ggadget/win32/font_fallback.cc',
            'ggadget/win32/font_fallback.h',
            'ggadget/win32/gadget_window.cc',
            'ggadget/win32/gadget_window.h',
            'ggadget/win32/gdiplus_canvas.cc',
            'ggadget/win32/gdiplus_canvas.h',
            'ggadget/win32/gdiplus_font.cc',
            'ggadget/win32/gdiplus_font.h',
            'ggadget/win32/gdiplus_graphics.cc',
            'ggadget/win32/gdiplus_graphics.h',
            'ggadget/win32/gdiplus_image.cc',
            'ggadget/win32/gdiplus_image.h',
            'ggadget/win32/key_convert.cc',
            'ggadget/win32/key_convert.h',
            'ggadget/win32/main_loop.cc',
            'ggadget/win32/main_loop.h',
            'ggadget/win32/menu_builder.cc',
            'ggadget/win32/menu_builder.h',
            'ggadget/win32/port.h',
            'ggadget/win32/private_font_database.cc',
            'ggadget/win32/private_font_database.h',
            'ggadget/win32/single_view_host.cc',
            'ggadget/win32/single_view_host.h',
            'ggadget/win32/sysdeps.h',
            'ggadget/win32/tests',
            'ggadget/win32/text_renderer.cc',
            'ggadget/win32/text_renderer.h',
            'ggadget/win32/thread_local_singleton_holder.h',
            'ggadget/win32/utilities.cc',
            'ggadget/win32/utilities.h',
            'ggadget/win32/xml_parser.cc',
            'ggadget/win32/xml_parser.h',
            'ggadget/win32/xml_parser_int.cc',
            'ggadget/win32/xml_parser_int.h',
          ],
        },
      ],
    },],
  ],  # conditions
}
