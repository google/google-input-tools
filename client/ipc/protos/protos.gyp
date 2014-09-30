{
  'targets': [
    {
      'target_name': 'protos-cpp',
      'type': '<(library)',
      'sources': [
        'ipc.protodevel',
      ],
      'variables': {
        'proto_out_dir': 'ipc/protos',
      },
      'includes': ['../../third_party/protobuf/protoc.gypi'],
    },
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'protos-cpp_x64',
          'type': '<(library)',
          'sources': [
            'ipc.protodevel',
          ],
          'variables': {
            'proto_out_dir': 'ipc/protos',
          },
          'includes': ['../../third_party/protobuf/protoc.gypi'],
        },
      ],
    },],
  ],
}
