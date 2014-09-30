{
  'variables': {
    'srcs': [
      'file_utils_win.cc',
      'hotkey_util.cc',
      'focus_input_context_manager_sub_component.cc',
    ],
  },
  'conditions': [
    ['OS=="linux" or OS=="mac"', {
      'variables': {
        'srcs': [
          'file_utils_posix.cc',
        ],
      },
    },],
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'component_common_x64',
          'type': '<(library)',
          'dependencies': [
            '<(DEPTH)/base/base.gyp:base_x64',
            '<(DEPTH)/ipc/ipc.gyp:ipc_x64',
            '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp_x64',
          ],
          'sources': [
            '<@(srcs)',
          ],
        },
      ],
    },],
  ],
  'targets': [
    {
      'target_name': 'component_common',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/ipc/ipc.gyp:ipc',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
      ],
      'sources': [
        '<@(srcs)',
      ],
    },
    {
      'target_name': 'common_unittests',
      'type': 'executable',
      'dependencies': [
        'component_common',
        '<(DEPTH)/third_party/gtest/gtest.gyp:gtest',
        '<(DEPTH)/third_party/protobuf/protobuf.gyp:protobuf',
		'<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
      ],
      'sources': [
        'mock_app_component.cc',
        'focus_input_context_manager_sub_component_test.cc',
        'unit_tests.cc',
      ],
    }
  ],
}
