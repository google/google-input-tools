{
  'includes': [
    'build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'All',
      'type': 'none',
      'dependencies': [
        'base/base.gyp:base',
		'components/common/common.gyp:common',
		'components/keyboard_input/keyboard_input.gyp:keyboard_input',
		'components/plugin_manager/plugin_manager.gyp:plugin_manager',
		'components/plugin_wrapper/plugin_wrapper.gyp:plugin_wrapper',
		'components/plugin_wrapper/plugin_wrapper.gyp:plugin_component_stub',
		'components/win_frontend/ipc_console.gyp:ipc_console',
		'components/settings_store/settings_store.gyp:settings_store',
		'tsf/tsf.gyp:tsf',
		'components/ui/ui.gyp:ui',
        'ipc/ipc.gyp:ipc',
		
      ],
    },
    {
      'target_name': 'test_targets',
      'type': 'none',
      'dependencies': [
        'ipc/ipc.gyp:ipc_unittests',
		'components/common/common.gyp:common_unittests',
		'components/settings_store/settings_store.gyp:settings_store_unittests',
      ],
    },
  ],
}
