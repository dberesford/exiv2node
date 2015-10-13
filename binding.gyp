{
  'targets': [
    {
      'target_name': 'exiv2',
      'sources': [
        'exiv2node.cc'
      ],
      'include_dirs' : [
        '<!@(pkg-config --variable=includedir exiv2)',
        "<!(node -e \"require('nan')\")"
      ],
      'xcode_settings': {
        'MACOSX_DEPLOYMENT_TARGET': '10.7',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'OTHER_CPLUSPLUSFLAGS': ['-stdlib=libc++','-fcxx-exceptions', '-frtti'],
      },
      'cflags_cc': [
        '-fexceptions'
      ],
      'libraries': [
        '<!@(pkg-config --libs exiv2)'
      ],
    }
  ]
}
