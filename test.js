var sys = require('sys');
var exiv2node = require('exiv2node');
var assert = require('assert');
var fs = require('fs')

var ex = new exiv2node.Exiv2Node();

/* First make a fresh copy of our test image */
fs.writeFileSync('./testimages/copy.jpg', fs.readFileSync('./testimages/books.jpg'));

/* Set some tags on the image */
ex.setImageTags('./testimages/copy.jpg', { "Exif.Photo.UserComment" : "Some books..", "Exif.Canon.OwnerName" : "Damian Beresford"}, function(){
	console.log("setImageTags complete");
});

/* Test basic image with exif tags */
ex.getImageTags('./testimages/books.jpg', function(tags) {
	assert.notEqual(null, tags);
	
	if (tags) {	
		//console.log("All Tags:");
		//for (key in tags){
			//console.log(key + ":" + tags[key]);
		//}
		assert.equal("2008:12:16 21:28:36", tags["Exif.Image.DateTime"]);
		assert.equal("2008:12:16 21:28:36", tags["Exif.Photo.DateTimeOriginal"]);
	}
});

/* Test image with no tags */
ex.getImageTags('./testimages/damien.jpg', function(tags) {
	assert.equal(null, tags);
});