{
  'targets': [
    {
      'target_name': 'text_range',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/ipc/protos/protos.gyp:protos-cpp',
      ],
      'sources': [
        'edit_ctrl_text_range.cc',
        'edit_ctrl_text_range.h',
        'html_text_range.cc',
        'html_text_range.h',
        'window_utils.cc',
        'window_utils.h',
      ],
    },
  ],
}
