{
  'targets': [
    {
      'target_name': 'keyboard_input',
      'type': '<(library)',
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)/protoc_out',
      ],
      'sources': [
        'keyboard_input_component.cc',
		'keyboard_input_component.h',
      ],
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/ipc/ipc.gyp:ipc',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
      ],
    },
  ]
}
