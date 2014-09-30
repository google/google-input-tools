{
  'targets': [
    {
      'target_name': 'base',
      'type': '<(library)',
      'includes': [
        'base.gypi',
      ],
    }
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'base_x64',
          'type': '<(library)',
          'includes': [
            'base.gypi',
          ],
        }
      ],
    },],
  ],
}
