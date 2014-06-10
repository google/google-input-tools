{
  'targets': [
    {
      'target_name': 'settings_store',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/ipc/ipc.gyp:ipc',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
      ],
      'include_dirs': [
        '<(GTEST)/include',
      ],
      'sources': [
        'settings_store_base.cc',
        'settings_store_base.h',
        'settings_store_memory.cc',
        'settings_store_memory.h',
		'settings_store_win.cc',
		'settings_store_win.h',
      ],
    },
    {
      'target_name': 'settings_store_unittests',
      'type': 'executable',
      'dependencies': [
        'settings_store',
        '<(DEPTH)/ipc/ipc.gyp:ipc_test_util',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
        '<(DEPTH)/third_party/gtest/gtest.gyp:gtest',
      ],
      'sources': [
        'settings_store_tests.cc',
        'settings_store_base_test.cc',
        'settings_store_memory_test.cc',
      ],
    }
  ],
}
