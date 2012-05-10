var ex = require('../exiv2');
var assert = require('assert');
var fs = require('fs')

var dir = __dirname + '/../test/images';

// Test basic image with exif tags:
ex.getImageTags(dir + '/books.jpg', function(err, tags) {
  console.log(tags);
});

// Make a copy of our file so we don't polute the original.
fs.writeFileSync(dir + '/copy.jpg', fs.readFileSync(dir + '/books.jpg'));

// Set some tags on the image:
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

// Load the preview images:
ex.getImagePreviews(dir + '/books.jpg', function(err, previews) {
  // Display information about the previews.
  console.log("Preview images:");
  previews.forEach(function (p, index) {
    console.log("%d: %s %dx%dpx %d bytes", index, p.mimeType, p.height, p.width, p.data.length);
  });
  // Or you can save them--though you'll probably want to check the MIME type
  // before picking an extension.
  fs.writeFile('preview.jpg', previews[0].data);
});
