var ex = require('../exiv2');
var assert = require('assert');
var fs = require('fs')

var dir = __dirname + '/../test/images';

// Test basic image with exif tags
ex.getImageTags(dir + '/books.jpg', function(err, tags) {
  console.log(tags);
});

// Make a copy of our file so we don't polute the original.
fs.writeFileSync(dir + '/copy.jpg', fs.readFileSync(dir + '/books.jpg'));

// Set some tags on the image
var tags = {
  "Exif.Photo.UserComment" : "Some books..",
  "Exif.Canon.OwnerName" : "Damo's camera"
};
ex.setImageTags(dir + '/copy.jpg', tags, function(err){
  // Check our tags have been set
  ex.getImageTags(dir + '/copy.jpg', function(err, tags) {
    assert.equal("Some books..", tags["Exif.Photo.UserComment"]);
    assert.equal("Damo's camera", tags["Exif.Canon.OwnerName"]);
  });
});
