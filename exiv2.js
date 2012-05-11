var exiv2 = require('./build/Release/exiv2.node')

// Export the properties implemented in the extension.
exports = module.exports = exiv2;

/**
 * Convert exiv2's date-time strings into Date instances.
 *
 * The first parameter is the hash of tags loaded by getImageTags().
 *
 * The second parameter is an optional tag name. The default is
 * 'Exif.Photo.DateTimeOriginal' with sub-second data being loaded from
 * 'Exif.Photo.SubSecTimeOriginal', if it exists. See http://exiv2.org/tags.html
 * for a complete list of tags.
 */
exports.getDate = function getDate(tags, dateTimeTag) {
  dateTimeTag = dateTimeTag || 'Exif.Photo.DateTimeOriginal';
  if (tags[dateTimeTag] === undefined) {
    return null;
  }

  // Date.parse() doesn't recognize the custom format ('2012:04:14 17:45:52') so
  // the easiest thing to do is convert it to ISO 8601 (e.g.
  // "2012-04-14T17:45:52") which it does recognize. The colons in the date need
  // to become hyphens and the space needs to become a 'T'.

  // So split it into two...
  var datetime = tags[dateTimeTag].split(' ');
  // ...then change the colons in date into hyphens...
  datetime[0] = datetime[0].replace(/:/g, '-');
  // ...then join it back together with a 'T' and parse it.
  var d = new Date(Date.parse(datetime.join('T')));

  // If there's a matching sub second tag scale it from hundredths of a second
  // to thousands and apply it to the date. The extra precision can be useful
  // when sorting photos by time because multiple photos could be taken in a
  // single second.
  var subSecTag = dateTimeTag.replace(/\.DateTime/, '.SubSecTime');
  if (tags[subSecTag] !== undefined) {
    d.setMilliseconds(tags[subSecTag] * 10);
  }

  return d;
}
