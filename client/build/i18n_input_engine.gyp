{
  'variables': {
    'ENGINE_ROOT': '<(GOOGLE3)/i18n/input/engine',
  },
  'conditions': [
    ['OS!="win"', {
      'variables': {
        'PROTOC': '<!(which protoc)',
      },
    }],
  ],
  'targets': [
    {
      'target_name': 'stubs',
      'type': '<(library)',
      'sources': [
        '<(ENGINE_ROOT)/stubs/google3/base/commandlineflags.cc',
        '<(ENGINE_ROOT)/stubs/google3/base/commandlineflags_reporting.cc',
        '<(ENGINE_ROOT)/stubs/google3/base/init_google.cc',
        '<(ENGINE_ROOT)/stubs/google3/base/logging.cc',
        '<(ENGINE_ROOT)/stubs/google3/base/mutex.cc',
        '<(ENGINE_ROOT)/stubs/google3/base/scoped_ptr_internals.cc',
        '<(ENGINE_ROOT)/stubs/google3/base/sysinfo.cc',
        '<(ENGINE_ROOT)/stubs/google3/base/vlog_is_on.cc',
      ],
      'conditions': [
        ['OS=="win"', {
          'sources': [
            '<(ENGINE_ROOT)/stubs/google3/base/mutex-internal-win.cc',
            '<(ENGINE_ROOT)/stubs/posix/sys/mman.cc',
            '<(ENGINE_ROOT)/stubs/posix/sys/time.cc',
            '<(ENGINE_ROOT)/stubs/posix/unistd.cc',
          ],
          'include_dirs': [
            '<(ENGINE_ROOT)/stubs/posix/',
          ],
        }],
      ],
    },
  ]
}
