{
  'variables': {
    'srcs': [
      'locales.cc',
      'locales.h',
      'locale_utils.cc',
      'locale_utils.h',
      'text_utils.cc',
      'text_utils.h',
    ],
  },
  'targets': [
    {
      'target_name': 'locale',
      'type': '<(library)',
      'sources': [
        '<@(srcs)',
      ],
    },
    {
      'target_name': 'locale_x64',
      'type': '<(library)',
      'sources': [
        '<@(srcs)',
      ],
    },
  ],
}
