{
  'targets': [
    {
      'target_name': 'exiv2',
      'sources': [
        'exiv2node.cc'
      ],
      'xcode_settings': {
        'MACOSX_DEPLOYMENT_TARGET': '10.7',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'OTHER_CPLUSPLUSFLAGS': ['-stdlib=libc++','-fcxx-exceptions', '-frtti'],
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
