{
  'targets': [
    {
      'target_name': 'exiv2',
      'sources': [
        'exiv2node.cc'
      ],
      'xcode_settings': {
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
      },
      'cflags': [
        '<!@(pkg-config --cflags exiv2)'
      ],
      'cflags_cc': [
        '-fexceptions'
      ],
      'libraries': [
        '<!@(pkg-config --libs exiv2)'
      ],
      'ldflags': [
        '<!@(pkg-config --libs exiv2)'
      ]
    }
  ]
}
