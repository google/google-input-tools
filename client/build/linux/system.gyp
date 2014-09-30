{
  'targets': [
    {
      'target_name': 'glib',
      'type': '<(library)',
      'conditions': [
        ['_toolset=="target"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags glib-2.0)',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other glib-2.0)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l glib-2.0)',
              '-lpthread',
            ],
          },
      }]]
    },
    {
      'target_name': 'gtk',
      'type': '<(library)',
      'conditions': [
        ['_toolset=="target"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags gtk+-2.0)',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other gtk+-2.0)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l gtk+-2.0)',
              '-lpthread',
            ],
          },
      }]]
    },
    {
      'target_name': 'm17n',
      'type': '<(library)',
      'conditions': [
        ['_toolset=="target"', {
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags m17n-shell)',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other m17n-shell)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l m17n-shell)',
            ],
          },
      }]]
    },
  ],
}
