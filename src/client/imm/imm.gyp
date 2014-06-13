{
  'variables': {
    'srcs': [
      'export.cc',
      'candidate_info.cc',
      'composition_string.cc',
      'debug.cc',
      'input_context.cc',
      'order.cc',
      'registrar.cc',
      'candidate_info.h',
      'common.h',
      'composition_string.h',
      'context.h',
      'context_locker.h',
      'context_manager.h',
      'debug.h',
      'immdev.h',
      'input_context.h',
      'message_queue.h',
      'mock_objects.h',
      'order.h',
      'registrar.h',
      'test_main.h',
      'ui_window.h',
    ],
  },
  'targets': [
    {
      'target_name': 'imm',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
        '<(DEPTH)/third_party/gtest/gtest.gyp:gtest_prod',
      ],
      'sources': [
        '<@(srcs)',
      ],
    },
    {
      'target_name': 'imm_x64',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base_x64',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp_x64',
        '<(DEPTH)/third_party/gtest/gtest.gyp:gtest_prod',
      ],
      'sources': [
        '<@(srcs)',
      ],
    },
  ],
}
