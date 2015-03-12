
var exiv = require('../exiv2')
  , fs = require('fs')
  , util = require('util')
  , should = require('should')
  , dir = __dirname + '/images';

describe('exiv2', function(){
  describe('.getImageTags()', function(){
    it("should callback with image's tags", function(done) {
      exiv.getImageTags(dir + '/books.jpg', function(err, tags) {
        should.not.exist(err);

        tags.should.have.property('Exif.Image.DateTime', '2008:12:16 21:28:36');
        tags.should.have.property('Exif.Photo.DateTimeOriginal', '2008:12:16 21:28:36');
        done();
      })
    });

    it('should callback with null on untagged file', function(done) {
      exiv.getImageTags(dir + '/damien.jpg', function(err, tags) {
        should.not.exist(err);
        should.not.exist(tags);
        done();
      })
    });

    it('should throw if no file path is provided', function() {
      (function(){
        exiv.getImageTags()
      }).should.throw();
    });

    it('should throw if no callback is provided', function() {
      (function(){
        exiv.getImageTags(dir + '/books.jpg')
      }).should.throw();
    });

    it('should report an error on an invalid path', function(done) {
      exiv.getImageTags('idontexist.jpg', function(err, tags) {
        should.exist(err);
        should.not.exist(tags);
        done();
      });
    });
  });

  describe('.setImageTags()', function(){
    var temp = dir + '/copy.jpg';

    before(function() {
      fs.writeFileSync(temp, fs.readFileSync(dir + '/books.jpg'));
    });
    it('should write tags to image files', function(done) {
      var tags = {
        "Exif.Photo.UserComment" : "Some books..",
        "Exif.Canon.OwnerName" : "Sørens kamera",
        "Iptc.Application2.RecordVersion" : "2",
        "Xmp.dc.subject" : "A camera"
      };
      exiv.setImageTags(temp, tags, function(err){
        should.not.exist(err);

        exiv.getImageTags(temp, function(err, tags) {
          tags.should.have.property('Exif.Photo.UserComment', "Some books..");
          tags.should.have.property('Exif.Canon.OwnerName', "Sørens kamera");
          tags.should.have.property('Iptc.Application2.RecordVersion', "2");
          tags.should.have.property('Xmp.dc.subject', "A camera");
          done();
        });
      });
    })
    after(function(done) {
      fs.unlink(temp, done);
    });

    it('should throw if no file path is provided', function() {
      (function(){
        exiv.setImageTags()
      }).should.throw();
    });

    it('should throw if no callback is provided', function() {
      (function(){
        exiv.setImageTags(dir + '/books.jpg')
      }).should.throw();
    });

    it('should report an error on an invalid path', function(done) {
      exiv.setImageTags('idontexist.jpg', {}, function(err, tags) {
        should.exist(err);
        should.not.exist(tags);
        done();
      });
    });
  });

  describe('.deleteImageTags()', function(){
    var temp = dir + '/copy-deltags.jpg';

    before(function() {
      fs.writeFileSync(temp, fs.readFileSync(dir + '/books.jpg'));
    });
    it('should delete tags in image files', function(done) {
      var tags = ["Exif.Canon.OwnerName"];
      exiv.deleteImageTags(temp, tags, function(err){
        should.not.exist(err);

        exiv.getImageTags(temp, function(err, tags) {
          should.not.exist(err);
          tags.should.not.have.property('Exif.Canon.OwnerName');
          done();
        });
      });
    })
    after(function(done) {
      fs.unlink(temp, done);
    });
  });

  describe('.getImagePreviews()', function(){
    it("should callback with image's previews", function(done) {
      exiv.getImagePreviews(dir + '/books.jpg', function(err, previews) {
        should.not.exist(err);
        previews.should.be.an.instanceof(Array);
        previews.should.have.lengthOf(1);
        previews[0].should.have.property('mimeType', 'image/jpeg');
        previews[0].should.have.property('height', 120);
        previews[0].should.have.property('width', 160);
        previews[0].should.have.property('data').with.instanceof(Buffer);
        previews[0].data.should.have.property('length', 6071);
        done();
      });
    });

    it('should callback with an empty array for files no previews', function(done) {
      exiv.getImagePreviews(dir + '/damien.jpg', function(err, previews) {
        should.not.exist(err);
        previews.should.be.an.instanceof(Array);
        previews.should.have.lengthOf(0);
        done();
      })
    });


    it('should throw if no file path is provided', function() {
      (function(){
        exiv.getImagePreviews()
      }).should.throw();
    });

    it('should throw if no callback is provided', function() {
      (function(){
        exiv.getImagePreviews(dir + '/books.jpg')
      }).should.throw();
    });

    it('should report an error on an invalid path', function(done) {
      exiv.getImagePreviews('idontexist.jpg', function(err, previews) {
        should.exist(err);
        should.not.exist(previews);
        done();
      });
    });
  });

  describe('.getDate()', function() {
    var tags = {'Exif.Photo.DateTimeOriginal': '2012:04:14 17:45:52'};

    it("should return a false value if the tag doesn't exist", function() {
      var d = exiv.getDate({});
      should.not.exist(d);
    });

    it("should return a Date", function() {
      var d = exiv.getDate(tags);
      should.ok(util.isDate(d));
    });

    it("should be the correct date and time", function() {
      var d = exiv.getDate(tags);
      d.toISOString().should.equal('2012-04-14T17:45:52.000Z');
    });

    it("should include milliseconds if available", function() {
      tags['Exif.Photo.SubSecTimeOriginal'] = '99';
      var d = exiv.getDate(tags);
      d.toISOString().should.equal('2012-04-14T17:45:52.990Z');
    });

    it("should use custom tags", function() {
      tags['Exif.Photo.DateTimeDigitized'] = '2012:04:14 17:08:08';
      tags['Exif.Photo.SubSecTimeDigitized'] = '88';
      var d = exiv.getDate(tags, 'Exif.Photo.DateTimeDigitized');
      d.toISOString().should.equal('2012-04-14T17:08:08.880Z');
    });
  })
})
