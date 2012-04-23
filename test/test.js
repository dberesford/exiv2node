var ex = require('exiv2');
var assert = require('assert');
var fs = require('fs')

var dir = __dirname + '/images';

// Test basic image with exif tags
ex.getImageTags(dir + '/books.jpg', function(err, tags) {
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


// Set image tags, first make a fresh copy of our test image
fs.writeFileSync(dir + '/copy.jpg', fs.readFileSync(dir + '/books.jpg'));

// Set some tags on the image
ex.setImageTags(dir + '/copy.jpg', { "Exif.Photo.UserComment" : "Some books..", "Exif.Canon.OwnerName" : "Damo's camera"}, function(err){
	assert.equal(null, err); 
	
	if (err) {
		// console.log(err);
	}else {
		// console.log("setImageTags complete..");
	}

	// Check our tags have been set
	ex.getImageTags(dir + '/copy.jpg', function(err, tags) {
		assert.equal("Some books..", tags["Exif.Photo.UserComment"]);
		assert.equal("Damo's camera", tags["Exif.Canon.OwnerName"]);
	});
});

// Test image with no tags
ex.getImageTags(dir + '/damien.jpg', function(err, tags) {
	assert.equal(null, tags);
});

// Test non existent files
ex.setImageTags('idontexist.jpg', { "Exif.Photo.UserComment" : "test"}, function(err){
	assert.notEqual(null, err);
	//console.log(err);
});
ex.getImageTags('idontexist.jpg', function(err, tags) {
	assert.notEqual(null, err);
	//console.log(err);
});

