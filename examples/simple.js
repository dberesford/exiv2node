var ex = require('../exiv2');
var assert = require('assert');
var fs = require('fs')

var dir = __dirname + '/../test/images';

// Example of get image exif tags:
ex.getImageTags(dir + '/books.jpg', function(err, tags) {
  console.log(tags);
});

var image = dir + '/copy.jpg';

// Make a copy of our file so we don't polute the original.
fs.writeFileSync(image, fs.readFileSync(dir + '/books.jpg'));

// Set some tags on the image:
var SOMEBOOKS = "Some books..";
var MYCAMERA = "My Camera";

var newtags = {
  "Exif.Photo.UserComment" : SOMEBOOKS,
  "Exif.Canon.OwnerName" : MYCAMERA
};

ex.setImageTags(image, newtags, function(err){
  assert.ok(!err);

  // Check our tags have been set
  ex.getImageTags(image, function(err, tags) {
    assert.ok(!err);
    assert.equal(SOMEBOOKS, tags["Exif.Photo.UserComment"]);
    assert.equal(MYCAMERA, tags["Exif.Canon.OwnerName"]);

    // delete image tags
    var delTags = ["Exif.Photo.UserComment", "Exif.Canon.OwnerName"];
    ex.deleteImageTags(image, delTags, function(err) {
      assert.ok(!err);
      ex.getImageTags(image, function(err, tags) {
        assert.ok(!err);
        assert.ok(!tags["Exif.Canon.OwnerName"]);
        assert.ok(!tags["Exif.Photo.UserComment"]);
      });
    });
  });
});

// Example of loading the preview images:
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
