
##Exiv2Node

Exiv2Node is a native c++ extension for node.js that provides asynchronous support for reading image metadata via Exiv2 (http://www.exiv2.org). Support for modifying metadata to follow..

## Dependencies

Needs Exiv2, see http://www.exiv2.org/download.html.

## Build Instructions

 - Download source
 - node-waf configure && node-waf build
 - node-waf install

## Sample Usage

var exiv2node = require('exiv2node');
var assert = require('assert');

ex.getImageTags('./photo.jpg', function(tags) {
 if (tags) {	
  for (key in tags){
   console.log(key + ":" + tags[key]);
  }
	
  console.log("DateTime: " + tags["Exif.Image.DateTime"]);
  console.log("DateTimeOriginal: " + tags["Exif.Photo.DateTimeOriginal"]);
 }
});

See test.js.

email: dberesford at gmail
twitter: @dberesford
