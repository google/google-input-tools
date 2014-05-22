{
  'targets': [
    {
      'target_name': 'ipc_console',
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
		'<(DEPTH)/components/keyboard_input/keyboard_input.gyp:keyboard_input',
		'<(DEPTH)/components/plugin_wrapper/plugin_wrapper.gyp:plugin_component_stub',
		'<(DEPTH)/components/plugin_manager/plugin_manager.gyp:plugin_manager',
		'<(DEPTH)/components/ui/ui.gyp:ui',
		'<(DEPTH)/ipc/ipc.gyp:ipc',
		'<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
		'<(DEPTH)/third_party/google_gadgets_for_linux/ggadget.gyp:ggadget',
		'<(DEPTH)/skin/skin.gyp:skin',
      ],
      'sources': [
        'ipc_console.cc',
        '<(DEPTH)/resources/ipc_console.rc',
      ],
    },
  ],
}
