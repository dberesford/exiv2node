#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>
#include <unistd.h>
#include <string>
#include <map>
#include <exception>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <exiv2/preview.hpp>

using namespace node;
using namespace v8;

// Create a map of strings for passing them back and forth between the V8 and
// worker threads.
typedef std::map<std::string, std::string> tag_map_t;

class Exiv2Worker : public Nan::AsyncWorker {
 public:
  Exiv2Worker(Nan::Callback *callback, const std::string fileName)
    : Nan::AsyncWorker(callback), fileName(fileName) {}
  ~Exiv2Worker() {}

 protected:
  const std::string fileName;
  std::string exifException;
};

// - - -

class GetTagsWorker : public Exiv2Worker {
 public:
  GetTagsWorker(Nan::Callback *callback, const std::string fileName)
    : Exiv2Worker(callback, fileName) {}
  ~GetTagsWorker() {}

  // Should become protected...
  tag_map_t tags;

  // Executed inside the worker-thread. Not safe to access V8, or V8 data
  // structures here.
  void Execute () {
    try {
      Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fileName);
      assert(image.get() != 0);
      image->readMetadata();

      Exiv2::ExifData &exifData = image->exifData();
      if (exifData.empty() == false) {
        Exiv2::ExifData::const_iterator end = exifData.end();
        for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
          tags.insert(std::pair<std::string, std::string> (i->key(), i->value().toString()));
        }
      }

      Exiv2::IptcData &iptcData = image->iptcData();
      if (iptcData.empty() == false) {
        Exiv2::IptcData::const_iterator end = iptcData.end();
        for (Exiv2::IptcData::const_iterator i = iptcData.begin(); i != end; ++i) {
          tags.insert(std::pair<std::string, std::string> (i->key(), i->value().toString()));
        }
      }

      Exiv2::XmpData &xmpData = image->xmpData();
      if (xmpData.empty() == false) {
        Exiv2::XmpData::const_iterator end = xmpData.end();
        for (Exiv2::XmpData::const_iterator i = xmpData.begin(); i != end; ++i) {
          tags.insert(std::pair<std::string, std::string> (i->key(), i->value().toString()));
        }
      }
    } catch (std::exception& e) {
      exifException.append(e.what());
    }
  }

  // This function will be run inside the main event loop so it is safe to use
  // V8 again.
  void HandleOKCallback () {
    Nan::HandleScope scope;

    Local<Value> argv[2] = { Nan::Null(), Nan::Null() };

    if (!exifException.empty()){
      argv[0] = Nan::New<String>(exifException.c_str()).ToLocalChecked();
    } else if (!tags.empty()) {
      Local<Object> hash = Nan::New<Object>();
      // Copy the tags out.
      for (tag_map_t::iterator i = tags.begin(); i != tags.end(); ++i) {
        hash->Set(
          Nan::New<String>(i->first.c_str()).ToLocalChecked(),
          Nan::New<String>(i->second.c_str()).ToLocalChecked()
        );
      }
      argv[1] = hash;
    }

    // Pass the argv array object to our callback function.
    callback->Call(2, argv);
  };
};

NAN_METHOD(GetImageTags) {
  Nan::HandleScope scope;

  /* Usage arguments */
  if (info.Length() <= 1 || !info[1]->IsFunction())
    return Nan::ThrowTypeError("Usage: <filename> <callback function>");

  Nan::Callback *callback = new Nan::Callback(info[1].As<Function>());
  std::string filename = std::string(*Nan::Utf8String(info[0]));

  Nan::AsyncQueueWorker(new GetTagsWorker(callback, filename));
  return;
}

// - - -

class SetTagsWorker : public Exiv2Worker {
 public:
  SetTagsWorker(Nan::Callback *callback, const std::string fileName)
    : Exiv2Worker(callback, fileName) {}
  ~SetTagsWorker() {}

  // Should become protected...
  tag_map_t tags;

  // Executed inside the worker-thread. Not safe to access V8, or V8 data
  // structures here.
  void Execute () {
    try {
      Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fileName);
      assert(image.get() != 0);

      image->readMetadata();
      Exiv2::ExifData &exifData = image->exifData();
      Exiv2::IptcData &iptcData = image->iptcData();
      Exiv2::XmpData &xmpData = image->xmpData();

      // Assign the tags.
      for (tag_map_t::iterator i = tags.begin(); i != tags.end(); ++i) {
        if (i->first.compare(0, 5, "Exif.") == 0) {
          exifData[i->first].setValue(i->second);
        } else if (i->first.compare(0, 5, "Iptc.") == 0) {
          iptcData[i->first].setValue(i->second);
        } else if (i->first.compare(0, 4, "Xmp.") == 0) {
          xmpData[i->first].setValue(i->second);
        } else {
          //std::cerr << "skipping unknown tag " << i->first << std::endl;
        }
      }

      // Write the tag data the image file.
      image->setExifData(exifData);
      image->setIptcData(iptcData);
      image->setXmpData(xmpData);
      image->writeMetadata();
    } catch (std::exception& e) {
      exifException.append(e.what());
    }
  }

  // This function will be run inside the main event loop so it is safe to use
  // V8 again.
  void HandleOKCallback () {
    Nan::HandleScope scope;

    // Create an argument array for any errors.
    Local<Value> argv[1] = { Nan::Null() };
    if (!exifException.empty()) {
      argv[0] = Nan::New<String>(exifException.c_str()).ToLocalChecked();
    }

    // Pass the argv array object to our callback function.
    callback->Call(1, argv);
  };
};

NAN_METHOD(SetImageTags) {
  Nan::HandleScope scope;

  /* Usage arguments */
  if (info.Length() <= 2 || !info[2]->IsFunction())
    return Nan::ThrowTypeError("Usage: <filename> <tags> <callback function>");

  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());
  std::string filename = std::string(*Nan::Utf8String(info[0]));

  SetTagsWorker *worker = new SetTagsWorker(callback, filename);

  Local<Object> tags = Local<Object>::Cast(info[1]);
  Local<Array> keys = Nan::GetOwnPropertyNames(tags).ToLocalChecked();
  for (unsigned i = 0; i < keys->Length(); i++) {
    Local<Value> key = keys->Get(i);
    worker->tags.insert(std::pair<std::string, std::string> (
      *Nan::Utf8String(key),
      *Nan::Utf8String(tags->Get(key))
    ));
  }

  Nan::AsyncQueueWorker(worker);
  return;
}

// - - -

class DeleteTagsWorker : public Exiv2Worker {
 public:
  DeleteTagsWorker(Nan::Callback *callback, const std::string fileName)
    : Exiv2Worker(callback, fileName) {}
  ~DeleteTagsWorker() {}

  // Should become protected...
  std::vector<std::string> tags;

  // Executed inside the worker-thread. Not safe to access V8, or V8 data
  // structures here.
  void Execute () {
    try {
      Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fileName);
      assert(image.get() != 0);

      image->readMetadata();
      Exiv2::ExifData &exifData = image->exifData();
      Exiv2::IptcData &iptcData = image->iptcData();
      Exiv2::XmpData &xmpData = image->xmpData();

      // Erase the tags.
      for (std::vector<std::string>::iterator i = tags.begin(); i != tags.end(); ++i) {
        if (i->compare(0, 5, "Exif.") == 0) {
          Exiv2::ExifKey *k = new Exiv2::ExifKey(*i);
          exifData.erase(exifData.findKey(*k));
          delete k;
        } else if (i->compare(0, 5, "Iptc.") == 0) {
          Exiv2::IptcKey *k = new Exiv2::IptcKey(*i);
          iptcData.erase(iptcData.findKey(*k));
          delete k;
        } else if (i->compare(0, 4, "Xmp.") == 0) {
          Exiv2::XmpKey *k = new Exiv2::XmpKey(*i);
          xmpData.erase(xmpData.findKey(*k));
          delete k;
        } else {
          //std::cerr << "skipping unknown tag " << i << std::endl;
        }
      }

      // Write the tag data the image file.
      image->setExifData(exifData);
      image->setIptcData(iptcData);
      image->setXmpData(xmpData);
      image->writeMetadata();
    } catch (std::exception& e) {
      exifException.append(e.what());
    }
  }

  // This function will be run inside the main event loop so it is safe to use
  // V8 again.
  void HandleOKCallback () {
    Nan::HandleScope scope;

    // Create an argument array for any errors.
    Local<Value> argv[1] = { Nan::Null() };
    if (!exifException.empty()) {
      argv[0] = Nan::New<String>(exifException.c_str()).ToLocalChecked();
    }

    // Pass the argv array object to our callback function.
    callback->Call(1, argv);
  };
};

NAN_METHOD(DeleteImageTags) {
  Nan::HandleScope scope;

  /* Usage arguments */
  if (info.Length() <= 2 || !info[2]->IsFunction())
    return Nan::ThrowTypeError("Usage: <filename> <tags> <callback function>");

  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());
  std::string filename = std::string(*Nan::Utf8String(info[0]));

  DeleteTagsWorker *worker = new DeleteTagsWorker(callback, filename);

  Local<Array> keys = Local<Array>::Cast(info[1]);
  for (unsigned i = 0; i < keys->Length(); i++) {
    Local<v8::Value> key = keys->Get(i);
    worker->tags.push_back(*Nan::Utf8String(key));
  }

  Nan::AsyncQueueWorker(worker);
  return;
}

// - - -

class GetPreviewsWorker : public Exiv2Worker {
 public:
  GetPreviewsWorker(Nan::Callback *callback, const std::string fileName)
    : Exiv2Worker(callback, fileName) {}
  ~GetPreviewsWorker() {}

  // Executed inside the worker-thread. Not safe to access V8, or V8 data
  // structures here.
  void Execute () {
    try {
      Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fileName);
      assert(image.get() != 0);
      image->readMetadata();

      Exiv2::PreviewManager manager(*image);
      Exiv2::PreviewPropertiesList list = manager.getPreviewProperties();

      // Make sure we're not reusing a baton with memory allocated.
      for (Exiv2::PreviewPropertiesList::iterator pos = list.begin(); pos != list.end(); pos++) {
        Exiv2::PreviewImage image = manager.getPreviewImage(*pos);

        previews.push_back(Preview(pos->mimeType_, pos->height_,
          pos->width_, (char*) image.pData(), pos->size_));
      }
    } catch (std::exception& e) {
      exifException.append(e.what());
    }
  }

  // This function will be run inside the main event loop so it is safe to use
  // V8 again.
  void HandleOKCallback () {
    Nan::HandleScope scope;

    Local<Value> argv[2] = { Nan::Null(), Nan::Null() };
    if (!exifException.empty()){
      argv[0] = Nan::New<String>(exifException.c_str()).ToLocalChecked();
    } else {
      // Convert the data into V8 values.
      Local<Array> array = Nan::New<Array>(previews.size());
      for (size_t i = 0; i < previews.size(); ++i) {
        Local<Object> preview = Nan::New<Object>();
        preview->Set(Nan::New<String>("mimeType").ToLocalChecked(), Nan::New<String>(previews[i].mimeType.c_str()).ToLocalChecked());
        preview->Set(Nan::New<String>("height").ToLocalChecked(), Nan::New<Number>(previews[i].height));
        preview->Set(Nan::New<String>("width").ToLocalChecked(), Nan::New<Number>(previews[i].width));
        preview->Set(Nan::New<String>("data").ToLocalChecked(), Nan::CopyBuffer(previews[i].data, previews[i].size).ToLocalChecked());

        array->Set(i, preview);
      }
      argv[1] = array;
    }

    // Pass the argv array object to our callback function.
    callback->Call(2, argv);
  };

 protected:

  // Holds a preview image when we copy it from the worker to V8.
  struct Preview {
    Preview(const Preview &p)
      : mimeType(p.mimeType), height(p.height), width(p.width), size(p.size)
    {
      data = new char[size];
      memcpy(data, p.data, size);
    }
    Preview(std::string type_, uint32_t height_, uint32_t width_, const char *data_, size_t size_)
      : mimeType(type_), height(height_), width(width_), size(size_)
    {
      data = new char[size];
      memcpy(data, data_, size);
    }
    ~Preview() {
      delete[] data;
    }

    std::string mimeType;
    uint32_t height;
    uint32_t width;
    size_t size;
    char* data;
  };

  std::vector<Preview> previews;
};

NAN_METHOD(GetImagePreviews) {
  Nan::HandleScope scope;

  /* Usage arguments */
  if (info.Length() <= 1 || !info[1]->IsFunction())
    return Nan::ThrowTypeError("Usage: <filename> <callback function>");

  Nan::Callback *callback = new Nan::Callback(info[1].As<Function>());
  std::string filename = std::string(*Nan::Utf8String(info[0]));

  Nan::AsyncQueueWorker(new GetPreviewsWorker(callback, filename));
  return;
}

// - - -

void InitAll(Handle<Object> target) {
  target->Set(Nan::New<String>("getImageTags").ToLocalChecked(), Nan::New<FunctionTemplate>(GetImageTags)->GetFunction());
  target->Set(Nan::New<String>("setImageTags").ToLocalChecked(), Nan::New<FunctionTemplate>(SetImageTags)->GetFunction());
  target->Set(Nan::New<String>("deleteImageTags").ToLocalChecked(), Nan::New<FunctionTemplate>(DeleteImageTags)->GetFunction());
  target->Set(Nan::New<String>("getImagePreviews").ToLocalChecked(), Nan::New<FunctionTemplate>(GetImagePreviews)->GetFunction());
}
NODE_MODULE(exiv2, InitAll)
