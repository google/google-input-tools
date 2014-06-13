{
  'includes': [
    'build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'All',
      'type': 'none',
      'dependencies': [
        'components/win_frontend/ipc_console.gyp:ipc_console',
        'components/win_frontend/frontend_component.gyp:frontend_component',
        'components/win_frontend/frontend_component.gyp:frontend_component_x64',
        'installer/installer.gyp:win32_register',
        'ipc/service/ipc_service_win.gyp:ipc_service',
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
