/* This code is PUBLIC DOMAIN, and is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND. See the accompanying 
 * LICENSE file.
 */

#include <v8.h>
#include <node.h>
#include <unistd.h>
#include <string>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>

using namespace std;
using namespace node;
using namespace v8;

static const std::string EXIF_IMAGE_DATETIME = "Exif.Image.DateTime";
static const std::string EXIF_PHOTO_DATEIMTEORIGINAL = "Exif.Photo.DateTimeOriginal";

class Exiv2Node: ObjectWrap
{
private:
  int m_count;
public:

  static Persistent<FunctionTemplate> s_ct;
  static void Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("Exiv2Node"));

    NODE_SET_PROTOTYPE_METHOD(s_ct, "imageDateTime", ImageDateTime);

    target->Set(String::NewSymbol("Exiv2Node"), s_ct->GetFunction());
  }

  Exiv2Node() :
    m_count(0)
  {
  }

  ~Exiv2Node()
  {
  }

  static Handle<Value> New(const Arguments& args)
  {
    HandleScope scope;
    Exiv2Node* exiv2node = new Exiv2Node();
    exiv2node->Wrap(args.This());
    return args.This();
  }

  struct exiv2node_baton_t {
    Exiv2Node *exiv2node;
    int increment_by;
    int sleep_for;
    Persistent<Function> cb;
    Persistent<String> fname;
    std::string *dateTime;
  };

  static Handle<Value> ImageDateTime(const Arguments& args)
  {
    HandleScope scope;

    /* Usage arguments */
    if (args.Length() <= (1) || !args[1]->IsFunction())
      return ThrowException(Exception::TypeError(String::New("Usage: <filename> <callback function>")));
    Local<String> fname = Local<String>::Cast(args[0]);
    Local<Function> cb = Local<Function>::Cast(args[1]);

    Exiv2Node* exiv2node = ObjectWrap::Unwrap<Exiv2Node>(args.This());

    exiv2node_baton_t *baton = new exiv2node_baton_t();
    baton->exiv2node = exiv2node;
    baton->increment_by = 2;
    baton->sleep_for = 1;
    baton->cb = Persistent<Function>::New(cb);    
    baton->fname = Persistent<String>::New(fname);

    exiv2node->Ref();

    eio_custom(EIO_ImageDateTime, EIO_PRI_DEFAULT, EIO_AfterImageDateTime, baton);
    ev_ref(EV_DEFAULT_UC);

    return Undefined();
  }


  static bool hasKey(Exiv2::ExifData &data, const std::string key)
  {
    Exiv2::ExifData::iterator pos = data.findKey(Exiv2::ExifKey(key));
    return (pos != data.end());
  }

  static int EIO_ImageDateTime(eio_req *req)
  {
    exiv2node_baton_t *baton = static_cast<exiv2node_baton_t *>(req->data);

    sleep(baton->sleep_for);
    baton->exiv2node->m_count += baton->increment_by;

    /* EXIV stuff here.. */
    /* Get our image metadata */
    String::AsciiValue val(baton->fname);
    std::string name(*val);
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(name);
    image->readMetadata();
    Exiv2::ExifData &exifData = image->exifData();

    /* Look for our datatime stamp */
    if (exifData.empty() == false)
    {
        std::string *key = (std::string*)&EXIF_PHOTO_DATEIMTEORIGINAL;
        if (!hasKey(exifData, *key))
        {
            key = (std::string*)&EXIF_IMAGE_DATETIME;
            if (!hasKey(exifData, *key))
            {
                key = NULL;
            }
        }

        if (key != NULL)
        {
            baton->dateTime = new std::string(exifData[*key].value().toString());
        }
    }
    return 0;
  }

  static int EIO_AfterImageDateTime(eio_req *req)
  {
    HandleScope scope;
    exiv2node_baton_t *baton = static_cast<exiv2node_baton_t *>(req->data);
    ev_unref(EV_DEFAULT_UC);
    baton->exiv2node->Unref();

    Local<Value> argv[1];

    //argv[0] = baton->fname->ToString();
    argv[0] = String::New(baton->dateTime->c_str());

    TryCatch try_catch;

    baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    baton->cb.Dispose();
    baton->fname.Dispose();

    delete baton;
    return 0;
  }

};

Persistent<FunctionTemplate> Exiv2Node::s_ct;

extern "C" {
  static void init (Handle<Object> target)
  {
    Exiv2Node::Init(target);
  }

  NODE_MODULE(exiv2node, init);
}
