{
  'targets': [
    {
      'target_name': 'ipc_service',
	  'variables': {
          'signing%': 'true',
      },
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/common/common.gyp:common',
      ],
      'sources': [
        'ipc_service_win.cc',
        'ipc_service_win.rc',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          'AdditionalDependencies': [
            'wtsapi32.lib',
          ],
        },
      }
    },
  ],
}
