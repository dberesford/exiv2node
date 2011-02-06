var sys = require('sys');
var exiv2node = require('exiv2node');
var assert = require('assert');

var ex = new exiv2node.Exiv2Node();

/* Test basic image with exif tags */
ex.getImageTags('./testimages/books.jpg', function(tags) {
	assert.notEqual(null, tags);
	
	if (tags) {	
		console.log("All Tags:");
		for (key in tags){
			console.log(key + ":" + tags[key]);
		}
	
		console.log("\nDateTime: " + tags["Exif.Image.DateTime"]);
		console.log("DateTimeOriginal: " + tags["Exif.Photo.DateTimeOriginal"]);
		
		assert.equal("2008:12:16 21:28:36", tags["Exif.Image.DateTime"]);
		assert.equal("2008:12:16 21:28:36", tags["Exif.Photo.DateTimeOriginal"]);
	}
});

/* Test image with no tags */
ex.getImageTags('./testimages/damien.jpg', function(tags) {
	assert.equal(null, tags);
});