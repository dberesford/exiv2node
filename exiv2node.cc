#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>
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
uv_async_t g_async;

// Base structure for passing data to and from our libuv workers.
struct Baton {
  uv_work_t request;
  Persistent<Function> cb;
  std::string fileName;
  std::string exifException;
  tag_map_t *tags;

  Baton(Local<String> fn_, Handle<Function> cb_) {
#if NODE_VERSION_AT_LEAST(0, 7, 9)
    uv_ref((uv_handle_t *) & g_async);
#else
    uv_ref(uv_default_loop());
#endif
    request.data = this;
    cb = Persistent<Function>::New(cb_);
    fileName = std::string(*String::AsciiValue(fn_));
    exifException = std::string();
    tags = new tag_map_t();
  }
  virtual ~Baton() {
#if NODE_VERSION_AT_LEAST(0, 7, 9)
    uv_unref((uv_handle_t *) & g_async);
#else
     uv_unref(uv_default_loop());
#endif
    cb.Dispose();
    delete tags;
  }
};

// Holds a preview image when we copy it from the worker to V8.
struct Preview {
  std::string mimeType;
  uint32_t height;
  uint32_t width;
  char* data;
  size_t size;

  Preview(std::string type_, uint32_t height_, uint32_t width_, const char *data_, size_t size_)
    : mimeType(type_), height(height_), width(width_), size(size_)
  {
    data = new char[size_];
    memcpy(data, data_, size_);
  }
  virtual ~Preview() {
    delete[] data;
  }
};

struct GetPreviewBaton : Baton {
  Preview **previews;
  size_t size;

  GetPreviewBaton(Local<String> fn_, Handle<Function> cb_)
    : Baton(fn_, cb_), previews(0), size(0)
  {}
  virtual ~GetPreviewBaton() {
    for (size_t i = 0; i < size; ++i) {
      delete previews[i];
    }
    delete previews;
  }
};

/**
 * Create a Buffer object from raw data.
 *
 * Based on code from: http://sambro.is-super-awesome.com/tag/slowbuffer/
 */
static Local<Object> makeBuffer(char* data, size_t size) {
  HandleScope scope;

  // It ends up being kind of a pain to convert a slow buffer into a fast
  // one since the fast part is implemented in JavaScript.
  Local<Buffer> slowBuffer = Buffer::New(data, size);
  // First get the Buffer from global scope...
  Local<Object> global = Context::GetCurrent()->Global();
  Local<Value> bv = global->Get(String::NewSymbol("Buffer"));
  assert(bv->IsFunction());
  Local<Function> b = Local<Function>::Cast(bv);
  // ...call Buffer() with the slow buffer, its size and the starting offset...
  Handle<Value> argv[3] = { slowBuffer->handle_, Integer::New(size), Integer::New(0) };
  Local<Object> fastBuffer = b->NewInstance(3, argv);

  // ...and get a fast buffer that we can return.
  return scope.Close(fastBuffer);
}

static void GetImageTagsWorker(uv_work_t* req);
static void AfterGetImageTags(uv_work_t* req, int status);
static void SetImageTagsWorker(uv_work_t *req);
static void AfterSetImageTags(uv_work_t* req, int status);
static void GetImagePreviewsWorker(uv_work_t* req);
static void AfterGetImagePreviews(uv_work_t* req, int status);

static Handle<Value> GetImageTags(const Arguments& args) {
  HandleScope scope;

  /* Usage arguments */
  if (args.Length() <= (1) || !args[1]->IsFunction())
    return ThrowException(Exception::TypeError(String::New("Usage: <filename> <callback function>")));

  // Set up our thread data struct, pass off to the libuv thread pool.
  Baton *thread_data = new Baton(Local<String>::Cast(args[0]), Local<Function>::Cast(args[1]));

  int status = uv_queue_work(uv_default_loop(), &thread_data->request, GetImageTagsWorker, AfterGetImageTags);
  assert(status == 0);

  return Undefined();
}

static void GetImageTagsWorker(uv_work_t* req) {
  Baton *thread_data = static_cast<Baton *> (req->data);

  try {
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(thread_data->fileName);
    assert(image.get() != 0);
    image->readMetadata();

    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty() == false) {
      Exiv2::ExifData::const_iterator end = exifData.end();
      for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
        thread_data->tags->insert(std::pair<std::string, std::string> (i->key(), i->value().toString()));
      }
    }

    Exiv2::IptcData &iptcData = image->iptcData();
    if (iptcData.empty() == false) {
      Exiv2::IptcData::const_iterator end = iptcData.end();
      for (Exiv2::IptcData::const_iterator i = iptcData.begin(); i != end; ++i) {
        thread_data->tags->insert(std::pair<std::string, std::string> (i->key(), i->value().toString()));
      }
    }

    Exiv2::XmpData &xmpData = image->xmpData();
    if (xmpData.empty() == false) {
      Exiv2::XmpData::const_iterator end = xmpData.end();
      for (Exiv2::XmpData::const_iterator i = xmpData.begin(); i != end; ++i) {
        thread_data->tags->insert(std::pair<std::string, std::string> (i->key(), i->value().toString()));
      }
    }
  } catch (std::exception& e) {
    thread_data->exifException.append(e.what());
  }
}

/* Thread complete callback.. */
static void AfterGetImageTags(uv_work_t* req, int status) {
  HandleScope scope;
  Baton *thread_data = static_cast<Baton *> (req->data);

  Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(Null()) };
  if (!thread_data->exifException.empty()){
    argv[0] = Local<String>::New(String::New(thread_data->exifException.c_str()));
  } else if (!thread_data->tags->empty()) {
    Local<Object> tags = Object::New();
    // Copy the tags out.
    for (tag_map_t::iterator i = thread_data->tags->begin(); i != thread_data->tags->end(); ++i) {
      tags->Set(String::New(i->first.c_str()), String::New(i->second.c_str()), ReadOnly);
    }
    argv[1] = tags;
  }

  // Pass the argv array object to our callback function.
  TryCatch try_catch;
  thread_data->cb->Call(Context::GetCurrent()->Global(), 2, argv);
  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  delete thread_data;
}

/* Set Image Tag support.. */
static Handle<Value> SetImageTags(const Arguments& args) {
  HandleScope scope;

  /* Usage arguments */
  if (args.Length() <= 2 || !args[2]->IsFunction())
    return ThrowException(Exception::TypeError(String::New("Usage: <filename> <tags> <callback function>")));

  // Set up our thread data struct, pass off to the libuv thread pool.
  Baton *thread_data = new Baton(Local<String>::Cast(args[0]), Local<Function>::Cast(args[2]));

  Local<Object> tags = Local<Object>::Cast(args[1]);
  Local<Array> keys = tags->GetPropertyNames();
  for (unsigned i = 0; i < keys->Length(); i++) {
    Handle<v8::Value> key = keys->Get(i);
    thread_data->tags->insert(std::pair<std::string, std::string> (
      *String::AsciiValue(key),
      *String::AsciiValue(tags->Get(key)))
    );
  }

  int status = uv_queue_work(uv_default_loop(), &thread_data->request, SetImageTagsWorker, AfterSetImageTags);
  assert(status == 0);

  return Undefined();
}

static void SetImageTagsWorker(uv_work_t *req) {
  Baton *thread_data = static_cast<Baton*> (req->data);

  try {
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(thread_data->fileName);
    assert(image.get() != 0);

    image->readMetadata();
    Exiv2::ExifData &exifData = image->exifData();
    Exiv2::IptcData &iptcData = image->iptcData();
    Exiv2::XmpData &xmpData = image->xmpData();

    // Assign the tags.
    for (tag_map_t::iterator i = thread_data->tags->begin(); i != thread_data->tags->end(); ++i) {
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
    thread_data->exifException.append(e.what());
  }
}

/* Thread complete callback.. */
static void AfterSetImageTags(uv_work_t* req, int status) {
  HandleScope scope;
  Baton *thread_data = static_cast<Baton*> (req->data);

  // Create an argument array for any errors.
  Local<Value> argv[1] = { Local<Value>::New(Null()) };
  if (!thread_data->exifException.empty()) {
    argv[0] = Local<String>::New(String::New(thread_data->exifException.c_str()));
  }

  // Pass the argv array object to our callback function.
  TryCatch try_catch;
  thread_data->cb->Call(Context::GetCurrent()->Global(), 1, argv);
  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  delete thread_data;
}


// Fetch preview images
static Handle<Value> GetImagePreviews(const Arguments& args) {
  HandleScope scope;

  // Usage arguments
  if (args.Length() <= (1) || !args[1]->IsFunction())
    return ThrowException(Exception::TypeError(String::New("Usage: <filename> <callback function>")));

  // Set up our thread data struct, pass off to the libuv thread pool.
  GetPreviewBaton *thread_data = new GetPreviewBaton(Local<String>::Cast(args[0]), Local<Function>::Cast(args[1]));

  int status = uv_queue_work(uv_default_loop(), &thread_data->request, GetImagePreviewsWorker, AfterGetImagePreviews);
  assert(status == 0);

  return Undefined();
}

// Extract the previews and copy them into the baton.
static void GetImagePreviewsWorker(uv_work_t *req) {
  GetPreviewBaton *thread_data = static_cast<GetPreviewBaton*> (req->data);

  try {
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(thread_data->fileName);
    assert(image.get() != 0);
    image->readMetadata();

    Exiv2::PreviewManager manager(*image);
    Exiv2::PreviewPropertiesList list = manager.getPreviewProperties();

    // Make sure we're not reusing a baton with memory allocated.
    assert(thread_data->previews == 0);
    assert(thread_data->size == 0);

    thread_data->size = list.size();
    thread_data->previews = new Preview*[thread_data->size];

    int i = 0;
    for (Exiv2::PreviewPropertiesList::iterator pos = list.begin(); pos != list.end(); pos++) {
      Exiv2::PreviewImage image = manager.getPreviewImage(*pos);

      thread_data->previews[i++] = new Preview(pos->mimeType_, pos->height_,
        pos->width_, (char*) image.pData(), pos->size_);
    }
  } catch (std::exception& e) {
    thread_data->exifException.append(e.what());
  }
}

// Convert the previews from the baton into V8 objects and fire the callback.
static void AfterGetImagePreviews(uv_work_t* req, int status) {
  HandleScope scope;
  GetPreviewBaton *thread_data = static_cast<GetPreviewBaton*> (req->data);

  Local<Value> argv[2] = { Local<Value>::New(Null()), Local<Value>::New(Null()) };
  if (!thread_data->exifException.empty()){
    argv[0] = Local<String>::New(String::New(thread_data->exifException.c_str()));
  } else {
    // Convert the data into V8 values.
    Local<Array> previews = Array::New(thread_data->size);
    for (size_t i = 0; i < thread_data->size; ++i) {
      Preview *p = thread_data->previews[i];

      Local<Object> preview = Object::New();
      preview->Set(String::New("mimeType"), String::New(p->mimeType.c_str()));
      preview->Set(String::New("height"), Number::New(p->height));
      preview->Set(String::New("width"), Number::New(p->width));
      preview->Set(String::New("data"), makeBuffer(p->data, p->size));

      previews->Set(i, preview);
    }
    argv[1] = previews;
  }

  // Pass the argv array object to our callback function.
  TryCatch try_catch;
  thread_data->cb->Call(Context::GetCurrent()->Global(), 2, argv);
  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  delete thread_data;
}

void InitAll(Handle<Object> target) {
  target->Set(String::NewSymbol("getImageTags"), FunctionTemplate::New(GetImageTags)->GetFunction());
  target->Set(String::NewSymbol("setImageTags"), FunctionTemplate::New(SetImageTags)->GetFunction());
  target->Set(String::NewSymbol("getImagePreviews"), FunctionTemplate::New(GetImagePreviews)->GetFunction());
}
NODE_MODULE(exiv2, InitAll)
