/*
  Copyright 2011 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "host_utils.h"

#include <ctime>
#include <string>
#include <map>
#include <sys/types.h>

#include "common.h"
#include "dir_file_manager.h"
#include "element_factory.h"
#include "file_manager_factory.h"
#include "file_manager_wrapper.h"
#include "gadget_consts.h"
#include "gadget_manager_interface.h"
#include "host_interface.h"
#include "locales.h"
#include "localized_file_manager.h"
#include "logger.h"
#include "main_loop_interface.h"
#include "messages.h"
#include "options_interface.h"
#include "scriptable_function.h"
#include "scriptable_map.h"
#include "scriptable_view.h"
#include "script_runtime_interface.h"
#include "script_runtime_manager.h"
#include "slot.h"
#include "string_utils.h"
#include "view.h"
#include "xml_http_request_interface.h"
#include "xml_parser_interface.h"
#include "gadget.h"

namespace ggadget {

static const char *kGlobalResourcePaths[] = {
#ifdef _DEBUG
  "resources.gg",
  "resources",
#endif
#ifdef GGL_RESOURCE_DIR
  GGL_RESOURCE_DIR "/resources.gg",
  GGL_RESOURCE_DIR "/resources",
#endif
  NULL
};

bool SetupGlobalFileManager(const char *profile_dir) {
  FileManagerWrapper *fm_wrapper = new FileManagerWrapper();
  FileManagerInterface *fm;

  for (size_t i = 0; kGlobalResourcePaths[i]; ++i) {
    fm = CreateFileManager(kGlobalResourcePaths[i]);
    if (fm) {
      fm_wrapper->RegisterFileManager(kGlobalResourcePrefix,
                                      new LocalizedFileManager(fm));
      break;
    }
  }
  if ((fm = CreateFileManager(kDirSeparatorStr)) != NULL) {
    fm_wrapper->RegisterFileManager(kDirSeparatorStr, fm);
  }
#ifdef _DEBUG
  std::string dot_slash(".");
  dot_slash += kDirSeparatorStr;
  if ((fm = CreateFileManager(dot_slash.c_str())) != NULL) {
    fm_wrapper->RegisterFileManager(dot_slash.c_str(), fm);
  }
#endif

  fm = DirFileManager::Create(profile_dir, true);
  if (fm != NULL) {
    fm_wrapper->RegisterFileManager(kProfilePrefix, fm);
  } else {
    LOG("Failed to initialize profile directory.");
  }

  SetGlobalFileManager(fm_wrapper);
  return true;
}

static int g_log_level = LOG_TRACE;
static bool g_long_log = true;
static Connection *g_global_listener_connection = NULL;


static std::string DefaultLogListener(LogLevel level,
                                      const char *filename,
                                      int line,
                                      const std::string &message) {
#if defined(OS_WIN)
  if (level >= g_log_level) {
    std::string output_message;
    if (g_long_log) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      StringAppendPrintf(&output_message,
                         "%02d:%02d.%03d: ",
                         static_cast<int>(tv.tv_sec / 60 % 60),
                         static_cast<int>(tv.tv_sec % 60),
                         static_cast<int>(tv.tv_usec / 1000));
      if (filename) {
        // Print only the last part of the file name.
        const char *name = strrchr(filename, kDirSeparator);
        if (name)
          filename = name + 1;
        StringAppendPrintf(&output_message, "%s:%d: ", filename, line);
      }
    }
    output_message.append(message);
    output_message.append("\n");
    ::OutputDebugStringA(output_message.c_str());
  }
  return message;
#elif defined(OS_POSIX)
  if (level >= g_log_level) {
    if (g_long_log) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      printf("%02d:%02d.%03d: ",
             static_cast<int>(tv.tv_sec / 60 % 60),
             static_cast<int>(tv.tv_sec % 60),
             static_cast<int>(tv.tv_usec / 1000));
      if (filename) {
        // Print only the last part of the file name.
        const char *name = strrchr(filename, '/');
        if (name)
          filename = name + 1;
        printf("%s:%d: ", filename, line);
      }
    }
    printf("%s\n", message.c_str());
    fflush(stdout);
  }
  return message;
#endif
}

void SetupLogger(int log_level, bool long_log) {
  g_log_level = log_level;
  g_long_log = long_log;
  if (!g_global_listener_connection) {
    g_global_listener_connection =
        ConnectGlobalLogListener(NewSlot(DefaultLogListener));
  }
}

bool CheckRequiredExtensions(std::string *message) {
#if defined(OS_WIN)
  if (!GetGlobalFileManager()) {
    *message = "Program can't start because gloable file manager is not ready";
    return false;
  }
  if (!GetXMLParser()) {
    *message = "Program can't start because xml parser is not ready";
    return false;
  }
  if (!GetGlobalMainLoop()) {
    *message = "Program can't start because global main loop is not ready";
    return false;
  }
  return true;
#elif defined(OS_POSIX)
  if (!GetGlobalFileManager()->FileExists(kCommonJS, NULL)) {
    // We can't use localized message here because resource failed to load.
    *message = "Program can't start because it failed to load resources";
    return false;
  }

  if (!GetXMLParser()) {
    // We can't use localized message here because XML parser is not available.
    *message = "Program can't start because it failed to load the "
        "libxml2-xml-parser module.";
    return false;
  }

  *message = "";
  if (!ScriptRuntimeManager::get()->GetScriptRuntime("js"))
    *message += "js-script-runtime\n";
  if (!GetXMLHttpRequestFactory())
    *message += "xml-http-request\n";
  if (!GetGadgetManager())
    *message += "google-gadget-manager\n";

  if (!message->empty()) {
    *message = GMS_("LOAD_EXTENSIONS_FAIL") + "\n\n" + *message;
    return false;
  }
  return true;
#endif
}

#if defined(OS_POSIX)
void InitXHRUserAgent(const char *app_name) {
  XMLHttpRequestFactoryInterface *xhr_factory = GetXMLHttpRequestFactory();
  ASSERT(xhr_factory);
  std::string platform(GGL_PLATFORM);
  if (!platform.empty())
    platform[0] = static_cast<char>(toupper(platform[0]));
  std::string user_agent = StringPrintf(
      "%s/" GGL_VERSION " (%s; %s; ts:" GGL_VERSION_TIMESTAMP
      "; api:" GGL_API_VERSION
#ifdef GGL_OEM_BRAND
      "; oem:" GGL_OEM_BRAND ")",
#else
      ")",
#endif
      app_name, platform.c_str(), GetSystemLocaleName().c_str());
  xhr_factory->SetDefaultUserAgent(user_agent.c_str());
}
#endif

static int BestPosition(int total, int pos, int size) {
  if (pos + size < total)
    return pos;
  else if (size > total)
    return 0;
  else
    return total - size;
}

void GetPopupPosition(int x, int y, int w, int h,
                      int w1, int h1, int sw, int sh,
                      int *x1, int *y1) {
  int left_gap = x - w1;
  int right_gap = sw - (x + w + w1);
  int top_gap = y - h1;
  int bottom_gap = sh - (y + h + h1);

  // We prefer to popup to right
  if (right_gap >= 0) {
    *x1 = x + w;
    *y1 = BestPosition(sh, y, h1);
  } else if (left_gap > top_gap && left_gap > bottom_gap) {
    *x1 = x - w1;
    *y1 = BestPosition(sh, y, h1);
  } else if (top_gap > bottom_gap) {
    *y1 = y - h1;
    *x1 = BestPosition(sw, x, w1);
  } else {
    *y1 = y + h;
    *x1 = BestPosition(sw, x, w1);
  }
}

#if defined(OS_POSIX)
static bool DialogCallback(int flag, View *view) {
  SimpleEvent event((flag == ViewInterface::OPTIONS_VIEW_FLAG_OK) ?
                    Event::EVENT_OK : Event::EVENT_CANCEL);
  return view->OnOtherEvent(event) != EVENT_RESULT_CANCELED;
}

bool ShowDialogView(HostInterface *host, const char *location, int flags,
                    const LightMap<std::string, Variant> &params) {
  FileManagerInterface *file_manager = GetGlobalFileManager();
  std::string xml;
  if (!file_manager || !file_manager->ReadFile(location, &xml))
    return false;
  std::string full_path = file_manager->GetFullPath(location);

  ScriptContextInterface *context =
      ScriptRuntimeManager::get()->CreateScriptContext("js");
  if (!context)
    return false;

  bool ret = false;
  ElementFactory element_factory;
  View *view = new View(
      host->NewViewHost(NULL, ViewHostInterface::VIEW_HOST_OPTIONS),
      NULL, &element_factory, context);
  ScriptableView *scriptable_view = new ScriptableView(view, NULL, context);
  // Set up the "optionsViewData" variable in the opened dialog.
  context->AssignFromNative(NULL, "", "optionsViewData",
                            Variant(NewScriptableMap(params)));
  if (scriptable_view->InitFromXML(xml, full_path.c_str()))
    ret = view->ShowView(true, flags, NewSlot(DialogCallback, view));

  delete scriptable_view;
  delete view;
  context->Destroy();
  return ret;
}

static void AboutOpenURL(const char *url, HostInterface *host) {
  host->OpenURL(NULL, url);
}

void ShowAboutDialog(HostInterface *host) {
  LightMap<std::string, Variant> params;
  params["title"] = Variant(GMS_("GOOGLE_GADGETS"));
  params["version"] = Variant(std::string(GGL_VERSION) + " " +
      StringPrintf(GM_("API_VERSION"), GGL_API_VERSION)
#ifdef _DEBUG
      + StringPrintf("\npid: %u", getpid())
#endif
      );
  params["copyright"] = Variant(GMS_("GGL_COPYRIGHT"));
  params["description"] = Variant(GMS_("GGL_DESCRIPTION"));
  // AnchorElement's OpenURL() doesn't work because the view is run without
  // a gadget. Provides it a working openURL().
  ScriptableFunction *function =
    new ScriptableFunction(NewSlot(AboutOpenURL, host));
  function->Ref();
  params["openURL"] = Variant(function);
  ShowDialogView(host, kGGLAboutView, ViewInterface::OPTIONS_VIEW_FLAG_OK,
                 params);
  function->Unref();
}

class HostArgumentParser::Impl {
 public:
  Impl(const HostArgumentInfo *args)
    : args_(args), last_id_(-1), last_type_(Variant::TYPE_VOID),
      started_(false), error_occurred_(false) {
    ASSERT(args);
#ifdef _DEBUG
    for (const HostArgumentInfo *arg = args_; arg->id >= 0; ++arg) {
      // An argument must have at least one name.
      if ((!arg->short_name || !*arg->short_name) &&
          (!arg->long_name || !*arg->long_name)) {
        ASSERT_M(false, ("Argument %d doesn't have a name.", arg->id));
      }
      switch (arg->type) {
        case Variant::TYPE_BOOL:
        case Variant::TYPE_INT64:
        case Variant::TYPE_DOUBLE:
        case Variant::TYPE_STRING:
          break;
        default:
          ASSERT_M(false, ("Type of argument %d is invalid.", arg->id));
          break;
      }
    }
#endif
  }

  bool SetArgumentValue(int id, Variant::Type type, const std::string &value) {
    bool result = false;
    switch(type) {
      case Variant::TYPE_BOOL:
        bool bool_value;
        if (Variant(value).ConvertToBool(&bool_value)) {
          specified_args_[id] = Variant(bool_value);
          result = true;
        }
        break;
      case Variant::TYPE_INT64:
        int64_t int_value;
        if (Variant(value).ConvertToInt64(&int_value)) {
          specified_args_[id] = Variant(int_value);
          result = true;
        }
        break;
      case Variant::TYPE_DOUBLE:
        double double_value;
        if (Variant(value).ConvertToDouble(&double_value)) {
          specified_args_[id] = Variant(double_value);
          result = true;
        }
        break;
      case Variant::TYPE_STRING:
        specified_args_[id] = Variant(value);
        result = true;
        break;
      default:
        DLOG("Type of argument %d is invalid.", id);
        return false;
    }

    return result;
  }

  const HostArgumentInfo *GetArgumentInfoById(int id) {
    for (const HostArgumentInfo *p = args_; p->id >= 0; ++p) {
      if (p->id == id)
        return p;
    }
    return NULL;
  }

  const HostArgumentInfo *args_;
  int last_id_;
  Variant::Type last_type_;
  bool started_;
  bool error_occurred_;

  typedef LightMap<int, Variant> ArgumentValueMap;
  ArgumentValueMap specified_args_;
  StringVector remained_args_;
};

HostArgumentParser::HostArgumentParser(const HostArgumentInfo *args)
  : impl_(new Impl(args)) {
}

HostArgumentParser::~HostArgumentParser() {
  delete impl_;
  impl_ = NULL;
}

bool HostArgumentParser::Start() {
  if (impl_->started_) {
    DLOG("Argument parse process is already started.");
    return false;
  }

  impl_->last_id_ = -1;
  impl_->last_type_ = Variant::TYPE_VOID;
  impl_->started_ = true;
  impl_->error_occurred_ = false;
  impl_->remained_args_.clear();
  impl_->specified_args_.clear();
  return true;
}

bool HostArgumentParser::AppendArgument(const char *arg) {
  ASSERT(arg && *arg);

  if (!impl_->started_) {
    DLOG("Argument parse process is not started yet.");
    return false;
  }
  if (impl_->error_occurred_) {
    DLOG("Error has been occurred.");
    return false;
  }

  std::string arg_str = TrimString(arg);

  // If last argument doesn't have a value yet, then this argument must be the
  // value.
  // Boolean arguments will not be handled here, because they always have
  // default value.
  if (impl_->last_id_ >= 0 &&
      impl_->specified_args_[impl_->last_id_].type() == Variant::TYPE_VOID) {
    if (impl_->SetArgumentValue(impl_->last_id_, impl_->last_type_, arg_str)) {
      impl_->last_id_ = -1;
      impl_->last_type_ = Variant::TYPE_VOID;
      return true;
    }
    DLOG("Invalid value for argument %d: %s", impl_->last_id_, arg_str.c_str());
    impl_->error_occurred_ = true;
    return false;
  }

  std::string arg_name, arg_value;
  SplitString(arg_str, "=", &arg_name, &arg_value);

  arg_name = TrimString(arg_name);
  arg_value = TrimString(arg_value);

  // Find the argument in known arguments list.
  if (arg_name.length()) {
    for (const HostArgumentInfo *a = impl_->args_; a->id >= 0; ++a) {
      if ((a->short_name && strcmp(a->short_name, arg_name.c_str()) == 0) ||
          (a->long_name && strcmp(a->long_name, arg_name.c_str()) == 0)) {
        if (impl_->specified_args_.find(a->id) !=
            impl_->specified_args_.end()) {
          DLOG("Argument %s is already specified.", arg_name.c_str());
          impl_->error_occurred_ = true;
          return false;
        }

        // Boolean arguments have default "true" value.
        if (a->type == Variant::TYPE_BOOL && arg_value.empty())
          arg_value = "true";

        bool result = false;
        if (arg_value.length()) {
          result = impl_->SetArgumentValue(a->id, a->type, arg_value);
        } else {
          // Value is not specified yet.
          impl_->specified_args_[a->id] = Variant();
          result = true;
        }

        if (result) {
          impl_->last_id_ = a->id;
          impl_->last_type_ = a->type;
        } else {
          DLOG("Invalid value for argument %d: %s", a->id, arg_value.c_str());
          impl_->error_occurred_ = true;
        }
        return result;
      }
    }
  }

  // The argument is not found, it might be a remained argument, or the value
  // of previous boolean argument.
  if (impl_->last_id_ >= 0 && impl_->last_type_ == Variant::TYPE_BOOL) {
    if (impl_->SetArgumentValue(impl_->last_id_, impl_->last_type_, arg_str)) {
      impl_->last_id_ = -1;
      impl_->last_type_ = Variant::TYPE_VOID;
      return true;
    }
  }

  impl_->last_id_ = -1;
  impl_->last_type_ = Variant::TYPE_VOID;
  impl_->remained_args_.push_back(arg_str);
  return true;
}

bool HostArgumentParser::AppendArguments(int argc, const char * const argv[]) {
  // argc == 0 is allowed.
  ASSERT(argv || argc == 0);

  if (!impl_->started_) {
    DLOG("Argument parse process is not started yet.");
    return false;
  }
  if (impl_->error_occurred_) {
    DLOG("Error has been occurred.");
    return false;
  }

  bool result = true;
  for (int i = 0; i < argc; ++i) {
    result = AppendArgument(argv[i]);
    if (!result)
      break;
  }
  return result;
}

bool HostArgumentParser::Finish() {
  if (!impl_->started_) {
    DLOG("Argument parse process is not started yet.");
    return false;
  }

  impl_->started_ = false;
  if (impl_->error_occurred_) {
    DLOG("Error has been occurred.");
    return false;
  }

  Impl::ArgumentValueMap::iterator it = impl_->specified_args_.begin();
  Impl::ArgumentValueMap::iterator end = impl_->specified_args_.end();
  for (; it != end; ++it) {
    if (it->second.type() == Variant::TYPE_VOID) {
      DLOG("Argument %d has no value.", it->first);
      return false;
    }
  }

  return true;
}

bool HostArgumentParser::GetArgumentValue(int id, Variant *value) const {
  Impl::ArgumentValueMap::const_iterator it = impl_->specified_args_.find(id);
  if (it != impl_->specified_args_.end()) {
    if (value)
      *value = it->second;
    return it->second.type() != Variant::TYPE_VOID;
  }
  if (value)
    *value = Variant();
  return false;
}

bool HostArgumentParser::EnumerateRecognizedArgs(
    Slot1<bool, const std::string &> *slot) const {
  ASSERT(slot);
  Impl::ArgumentValueMap::const_iterator it = impl_->specified_args_.begin();
  Impl::ArgumentValueMap::const_iterator end = impl_->specified_args_.end();
  bool result = true;
  for (; it != end; ++it) {
    const HostArgumentInfo *info = impl_->GetArgumentInfoById(it->first);
    const char *name = info->long_name ? info->long_name : info->short_name;
    std::string value;
    if (info && name && it->second.ConvertToString(&value)) {
      std::string arg = StringPrintf("%s=%s", name, value.c_str());
      result = (*slot)(arg);
      if (!result) break;
    }
  }
  delete slot;
  return result;
}

bool HostArgumentParser::EnumerateRemainedArgs(
    Slot1<bool, const std::string &> *slot) const {
  ASSERT(slot);
  StringVector::const_iterator it = impl_->remained_args_.begin();
  StringVector::const_iterator end = impl_->remained_args_.end();
  bool result = true;
  for (; it != end; ++it) {
    result = (*slot)(*it);
    if (!result) break;
  }
  delete slot;
  return result;
}

const char HostArgumentParser::kStartSignature[] = "<|START|>";
const char HostArgumentParser::kFinishSignature[] = "<|FINISH|>";

void SetupGadgetGetFeedbackURLHandler(Gadget *gadget) {
  class GetFeedbackURLSlot : public Slot0<std::string> {
   public:
    GetFeedbackURLSlot(Gadget *gadget)
      : gadget_(gadget), url_retrieved_(false) {
    }
    virtual ResultVariant Call(ScriptableInterface *object,
                               int argc, const Variant argv[]) const {
      GGL_UNUSED(object);
      GGL_UNUSED(argc);
      GGL_UNUSED(argv);
      if (!url_retrieved_) {
        GadgetManagerInterface *gadget_manager = GetGadgetManager();
        if (gadget_ && gadget_manager) {
          url_ = gadget_manager->GetGadgetInstanceFeedbackURL(
              gadget_->GetInstanceID());
        }
        url_retrieved_ = true;
      }

      return ResultVariant(Variant(url_));
    }
    virtual bool operator==(const Slot &another) const {
      GGL_UNUSED(another);
      return false;
    }
   private:
    Gadget *gadget_;
    mutable std::string url_;
    mutable bool url_retrieved_;
  };

  if (gadget) {
    gadget->ConnectOnGetFeedbackURL(new GetFeedbackURLSlot(gadget));
  }
}
#endif

} // namespace ggadget
