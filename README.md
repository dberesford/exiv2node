[![build status](https://secure.travis-ci.org/dberesford/exiv2node.png)](http://travis-ci.org/dberesford/exiv2node)

#Exiv2

Exiv2 is a native c++ extension for [node.js](http://nodejs.org/) that provides support for reading & writing image metadata via [Exiv2 library](http://www.exiv2.org).

## Dependencies

To build this addon you'll need the Exiv2 library and headers. On Debian/Ubuntu, `sudo apt-get install exiv2 libexiv2-dev`. See the [Exiv2 download page](http://www.exiv2.org/download.html) for more information.

The tests are written using [Mocha](https://github.com/visionmedia/mocha) and [Should](https://github.com/visionmedia/should.js).

## Installation Instructions

Install the library and headers using package manager appropriate to your system:

  - Debian: `apt-get install libexiv2 libexiv2-dev`
  - OS X MacPorts: `port install pkgconfig exiv2`
  - OS X Homebrew: `brew install pkg-config exiv2`

Install the module with npm:

    npm install exiv2

## Sample Usage

### Read tags:

    var ex = require('exiv2');

    ex.getImageTags('./photo.jpg', function(err, tags) {
      console.log("DateTime: " + tags["Exif.Image.DateTime"]);
      console.log("DateTimeOriginal: " + tags["Exif.Photo.DateTimeOriginal"]);
    });

### Load preview images:

    var ex = require('exiv2')
      , fs = require('fs');

    ex.getImagePreviews('./photo.jpg', function(err, previews) {
      // Display information about the previews.
      console.log(previews);

      // Or you can save them--though you'll probably want to check the MIME
      // type before picking an extension.
      fs.writeFile('preview.jpg', previews[0].data);
    });

### Write tags:

    var ex = require('exiv2')

    var newTags = {
      "Exif.Photo.UserComment" : "Some Comment..",
      "Exif.Canon.OwnerName" : "My Camera"
    };
    ex.setImageTags('./photo.jpg', , function(err){
      if (err) {
        console.log(err);
      } else {
        console.log("setImageTags complete..");
      }
    });

Take a look at the `examples/` and `test/` directories for more.

email: dberesford at gmail
twitter: @dberesford
