
##Exiv2Node

Exiv2Node is a native c++ extension for node.js that provides asynchronous support for reading & writing image metadata via Exiv2 (http://www.exiv2.org).

## Dependencies

Needs Exiv2, see http://www.exiv2.org/download.html.

## Installation Instructions

Install the library and headers using package manager appropriate to your system:

  - Debian: `apt-get install libexiv2 libexiv2-dev`
  - OS X: `port install exiv2`

Install the module with npm:

    npm install exiv2

## Sample Usage

    var exiv2node = require('exiv2node');
  
    ex.getImageTags('./photo.jpg', function(err, tags) {
     if (err) {
       console.log(err);
     }else {	
      for (key in tags){
       console.log(key + ":" + tags[key]);
      }
    	
      console.log("DateTime: " + tags["Exif.Image.DateTime"]);
      console.log("DateTimeOriginal: " + tags["Exif.Photo.DateTimeOriginal"]);
     }
    });
    
    ex.setImageTags('./photo.jpg', { "Exif.Photo.UserComment" : "Some Comment..", "Exif.Canon.OwnerName" : "My Camera"}, function(err){    
      if (err) {
        console.log(err);
      }else {
        console.log("setImageTags complete..");
      }
    });

See test.js.

email: dberesford at gmail
twitter: @dberesford
