var sys = require('sys');
var exiv2node = require('exiv2node');
var assert = require('assert');
var fs = require('fs')

var ex = new exiv2node.Exiv2Node();

/* Test basic image with exif tags */
ex.getImageTags('./testimages/books.jpg', function(err, tags) {
	assert.equal(null, err);
	assert.notEqual(null, tags);
	
	if (tags) {	
		//console.log("All Tags:");
		//for (key in tags){
		//	console.log(key + ":" + tags[key]);
		//}
		assert.equal("2008:12:16 21:28:36", tags["Exif.Image.DateTime"]);
		assert.equal("2008:12:16 21:28:36", tags["Exif.Photo.DateTimeOriginal"]);
	}
});


/* Set image tags, first make a fresh copy of our test image */
fs.writeFileSync('./testimages/copy.jpg', fs.readFileSync('./testimages/books.jpg'));

/* Set some tags on the image */
ex.setImageTags('./testimages/copy.jpg', { "Exif.Photo.UserComment" : "Some books..", "Exif.Canon.OwnerName" : "Damo's camera"}, function(err){
	assert.equal(null, err); 
	
	if (err) {
		// console.log(err);
	}else {
		// console.log("setImageTags complete..");
	}

	/* Check our tags have been set */
	ex.getImageTags('./testimages/copy.jpg', function(err, tags) {
		assert.equal("Some books..", tags["Exif.Photo.UserComment"]);
		assert.equal("Damo's camera", tags["Exif.Canon.OwnerName"]);
	});
});

/* Test image with no tags */
ex.getImageTags('./testimages/damien.jpg', function(err, tags) {	
	assert.equal(null, tags);
});

/* Test non existent files */
ex.setImageTags('./testimages/idontexist.jpg', { "Exif.Photo.UserComment" : "test"}, function(err){
	assert.notEqual(null, err);
	//console.log(err);
});

ex.getImageTags('./testimages/idontexist.jpg', function(err, tags) {	
	assert.notEqual(null, err);
	//console.log(err);
});

