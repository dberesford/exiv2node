
##Exiv2Node

Exiv2Node is a native c++ extension for node.js that provides asynchronous support for reading & writing image metadata via Exiv2 (http://www.exiv2.org).

## Dependencies

Needs Exiv2, see http://www.exiv2.org/download.html.

## Build Instructions

 - Download source
 - node-waf configure && node-waf build
 - node-waf install

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
