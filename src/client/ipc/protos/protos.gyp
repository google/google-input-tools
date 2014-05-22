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
}
