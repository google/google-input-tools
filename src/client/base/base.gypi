{
  'type': '<(library)',
  'sources': [
    'at_exit.cc',
    'callback.cc',
    'commandlineflags.cc',
    'commandlineflags_reporting.cc',
    'cpu.cc',
    'file/base_paths.cc',
    'file/file_path.cc',
    'file/path_service.cc',
    'logging.cc',
    'mutex-internal-win.cc',
    'mutex.cc',
    'stringprintf.cc',
    'sysinfo.cc',
    'vlog_is_on.cc',
    'stringprintf.cc',
    'synchronization/cancellation_flag.cc', # Synchronization
    'synchronization/lock.cc',
    'threading/thread_collision_warner.cc', # Threading
    'time.cc',
    'security_utils_win.cc',
    'string_utils_win.cc',
  ],
  'conditions': [
    ['OS=="win"', {
      'sources': [
        'file/base_paths_win.cc',
        'file/file_util_win.cc',
        'synchronization/lock_impl_win.cc',
        'synchronization/waitable_event_win.cc',
        'threading/platform_thread_win.cc', # Threading
        'time_win.cc',
        'win/windows_version.cc',
        'win/security_util_win.cc',
        'win/shellutils.cc',
        'posix/sys/mman.cc',
        'posix/unistd.cc',
      ],
    }],
    ['OS!="win"', {
      'sources': [
         'synchronization/lock_impl_posix.cc',
      ],
    }],
    ['OS=="mac"', {
      'sources': [
         # 'file/base_paths_mac.mm',
      ],
    }],
    ['OS=="linux"', {
      'sources': [
         # 'file/base_paths_linux.cc,
      ],
    }],
  ],
}
