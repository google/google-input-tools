{
  'conditions': [
    ['OS=="mac"', {
      'targets': [
        {
          'target_name': 'protobuf',
          'type': 'none',
          'direct_dependent_settings': {
            'xcode_settings': {
              'OTHER_LDFLAGS': [
                '<!@(pkg-config --libs-only-L --libs-only-l protobuf)',
              ],
              'OTHER_CFLAGS': [
                '<!@(pkg-config --cflags protobuf)',
              ],
            },
          },
        },
      ],
    },],
    ['OS=="linux"', {
      'targets': [
        {
          'target_name': 'protobuf',
          'type': 'none',
          'direct_dependent_settings': {
            'cflags': [
              '<!@(pkg-config --cflags protobuf)',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(pkg-config --libs-only-L --libs-only-other protobuf)',
            ],
            'libraries': [
              '<!@(pkg-config --libs-only-l protobuf)',
            ],
          },
        },
      ],
    },],
    ['OS=="win"', {
      'variables': {
        'srcs': [
           '<(PROTOBUF)/google/protobuf/stubs/atomicops_internals_x86_msvc.cc',
           '<(PROTOBUF)/google/protobuf/stubs/common.cc',
           '<(PROTOBUF)/google/protobuf/stubs/once.cc',
           '<(PROTOBUF)/google/protobuf/stubs/stringprintf.cc',
           '<(PROTOBUF)/google/protobuf/stubs/structurally_valid.cc',
           '<(PROTOBUF)/google/protobuf/stubs/strutil.cc',
           '<(PROTOBUF)/google/protobuf/stubs/substitute.cc',
           '<(PROTOBUF)/google/protobuf/descriptor.cc',
           '<(PROTOBUF)/google/protobuf/descriptor.pb.cc',
           '<(PROTOBUF)/google/protobuf/descriptor_database.cc',
           '<(PROTOBUF)/google/protobuf/dynamic_message.cc',
           '<(PROTOBUF)/google/protobuf/extension_set.cc',
           '<(PROTOBUF)/google/protobuf/extension_set_heavy.cc',
           '<(PROTOBUF)/google/protobuf/generated_message_reflection.cc',
           '<(PROTOBUF)/google/protobuf/generated_message_util.cc',
           '<(PROTOBUF)/google/protobuf/message.cc',
           '<(PROTOBUF)/google/protobuf/message_lite.cc',
           '<(PROTOBUF)/google/protobuf/repeated_field.cc',
           '<(PROTOBUF)/google/protobuf/service.cc',
           '<(PROTOBUF)/google/protobuf/text_format.cc',
           '<(PROTOBUF)/google/protobuf/unknown_field_set.cc',
           '<(PROTOBUF)/google/protobuf/io/coded_stream.cc',
           '<(PROTOBUF)/google/protobuf/io/printer.cc',
           '<(PROTOBUF)/google/protobuf/io/tokenizer.cc',
           '<(PROTOBUF)/google/protobuf/io/zero_copy_stream.cc',
           '<(PROTOBUF)/google/protobuf/io/zero_copy_stream_impl.cc',
           '<(PROTOBUF)/google/protobuf/io/zero_copy_stream_impl_lite.cc',
           '<(PROTOBUF)/google/protobuf/compiler/importer.cc',
           '<(PROTOBUF)/google/protobuf/compiler/parser.cc',
           '<(PROTOBUF)/google/protobuf/stubs/strutil.cc',
           '<(PROTOBUF)/google/protobuf/reflection_ops.cc',
           '<(PROTOBUF)/google/protobuf/wire_format.cc',
           '<(PROTOBUF)/google/protobuf/wire_format_lite.cc',
        ],
      },
      'targets': [
        {
          'target_name': 'protobuf',
          'type': "<(library)",
          'msvs_settings': {
            'VCCLCompilerTool': {
              'UsePrecompiledHeader': '0',           # /Yc
              'PrecompiledHeaderThrough': '',
              'ForcedIncludeFiles': '<(DEPTH)/build/min_max.h',
            },
          },
          'include_dirs': [
            '<(PROTOBUF)/../vsprojects/',
          ],
          'sources': [
            '<@(srcs)',
          ],
        },
        {
          'target_name': 'protobuf_x64',
          'type': "<(library)",
          'msvs_settings': {
            'VCCLCompilerTool': {
              'UsePrecompiledHeader': '0',           # /Yc
              'PrecompiledHeaderThrough': '',
              'ForcedIncludeFiles': '<(DEPTH)/build/min_max.h',
            },
          },
          'include_dirs': [
            '<(PROTOBUF)/../vsprojects/',
          ],
          'sources': [
            '<@(srcs)',
          ],
        },
      ],
    },],
  ],
}
