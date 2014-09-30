{
  'targets': [
    {
      'target_name': 'zlib',
      'conditions': [
        ['_toolset=="target"', {
          'conditions': [
            ['OS=="mac"', {
              'type': 'none',
              'direct_dependent_settings': {
                'xcode_settings': {
                  'OTHER_LDFLAGS': [
                    '<!@(pkg-config --libs-only-L --libs-only-l zlib)',
                  ],
                  'OTHER_CFLAGS': [
                    '<!@(pkg-config --cflags zlib)',
                  ],
                },
              }
            }],
            ['OS=="linux"', {
              'type': 'none',
              'direct_dependent_settings': {
                'cflags': [
                  '<!@(pkg-config --cflags zlib)',
                ],
              },
              'link_settings': {
                'ldflags': [
                  '<!@(pkg-config --libs-only-L --libs-only-other zlib)',
                ],
                'libraries': [
                  '<!@(pkg-config --libs-only-l zlib)',
                ],
              },
            }],
            ['OS=="win"', {
              'type': "<(library)",
              'msvs_settings': {
                'VCCLCompilerTool': {
                  'UsePrecompiledHeader': '0',           # /Yc
                  'PrecompiledHeaderThrough': '',
                  'ForcedIncludeFiles': '',
                },
              },
              'sources': [
                 '<(ZLIB)/adler32.c',
                 '<(ZLIB)/compress.c',
                 '<(ZLIB)/crc32.c',
                 '<(ZLIB)/deflate.c',
                 '<(ZLIB)/gzclose.c',
                 '<(ZLIB)/gzlib.c',
                 '<(ZLIB)/gzread.c',
                 '<(ZLIB)/gzwrite.c',
                 '<(ZLIB)/infback.c',
                 '<(ZLIB)/inffast.c',
                 '<(ZLIB)/inflate.c',
                 '<(ZLIB)/inftrees.c',
                 '<(ZLIB)/trees.c',
                 '<(ZLIB)/uncompr.c',
                 '<(ZLIB)/zutil.c',
              ],
            }],
          ],
        }],
      ]
    },
  ],
}
