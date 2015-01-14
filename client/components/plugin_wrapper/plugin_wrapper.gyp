{
  'targets': [
    {
      'target_name': 'plugin_wrapper',
      'type': '<(library)',
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)/protoc_out',
      ],
      'sources': [
        'exports.cc',
        'plugin_component_host.cc',
      ],
      'dependencies': [
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
      ],
    },
    {
      'target_name': 'plugin_component_stub',
      'type': '<(library)',
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)/protoc_out',
      ],
      'dependencies': [
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
      ],
      'sources': [
        'plugin_component_stub.cc',
        'plugin_instance_win.cc',
        #'plugin_instance_posix.cc',
      ],
      'conditions': [
        ['OS=="win"', {
          'sources/': [
            ['exclude', '_posix\\.cc$'],
          ]
        }],
        ['OS=="linux" or OS=="mac"', {
          'sources/': [
            ['exclude', '_win\\.cc$'],
          ]
        }],
      ],
    }
  ],
}
