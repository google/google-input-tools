{
  'targets': [
    {
      'target_name': 'ui',
      'type': '<(library)',
      'include_dirs': [
        '<(DEPTH)/third_party/google_gadgets_for_linux',
        '<(SHARED_INTERMEDIATE_DIR)/third_party/google_gadgets_for_linux',
      ],
      'dependencies': [
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
        '<(DEPTH)/third_party/google_gadgets_for_linux/ggadget.gyp:ggadget',
		'<(DEPTH)/skin/skin.gyp:skin',
      ],
      'sources': [
        'composing_window_position.cc',
        'composing_window_position.h',
        'cursor_trapper.cc',
        'cursor_trapper.h',
        'skin_toolbar_manager.cc',
        'skin_toolbar_manager.h',
        'skin_ui_component.cc',
        'skin_ui_component.h',
        'skin_ui_component_utils.h',
        'skin_ui_component_utils.cc',
        'ui_component_base.cc',
        'ui_component_base.h',
      ],
      'conditions': [
        ['OS=="mac"', {
          'sources': [
            'skin_ui_component_mac.mm',
            'skin_ui_component_utils_mac.mm',
          ],
        }],
		['OS=="win"', {
		  'sources': [
            'about_dialog.h',
            'about_dialog.cc',
            'skin_ui_component_win.cc',
            'skin_ui_component_utils_win.cc'
		  ]
		}],
      ],
    },
  ],
}
