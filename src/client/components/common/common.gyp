{
  'targets': [
    {
      'target_name': 'common',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/ipc/ipc.gyp:ipc',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
      ],
      'sources': [
        'file_utils_posix.cc',
        'file_utils_win.cc',
        'hotkey_util.cc',
        'focus_input_context_manager_sub_component.cc',
        'virtual_keyboard_and_character_picker_consts.cc',
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
    },
    {
      'target_name': 'common_unittests',
      'type': 'executable',
      'dependencies': [
        'common',
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
