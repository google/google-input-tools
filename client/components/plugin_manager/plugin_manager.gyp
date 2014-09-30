{
  'targets': [
    {
      'target_name': 'plugin_manager',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp'
      ],
      'sources': [
        'plugin_manager.cc',
        'plugin_manager_utils.cc',
        'plugin_manager_component.cc',
      ],
    },
    {
      'target_name': 'plugin_manager_unittests',
      'type': 'executable',
      'dependencies': [
        'plugin_manager',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
        '<(DEPTH)/third_party/gtest/gtest.gyp:gtest',
      ],
      'sources': [
        'plugin_manager_test.cc',
      ],
    },
  ],
}
