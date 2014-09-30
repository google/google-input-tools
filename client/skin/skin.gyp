{
  'targets': [
    {
      'target_name': 'skin',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/third_party/google_gadgets_for_linux/ggadget.gyp:ggadget',
      ],
      'sources': [
        'candidate_element.cc',
        'candidate_element.h',
        'candidate_list_element.cc',
        'candidate_list_element.h',
        'composition_element.cc',
        'composition_element.h',
        'skin.cc',
        'skin.h',
        'skin_consts.cc',
        'skin_consts.h',
        'skin_host.cc',
        'skin_host.h',
        'toolbar_element.cc',
        'toolbar_element.h',
      ],
      'export_dependent_settings': [
        '<(DEPTH)/third_party/google_gadgets_for_linux/ggadget.gyp:ggadget',
      ],
      'conditions': [
        ['OS=="mac"', {
          'sources': [
            'skin_host_mac.mm',
            'skin_host_mac.h',
          ],
        }],
		['OS=="win"', {
          'sources': [
            'skin_host_win.cc',
            'skin_host_win.h',
			'skin_library_initializer.h',
			'skin_library_initializer.cc',
          ],
        }],
      ],
    },
  ],
  # We don't add unit tests for .cc files in this directory since it's
  # over-complicated to create and initialize a skin ui element, and it's more
  # intuitive to test these code by running real cases.
}
