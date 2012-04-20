#include <v8.h>
#include <node.h>
#include <unistd.h>
#include <string>
#include <map>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>

using namespace node;
using namespace v8;

#define DEBUGMODE 1
#define debug_printf(...) do {if(DEBUGMODE)printf(__VA_ARGS__);} while (0)

class Exiv2Node: ObjectWrap {
public:
  static Persistent<FunctionTemplate> s_ct;
  static void Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("Exiv2Node"));

    NODE_SET_PROTOTYPE_METHOD(s_ct, "getImageTags", GetImageTags);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "setImageTags", SetImageTags);

    target->Set(String::NewSymbol("Exiv2Node"), s_ct->GetFunction());
  }

  Exiv2Node() {
  }

  ~Exiv2Node() {
  }

  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;
    Exiv2Node* exiv2node = new Exiv2Node();
    exiv2node->Wrap(args.This());
    return args.This();
  }

  /* structure for passing our various objects around libuv */
  typedef std::map<std::string, std::string> tag_map_t;
  struct exiv2node_thread_data_t {
    uv_work_t request;
    Exiv2Node *exiv2node;
    Exiv2::Image::AutoPtr image;
    Persistent<Function> cb;
    std::string fileName;
    tag_map_t *tags;
    std::string exifException;

    exiv2node_thread_data_t(Exiv2Node* en_, Local<String> fn_, Handle<Function> cb_) : exiv2node(en_) {
      uv_ref(uv_default_loop());
      request.data = this;
      exiv2node->Ref();
      cb = Persistent<Function>::New(cb_);
      fileName = std::string(*String::AsciiValue(fn_));
      tags = new tag_map_t();
      exifException = std::string();
    }
    virtual ~exiv2node_thread_data_t() {
      uv_unref(uv_default_loop());
      exiv2node->Unref();
      cb.Dispose();
      delete tags;
      // Assuming std::auto_ptr does it's job here and Exiv2::Image::AutoPtr is
      // destroyed when it goes out of scope here..
    }
  };

  static Handle<Value> GetImageTags(const Arguments& args) {
    HandleScope scope;

    /* Usage arguments */
    if (args.Length() <= (1) || !args[1]->IsFunction())
      return ThrowException(Exception::TypeError(String::New("Usage: <filename> <callback function>")));

    Local<String> fileName = Local<String>::Cast(args[0]);
    Local<Function> cb = Local<Function>::Cast(args[1]);

    Exiv2Node* exiv2node = ObjectWrap::Unwrap<Exiv2Node>(args.This());

    /* Set up our thread data struct, pass off to the libuv thread pool */
    exiv2node_thread_data_t *thread_data = new exiv2node_thread_data_t(exiv2node, fileName, cb);

    int status = uv_queue_work(uv_default_loop(), &thread_data->request, GetImageTagsWorker, AfterGetImageTags);
    assert(status == 0);

    return Undefined();
  }

  static void GetImageTagsWorker(uv_work_t* req) {
    exiv2node_thread_data_t *thread_data = static_cast<exiv2node_thread_data_t *> (req->data);

    /* Exiv2 processing of the file.. */
    try {
      thread_data->image = Exiv2::ImageFactory::open(thread_data->fileName);
      thread_data->image->readMetadata();
    } catch (Exiv2::AnyError& e) {
      thread_data->exifException.append(e.what());
    }
  }

  /* Thread complete callback.. */
  static void AfterGetImageTags(uv_work_t* req) {
    HandleScope scope;
    exiv2node_thread_data_t *thread_data = static_cast<exiv2node_thread_data_t *> (req->data);

    Local<Value> argv[2];
    if (!thread_data->exifException.empty()){
      argv[0] = Local<String>::New(String::New(thread_data->exifException.c_str()));
      argv[1] = Local<Value>::New(Null());
    } else {
      argv[0] = Local<Value>::New(Null());

      /* Create a V8 array with all the image tags and their corresponding values */
      Exiv2::ExifData &exifData = thread_data->image->exifData();
      if (exifData.empty() == false) {
        Local<Array> tags = Array::New();
        Exiv2::ExifData::const_iterator end = exifData.end();
        for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
          tags->Set(String::New(i->key().c_str()), String::New(i->value().toString().c_str()));
        }
        argv[1] = tags;
      } else {
        argv[1] = Local<Value>::New(Null());
      }
    }

    /* Pass the argv array object to our callback function */
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

    Local<String> fileName = Local<String>::Cast(args[0]);
    Local<Object> tags = Local<Object>::Cast(args[1]);
    Local<Function> cb = Local<Function>::Cast(args[2]);

    Exiv2Node* exiv2node = ObjectWrap::Unwrap<Exiv2Node>(args.This());

    /* Set up our thread data struct, pass off to the libuv thread pool */
    exiv2node_thread_data_t *thread_data = new exiv2node_thread_data_t(exiv2node, fileName, cb);

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
    exiv2node_thread_data_t *thread_data = static_cast<exiv2node_thread_data_t*> (req->data);

    /* Read existing metadata.. TODO: also handle IPTC and XMP data here.. */
    try {
      thread_data->image = Exiv2::ImageFactory::open(thread_data->fileName);
      thread_data->image->readMetadata();
      Exiv2::ExifData &exifData = thread_data->image->exifData();

      // Assign the tags.
      for (tag_map_t::iterator i = thread_data->tags->begin(); i != thread_data->tags->end(); ++i) {
        exifData[i->first].setValue(i->second);
      }

      /* Write the Exif data to the image file */
      thread_data->image->setExifData(exifData);
      thread_data->image->writeMetadata();
    } catch (Exiv2::AnyError& e) {
      thread_data->exifException.append(e.what());
    }
  }

  /* Thread complete callback.. */
  static void AfterSetImageTags(uv_work_t *req) {
    HandleScope scope;
    exiv2node_thread_data_t *thread_data = static_cast<exiv2node_thread_data_t*> (req->data);

    Local<Value> argv[1];

    /* Create a V8 array with all the image tags and their corresponding values */
    if (thread_data->exifException.empty()) {
      argv[0] = Local<Value>::New(Null());
    } else {
      argv[0] = Local<String>::New(String::New(thread_data->exifException.c_str()));
    }

    /* Pass the argv array object to our callback function */
    TryCatch try_catch;
    thread_data->cb->Call(Context::GetCurrent()->Global(), 1, argv);
    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    delete thread_data;
  }

};

Persistent<FunctionTemplate> Exiv2Node::s_ct;

extern "C" {
  static void init(Handle<Object> target) {
    Exiv2Node::Init(target);
  }
  NODE_MODULE(exiv2node, init);
}
