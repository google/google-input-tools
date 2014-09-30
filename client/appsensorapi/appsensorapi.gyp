{
  'variables': {
    'srcs': [
      'appsensor.cc',
      'appsensor.h',
      'appsensorapi.cc',
      'appsensorapi.h',
      'appsensor_helper.cc',
      'appsensor_helper.h',
      'common.cc',
      'common.h',
      'handler.cc',
      'handler.h',
      'handlermanager.cc',
      'handlermanager.h',
      'versionreader.cc',
      'versionreader.h',
    ],
  },
  'targets': [
    {
      'target_name': 'appsensorapi',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        '<@(srcs)',
      ],
    },
    {
      'target_name': 'appsensorapi_x64',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base_x64',
      ],
      'sources': [
        '<@(srcs)',
      ],
    },
  ],
}
