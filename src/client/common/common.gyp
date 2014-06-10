{
  'targets': [
    {
      'target_name': 'common',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        'app_const.cc',
		'app_const.h',
        'clipboard.cc',
        'clipboard.h',
        'debug.h',
        'debug.cc',
        'google_search_utils.cc',
        'google_search_utils.h',
      ],
      'conditions': [
        ['OS=="win"', {
          'sources': [
		    'app_utils.cc',
		    'app_utils.h',
            'charsetutils.cc',
            'charsetutils.h',
            'clipboard_win.cc',
            'registry.cc',
            'registry.h',
            'registry_monitor.cc',
            'registry_monitor.h',
            'shellutils.cc',
            'shellutils.h',
            'smart_com_ptr.cc',
            'smart_com_ptr.h',
		    'process_quit_controller.cc',
		    'process_quit_controller.h',
            'ui_utils.cc',
            'ui_utils.h',
          ]
        }],
      ],
    },
  ],
}
