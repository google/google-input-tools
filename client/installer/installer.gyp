{
  'targets': [
    {
      'target_name': 'win32_register',
      'type': 'executable',
      'dependencies': [
	    '<(DEPTH)/appsensorapi/appsensorapi.gyp:appsensorapi',
        '<(DEPTH)/base/base.gyp:base',
		'<(DEPTH)/common/common.gyp:common',
        '<(DEPTH)/components/common/common.gyp:component_common',
        '<(DEPTH)/imm/imm.gyp:imm',
		'<(DEPTH)/ipc/ipc.gyp:ipc',
		'<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
        '<(DEPTH)/locale/locale.gyp:locale',
		'<(DEPTH)/third_party/google_gadgets_for_linux/ggadget.gyp:ggadget',
		'<(DEPTH)/tsf/tsf.gyp:tsf',
      ],
      'sources': [
        'win32_register.cc',
      ],
    },
  ],
}
