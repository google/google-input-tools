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

#include "npapi_plugin.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <cmath>

#include <ggadget/basic_element.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/graphics_interface.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/math_utils.h>
#include <ggadget/permissions.h>
#include <ggadget/signals.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/view.h>
#include <ggadget/xml_http_request_interface.h>

#include "npapi_plugin_script.h"
#include "npapi_utils.h"

#ifdef MOZ_X11
#include <X11/Xlib.h>
#endif

namespace ggadget {
namespace npapi {

static const char *kUserAgent = "ggl/" GGL_VERSION;

// The URL flashes use to send trace() messages in my test environment.
static const char *kFlashTraceURL = "http://localhost:8881";

// Timeout before releasing a plugin library.
static const int kReleasePluginLibraryTimeout = 1000;

class Plugin::Impl : public SmallObject<> {
 public:
  class StreamHandler {
   public:
    StreamHandler(Impl *owner, XMLHttpRequestInterface *http_request,
                  const std::string &method, const std::string &url_or_file,
                  const std::string &post_data, bool notify, void *notify_data)
        : owner_(owner),
          plugin_funcs_(&owner->library_info_->plugin_funcs),
          stream_(NULL), stream_offset_(0),
          // These fields hold string data to live during the stream's life.
          method_(method),
          url_or_file_(url_or_file),
          post_data_(post_data),
          http_request_(http_request),
          notify_(notify), notify_data_(notify_data),
          on_abort_connection_(owner_->abort_streams_.Connect(
              NewSlot(this, &StreamHandler::Abort))),
          on_state_change_connection_(NULL),
          on_data_received_connection_(NULL) {
      if (http_request) {
        http_request->Ref();
        on_state_change_connection_ = http_request->ConnectOnReadyStateChange(
            NewSlot(this, &StreamHandler::OnStateChange));
        on_data_received_connection_ = http_request->ConnectOnDataReceived(
            NewSlot(this, &StreamHandler::OnDataReceived));
      }
    }

    virtual ~StreamHandler() {
      if (http_request_) {
        on_state_change_connection_->Disconnect();
        on_data_received_connection_->Disconnect();
        http_request_->Abort();
        http_request_->Unref();
      }
      on_abort_connection_->Disconnect();
    }

    void Abort() {
      Done(NPRES_USER_BREAK);
    }

    void Start() {
      if (http_request_) {
        // HTTP mode.
        if (http_request_->Open(method_.c_str(), url_or_file_.c_str(), true,
                                NULL,NULL) != XMLHttpRequestInterface::NO_ERR ||
            http_request_->Send(post_data_) !=
                XMLHttpRequestInterface::NO_ERR) {
            Done(NPRES_NETWORK_ERR);
        }
      } else {
        // Local file mode.
        std::string content;
        if (ReadFileContents(url_or_file_.c_str(), &content) &&
            NewStream(content.size()) &&
            WriteStream(content.c_str(), content.size()) == content.size()) {
          Done(NPRES_DONE);
        } else {
          Done(NPRES_NETWORK_ERR);
        }
      }
    }

    void Done(NPReason reason) {
      if (stream_) {
        if (plugin_funcs_->destroystream)
          plugin_funcs_->destroystream(&owner_->instance_, stream_, reason);
        delete stream_;
        stream_ = NULL;
      }
      owner_->DoURLNotify(url_or_file_.c_str(), notify_, notify_data_, reason);
      delete this;
    }

    bool NewStream(size_t size) {
      ASSERT(!stream_);
      // NPAPI uses int32/uint32 for data size.
      // Limit data size to prevent overflow.
      if (size >= (1U << 31))
        return false;

      stream_ = new NPStream;
      memset(stream_, 0, sizeof(NPStream));
      stream_->ndata = owner_;
      stream_->url = url_or_file_.c_str();
      stream_->headers = headers_.c_str();
      stream_->end = static_cast<uint32>(size);
      stream_->notifyData = notify_data_;
      char *mime_type_copy = strdup(mime_type_.c_str());
      uint16 stream_type;
      NPError err = plugin_funcs_->newstream(&owner_->instance_, mime_type_copy,
                                             stream_, false, &stream_type);
      free(mime_type_copy);
      return err == NPERR_NO_ERROR && stream_type == NP_NORMAL;
    }

    size_t WriteStream(const void *data, size_t size) {
      ASSERT(stream_);
      size_t data_offset = 0;
      int idle_count = 0;
      while (data_offset < size) {
        int32 len = plugin_funcs_->writeready(&owner_->instance_, stream_);
        if (len <= 0) {
          if (++idle_count > 10) {
            LOG("Failed to write to stream");
            return data_offset;
          }
        }

        idle_count = 0;
        if (size < data_offset + len)
          len = static_cast<int32>(size - data_offset);
        int32 consumed = plugin_funcs_->write(
            &owner_->instance_, stream_,
            static_cast<int32>(stream_offset_), len,
            // This gets a non-const pointer.
            const_cast<void *>(data));
        // Error occurs.
        if (consumed < 0)
          return data_offset;
        data_offset += consumed;
        stream_offset_ += consumed;
      }
      return size;
    }

    std::string GetHeaders(unsigned short status) {
      std::string result = StringPrintf("HTTP/1.1 %hu", status);
      const std::string *status_text_ptr = NULL;
      http_request_->GetStatusText(&status_text_ptr);
      if (status_text_ptr) {
        result += ' ';
        result += *status_text_ptr;
      }
      result += '\n';
      const std::string *headers_ptr = NULL;
      http_request_->GetAllResponseHeaders(&headers_ptr);
      if (headers_ptr) {
        const char *p = headers_ptr->c_str();
        // Remove all '\r's according to NPAPI's requirement.
        while (*p) {
          if (*p != '\r')
            result += *p;
          p++;
        }
      }
      return result;
    }

    size_t OnDataReceived(const void *data, size_t size) {
      unsigned short status = 0;
      http_request_->GetStatus(&status);
      if (status == 200) {
        if (!stream_) {
          const std::string *length_str = NULL;
          http_request_->GetResponseHeader("Content-Length", &length_str);
          int length = 0;
          if (length_str)
            Variant(*length_str).ConvertToInt(&length);

          mime_type_ = http_request_->GetResponseContentType();
          if (mime_type_.empty())
            mime_type_ = owner_->mime_type_;
          headers_ = GetHeaders(status);
          std::string effective_url = http_request_->GetEffectiveUrl();
          if (url_or_file_ == owner_->location_) {
            // Change location to the effective URL only if the request is of
            // top level.
            owner_->location_ = effective_url;
          }
          url_or_file_ = effective_url;
          if (!NewStream(static_cast<size_t>(length))) {
            Done(NPRES_NETWORK_ERR);
            return 0;
          }
        }

        size_t written = WriteStream(data, size);
        if (written != size) {
          Done(NPRES_NETWORK_ERR);
          return 0;
        }
      }
      return size;
    }

    void OnStateChange() {
      if (http_request_->GetReadyState() == XMLHttpRequestInterface::DONE) {
        if (http_request_->IsSuccessful()) {
          Done(NPRES_DONE);
        } else {
          Done(NPRES_NETWORK_ERR);
        }
      }
    }

    Impl *owner_;
    NPPluginFuncs *plugin_funcs_;
    NPStream *stream_;
    size_t stream_offset_;
    std::string method_, url_or_file_, post_data_, mime_type_, headers_;
    XMLHttpRequestInterface *http_request_;
    bool notify_;
    void *notify_data_;
    Connection *on_abort_connection_;
    Connection *on_state_change_connection_;
    Connection *on_data_received_connection_;
  };

  Impl(const char *mime_type, BasicElement *element,
       PluginLibraryInfo *library_info,
       void *top_window, const NPWindow &window,
       const StringMap &parameters)
      : mime_type_(mime_type),
        element_(element),
        library_info_(library_info),
        plugin_root_(NULL),
        top_window_(top_window),
        window_(window),
        windowless_(false), transparent_(false),
        init_error_(NPERR_GENERIC_ERROR),
        dirty_rect_(kWholePluginRect),
        browser_window_npobject_(this, &kBrowserWindowClass),
        location_npobject_(this, &kLocationClass) {
    DLOG("New NPAPI Plugin for library: %p", library_info_);
    ASSERT(library_info);
    memset(&instance_, 0, sizeof(instance_));
    instance_.ndata = this;
    if (library_info->plugin_funcs.newp) {
      // NPP_NewUPP require non-const parameters.
      char **argn = NULL;
      char **argv = NULL;
      size_t argc = parameters.size();
      if (argc) {
        argn = new char *[argc];
        argv = new char *[argc];
        StringMap::const_iterator it = parameters.begin();
        for (size_t i = 0; i < argc; ++it, ++i) {
          argn[i] = strdup(it->first.c_str());
          argv[i] = strdup(it->second.c_str());
        }
      }
      init_error_ = library_info->plugin_funcs.newp(
          const_cast<NPMIMEType>(mime_type), &instance_, NP_EMBED,
          static_cast<int16>(argc), argn, argv, NULL);
      for (size_t i = 0; i < argc; i++) {
        free(argn[i]);
        free(argv[i]);
      }
      delete [] argn;
      delete [] argv;
    }

    if (!windowless_)
      SetWindow(top_window, window);
    // Otherwise the caller should handle the change of windowless state.
  }

  class ReleasePluginLibraryCallback : public WatchCallbackInterface {
   public:
    ReleasePluginLibraryCallback(PluginLibraryInfo *info) : info_(info) { }
    virtual bool Call(MainLoopInterface *, int) { return false; }
    virtual void OnRemove(MainLoopInterface *, int) {
      ReleasePluginLibrary(info_);
      delete this;
    }
   private:
    PluginLibraryInfo *info_;
  };

  ~Impl() {
    abort_streams_();
    if (plugin_root_)
      plugin_root_->Unref();

    if (library_info_->plugin_funcs.destroy) {
      NPError ret = library_info_->plugin_funcs.destroy(&instance_, NULL);
      if (ret != NPERR_NO_ERROR)
        LOG("Failed to destroy plugin instance - nperror code %d.", ret);
    }

    // Release the plugin library immediately may cause crash in plugin.
    GetGlobalMainLoop()->AddTimeoutWatch(
        kReleasePluginLibraryTimeout,
        new ReleasePluginLibraryCallback(library_info_));
  }

  bool SetWindow(void *top_window, const NPWindow &window) {
    // Host must have set the window info struct.
    if (!window.ws_info)
      return false;
    if (windowless_ != (window.type == NPWindowTypeDrawable)) {
      LOG("Window types don't match (passed in: %s, "
          "while plugin's type: %s)",
          window.type == NPWindowTypeDrawable ? "windowless" : "windowed",
          windowless_ ? "windowless" : "windowed");
      return false;
    }

    NPWindow window_tmp = window;
    if (library_info_->plugin_funcs.setwindow &&
        library_info_->plugin_funcs.setwindow(&instance_, &window_tmp) ==
            NPERR_NO_ERROR) {
      top_window_ = top_window;
      window_ = window_tmp;
      return true;
    }
    return false;
  }

  bool HandleEvent(void *event) {
    if (!library_info_->plugin_funcs.event)
      return false;
    return library_info_->plugin_funcs.event(&instance_, event);
  }

  ScriptableInterface *GetScriptablePlugin() {
    if (!plugin_root_) {
      NPObject *plugin_root;
      if (library_info_->plugin_funcs.getvalue != NULL &&
          library_info_->plugin_funcs.getvalue(
              &instance_, NPPVpluginScriptableNPObject, &plugin_root) ==
              NPERR_NO_ERROR &&
          plugin_root) {
        plugin_root_ = new ScriptableNPObject(plugin_root);
        plugin_root_->Ref();
        return plugin_root_;
      }
      return NULL;
    }
    return plugin_root_;
  }

  void DoURLNotify(const char *url, bool notify, void *notify_data,
                   NPReason reason) {
    if (notify && library_info_->plugin_funcs.urlnotify) {
      library_info_->plugin_funcs.urlnotify(&instance_, url, reason,
                                            notify_data);
    }
  }

  NPError HandleURL(const char *method, const char *url, const char *target,
                    const std::string &post_data,
                    bool notify, void *notify_data) {
    if (!url || !*url)
      return NPERR_INVALID_PARAM;
    if (notify && !library_info_->plugin_funcs.urlnotify)
      return NPERR_INVALID_PARAM;
    if (strncmp(url, "javascript:", 11) == 0) {
      DoURLNotify(url, notify, notify_data, NPRES_DONE);
      return NPERR_NO_ERROR;
    }

    std::string absolute_url = GetAbsoluteURL(location_.c_str(), url);
    if (absolute_url.empty())
      return NPERR_INVALID_PARAM;

    GadgetInterface *gadget = element_->GetView()->GetGadget();
    if (!gadget)
      return NPERR_GENERIC_ERROR;

    if (target) {
      // Let the gadget allow this OpenURL gracefully.
      bool old_interaction = gadget->SetInUserInteraction(true);
      gadget->OpenURL(absolute_url.c_str());
      gadget->SetInUserInteraction(old_interaction);
      // Mozilla also doesn't send notification if target is not NULL.
      return NPERR_NO_ERROR;
    }

    if (!library_info_->plugin_funcs.writeready ||
        !library_info_->plugin_funcs.write ||
        !library_info_->plugin_funcs.newstream)
      return NPERR_INVALID_PARAM;

    StreamHandler *handler = NULL;
    std::string file_path = GetPathFromFileURL(absolute_url.c_str());
    if (!file_path.empty()) {
      const Permissions *permissions = gadget->GetPermissions();
      // Only can set src to a local file. The plugin can't request to read
      // local files.
      if (absolute_url != location_ || !permissions ||
          !permissions->IsRequiredAndGranted(Permissions::FILE_READ)) {
        DoURLNotify(absolute_url.c_str(), notify, notify_data,
                    NPRES_USER_BREAK);
        LOG("The plugin is not permitted to read local files.");
        return NPERR_GENERIC_ERROR;
      }
      handler = new StreamHandler(this, NULL, "", file_path, "",
                                  notify, notify_data);
    } else {
      XMLHttpRequestInterface *request = gadget->CreateXMLHttpRequest();
      if (!request) {
        DoURLNotify(absolute_url.c_str(), notify, notify_data,
                    NPRES_USER_BREAK);
        return NPERR_GENERIC_ERROR;
      }
      handler = new StreamHandler(this, request, method, absolute_url,
                                  post_data, notify, notify_data);
    }
    handler->Start();
    return NPERR_NO_ERROR;
  }

  void InvalidateRect(NPRect *invalid_rect) {
    if (!invalid_rect)
      return;
    if (!(dirty_rect_ == kWholePluginRect)) {
      Rectangle rect(invalid_rect->left, invalid_rect->top,
                     invalid_rect->right - invalid_rect->left,
                     invalid_rect->bottom - invalid_rect->top);
      dirty_rect_.Union(rect);

      // QueueDrawRect must be in the element's original coordinates, so zoom
      // must be considered.
      double zoom = element_->GetView()->GetGraphics()->GetZoom();
      if (zoom != 1.0) {
        rect.x /= zoom;
        rect.y /= zoom;
        rect.w /= zoom;
        rect.h /= zoom;
      }
      element_->QueueDrawRect(rect);
    }
  }

  void ForceRedraw() {
    dirty_rect_ = kWholePluginRect;
    element_->MarkRedraw();
  }

  static NPError HandleURL(NPP instance, const char *method, const char *url,
                           const char *target, const std::string &post_data,
                           bool notify, void *notify_data) {
    ENSURE_MAIN_THREAD(NPERR_INVALID_PARAM);
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      return impl->HandleURL(method, url, target, post_data,
                             notify, notify_data);
    }
    return NPERR_INVALID_PARAM;
  }

  static NPError NPN_GetURL(NPP instance, const char *url, const char *target) {
    return HandleURL(instance, "GET", url, target, "", false, NULL);
  }

  static NPError NPN_GetURLNotify(NPP instance, const char *url,
                                  const char *target, void *notify_data) {
    return HandleURL(instance, "GET", url, target, "", true, notify_data);
  }

  static NPError HandlePostURL(NPP instance, const char *url,
                               const char *target, uint32 len, const char *buf,
                               NPBool file, bool notify, void *notify_data) {
    if (!buf)
      return NPERR_INVALID_PARAM;

    std::string post_data;
    if (file) {
      std::string file_name(buf, len);
      if (!ReadFileContents(file_name.c_str(), &post_data)) {
        LOG("Failed to read file: %s", file_name.c_str());
        return NPERR_GENERIC_ERROR;
      }
    } else {
      post_data.assign(buf, len);
    }

    if (strcmp(url, kFlashTraceURL) == 0) {
      size_t pos = post_data.find("\r\n\r\n");
      if (pos != post_data.npos)
        post_data = post_data.substr(pos + 4);
      post_data = DecodeURL(post_data);
      LOG("FLASH TRACE: %s", post_data.c_str());
      if (notify && instance && instance->ndata) {
        static_cast<Impl *>(instance->ndata)->DoURLNotify(
            url, notify, notify_data, NPRES_DONE);
      }
      return NPERR_NO_ERROR;
    }

    return HandleURL(instance, "POST", url, target, post_data,
                     notify, notify_data);
  }

  static NPError NPN_PostURL(NPP instance, const char *url, const char *target,
                             uint32 len, const char *buf, NPBool file) {
    return HandlePostURL(instance, url, target, len, buf, file, false, NULL);
  }

  static NPError NPN_PostURLNotify(NPP instance, const char *url,
                                   const char *target,
                                   uint32 len, const char *buf,
                                   NPBool file, void *notify_data) {
    return HandlePostURL(instance, url, target, len, buf, file,
                         true, notify_data);
  }

  static NPError NPN_RequestRead(NPStream *stream, NPByteRange *rangeList) {
    GGL_UNUSED(stream);
    GGL_UNUSED(rangeList);
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  static NPError NPN_NewStream(NPP instance, NPMIMEType type,
                               const char *target, NPStream **stream) {
    GGL_UNUSED(instance);
    GGL_UNUSED(type);
    GGL_UNUSED(target);
    GGL_UNUSED(stream);
    // Plugin-produced stream is not supported.
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  static int32 NPN_Write(NPP instance, NPStream *stream,
                         int32 len, void *buffer) {
    GGL_UNUSED(instance);
    GGL_UNUSED(stream);
    GGL_UNUSED(len);
    GGL_UNUSED(buffer);
    NOT_IMPLEMENTED();
    return -1;
  }

  static NPError NPN_DestroyStream(NPP instance, NPStream *stream,
                                   NPReason reason) {
    GGL_UNUSED(instance);
    GGL_UNUSED(stream);
    GGL_UNUSED(reason);
    NOT_IMPLEMENTED();
    return NPERR_GENERIC_ERROR;
  }

  static void NPN_Status(NPP instance, const char *message) {
    ENSURE_MAIN_THREAD_VOID();
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      impl->on_new_message_handler_(message);
    }
  }

  static const char *NPN_UserAgent(NPP instance) {
    GGL_UNUSED(instance);
    CHECK_MAIN_THREAD();
    return kUserAgent;
  }

  static uint32 NPN_MemFlush(uint32 size) {
    GGL_UNUSED(size);
    CHECK_MAIN_THREAD();
    return 0;
  }

  static void NPN_ReloadPlugins(NPBool reloadPages) {
    // We don't provide any plugin-in with the authority that can reload
    // all plug-ins in the plugins directory.
    GGL_UNUSED(reloadPages);
    NOT_IMPLEMENTED();
  }

  static JRIEnv *NPN_GetJavaEnv(void) {
    NOT_IMPLEMENTED();
    return NULL;
  }

  static jref NPN_GetJavaPeer(NPP instance) {
    GGL_UNUSED(instance);
    NOT_IMPLEMENTED();
    return NULL;
  }

  static NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value) {
    DLOG("NPN_GetValue: %d (0x%x)", variable, variable);
    // This function may be called before any instance is constructed.
    ENSURE_MAIN_THREAD(NPERR_INVALID_PARAM);
    switch (variable) {
      case NPNVjavascriptEnabledBool:
        *static_cast<NPBool *>(value) = true;
        break;
      case NPNVSupportsXEmbedBool:
        *static_cast<NPBool *>(value) = true;
        break;
      case NPNVToolkit:
        // This value is only applicable for GTK.
        *static_cast<NPNToolkitType *>(value) = NPNVGtk2;
        break;
      case NPNVisOfflineBool:
      case NPNVasdEnabledBool:
        *static_cast<NPBool *>(value) = false;
        break;
      case NPNVSupportsWindowless:
        *static_cast<NPBool *>(value) = true;
        break;
#ifdef MOZ_X11
      case NPNVxDisplay:
        *static_cast<Display **>(value) = display_;
        DLOG("NPN_GetValue NPNVxDisplay: %p", display_);
        break;
#endif
      case NPNVnetscapeWindow:
        if (instance && instance->ndata) {
          Impl *impl = static_cast<Impl *>(instance->ndata);
          *static_cast<Window *>(value) =
              reinterpret_cast<Window>(impl->top_window_);
          break;
        } else {
          return NPERR_GENERIC_ERROR;
        }
      case NPNVWindowNPObject:
        if (instance && instance->ndata) {
          Impl *impl = static_cast<Impl *>(instance->ndata);
          RetainNPObject(&impl->browser_window_npobject_);
          *static_cast<NPObject **>(value) = &impl->browser_window_npobject_;
          break;
        } else {
          return NPERR_GENERIC_ERROR;
        }
      case NPNVserviceManager:
      case NPNVDOMElement:
      case NPNVPluginElementNPObject:
        // TODO: This variable is for popup window/menu purpose when
        // windowless mode is used. We must provide a parent window for
        // the plugin to show popups if we want to support.
      case NPNVxtAppContext:
      default:
        LOG("NPNVariable %d is not supported.", variable);
        return NPERR_GENERIC_ERROR;
    }
    return NPERR_NO_ERROR;
  }

  static NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value) {
    ENSURE_MAIN_THREAD(NPERR_INVALID_PARAM);
    DLOG("NPN_SetValue: %d (0x%x) %p", variable, variable, value);
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      switch (variable) {
        case NPPVpluginWindowBool:
          impl->windowless_ = !value;
          break;
        case NPPVpluginTransparentBool:
          impl->transparent_ = value ? true : false;
          break;
        case NPPVjavascriptPushCallerBool:
        case NPPVpluginKeepLibraryInMemory:
        default:
          LOG("NPNVariable %d is not supported.", variable);
          return NPERR_GENERIC_ERROR;
      }
      return NPERR_NO_ERROR;
    }
    return NPERR_INVALID_PARAM;
  }

  static void NPN_InvalidateRect(NPP instance, NPRect *invalid_rect) {
    ENSURE_MAIN_THREAD_VOID();
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      impl->InvalidateRect(invalid_rect);
    }
  }

  static void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion) {
    GGL_UNUSED(instance);
    GGL_UNUSED(invalidRegion);
    NOT_IMPLEMENTED();
  }

  static void NPN_ForceRedraw(NPP instance) {
    ENSURE_MAIN_THREAD_VOID();
    if (instance && instance->ndata) {
      Impl *impl = static_cast<Impl *>(instance->ndata);
      impl->ForceRedraw();
    }
  }

  static void NPN_GetStringIdentifiers(const NPUTF8 **names,
                                       int32_t name_count,
                                       NPIdentifier *identifiers) {
    ENSURE_MAIN_THREAD_VOID();
    for (int32_t i = 0; i < name_count; i++)
      identifiers[i] = GetStringIdentifier(names[i]);
  }

  static bool NPN_Invoke(NPP npp, NPObject *npobj, NPIdentifier method_name,
                         const NPVariant *args, uint32_t arg_count,
                         NPVariant *result) {
    GGL_UNUSED(npp);
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->invoke &&
           npobj->_class->invoke(npobj, method_name, args, arg_count, result);
  }

  static bool NPN_InvokeDefault(NPP npp, NPObject *npobj,
                                const NPVariant *args, uint32_t arg_count,
                                NPVariant *result) {
    GGL_UNUSED(npp);
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->invokeDefault &&
           npobj->_class->invokeDefault(npobj, args, arg_count, result);
  }

  static bool NPN_Evaluate(NPP npp, NPObject *npobj, NPString *script,
                           NPVariant *result) {
    GGL_UNUSED(npp);
    GGL_UNUSED(npobj);
    GGL_UNUSED(script);
    GGL_UNUSED(result);
    NOT_IMPLEMENTED();
    return false;
  }

  static bool NPN_GetProperty(NPP npp, NPObject *npobj,
                              NPIdentifier property_name, NPVariant *result) {
    GGL_UNUSED(npp);
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->getProperty &&
           npobj->_class->getProperty(npobj, property_name, result);
  }

  static bool NPN_SetProperty(NPP npp, NPObject *npobj,
                              NPIdentifier property_name,
                              const NPVariant *value) {
    GGL_UNUSED(npp);
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->setProperty &&
           npobj->_class->setProperty(npobj, property_name, value);
  }

  static bool NPN_RemoveProperty(NPP npp, NPObject *npobj,
                                 NPIdentifier property_name) {
    GGL_UNUSED(npp);
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->removeProperty &&
           npobj->_class->removeProperty(npobj, property_name);
  }

  static bool NPN_HasProperty(NPP npp, NPObject *npobj,
                              NPIdentifier property_name) {
    GGL_UNUSED(npp);
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->hasProperty &&
           npobj->_class->hasProperty(npobj, property_name);
  }

  static bool NPN_HasMethod(NPP npp, NPObject *npobj,
                            NPIdentifier method_name) {
    GGL_UNUSED(npp);
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->hasMethod &&
           npobj->_class->hasMethod(npobj, method_name);
  }

  static void NPN_SetException(NPObject *npobj, const NPUTF8 *message) {
    GGL_UNUSED(npobj);
    GGL_UNUSED(message);
    NOT_IMPLEMENTED();
  }

  static bool NPN_PushPopupsEnabledState(NPP instance, NPBool enabled) {
    GGL_UNUSED(instance);
    GGL_UNUSED(enabled);
    NOT_IMPLEMENTED();
    return false;
  }

  static bool NPN_PopPopupsEnabledState(NPP instance) {
    GGL_UNUSED(instance);
    NOT_IMPLEMENTED();
    return false;
  }

  static bool NPN_Enumerate(NPP npp, NPObject *npobj,
                            NPIdentifier **identifiers, uint32_t *count) {
    GGL_UNUSED(npp);
    ENSURE_MAIN_THREAD(false);
    return npobj && npobj->_class && npobj->_class->enumerate &&
           npobj->_class->enumerate(npobj, identifiers, count);
  }

  class AsyncCall : public WatchCallbackInterface {
   public:
    AsyncCall(void (*func)(void *), void *user_data)
        : func_(func), user_data_(user_data) { }
    virtual bool Call(MainLoopInterface *mainloop, int watch_id) {
      GGL_UNUSED(mainloop);
      GGL_UNUSED(watch_id);
      (*func_)(user_data_);
      return false;
    }
    virtual void OnRemove(MainLoopInterface *mainloop, int watch_id) {
      GGL_UNUSED(mainloop);
      GGL_UNUSED(watch_id);
      delete this;
    }
   private:
    void (*func_)(void *);
    void *user_data_;
  };

  struct OwnedNPObject : public NPObject {
    OwnedNPObject(Impl *a_owner, NPClass *a_class) {
      memset(this, 0, sizeof(OwnedNPObject));
      owner = a_owner;
      _class = a_class;
      referenceCount = 1;
    }
    Impl *owner;
  };

  // According to NPAPI specification, plugins should perform appropriate
  // synchronization with the code in their NPP_Destroy routine to avoid
  // incorrect execution and memory leaks caused by the race conditions
  // between calling this function and termination of the plugin instance.
  static void NPN_PluginThreadAsyncCall(NPP instance, void (*func)(void *),
                                        void *user_data) {
    GGL_UNUSED(instance);
    if (GetGlobalMainLoop()->IsMainThread())
      DLOG("NPN_PluginThreadAsyncCall called from the non-main thread.");
    else
      DLOG("NPN_PluginThreadAsyncCall called from the main thread.");
    GetGlobalMainLoop()->AddTimeoutWatch(0, new AsyncCall(func, user_data));
  }

  static bool NPN_Construct(NPP npp, NPObject *npobj,
                            const NPVariant *args, uint32_t argc,
                            NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    return npp && npobj && npobj->_class && npobj->_class->construct &&
           npobj->_class->construct(npobj, args, argc, result);
  }

  // Only support window.top and window.location because the flash plugin
  // requires them.
  static bool BrowserWindowHasProperty(NPObject *npobj, NPIdentifier name) {
    GGL_UNUSED(npobj);
    ENSURE_MAIN_THREAD(false);
    std::string name_str = GetIdentifierName(name);
    return name_str == "location" || name_str == "top";
  }
  static bool BrowserWindowGetProperty(NPObject *npobj, NPIdentifier name,
                                       NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    std::string name_str = GetIdentifierName(name);
    Impl *owner = reinterpret_cast<OwnedNPObject *>(npobj)->owner;
    if (name_str == "location") {
      RetainNPObject(&owner->location_npobject_);
      OBJECT_TO_NPVARIANT(&owner->location_npobject_, *result);
      return true;
    } else if (name_str == "top") {
      RetainNPObject(&owner->browser_window_npobject_);
      OBJECT_TO_NPVARIANT(&owner->browser_window_npobject_, *result);
      return true;
    }
    return false;
  }

  static bool LocationHasMethod(NPObject *npobj, NPIdentifier name) {
    GGL_UNUSED(npobj);
    ENSURE_MAIN_THREAD(false);
    return GetIdentifierName(name) == "toString";
  }

  static bool LocationInvoke(NPObject *npobj, NPIdentifier name,
                             const NPVariant *args, uint32_t argCount,
                             NPVariant *result) {
    GGL_UNUSED(args);
    GGL_UNUSED(argCount);
    ENSURE_MAIN_THREAD(false);
    if (GetIdentifierName(name) == "toString") {
      Impl *owner = reinterpret_cast<OwnedNPObject *>(npobj)->owner;
      NewNPVariantString(owner->location_, result);
      return true;
    }
    return false;
  }

  static bool LocationHasProperty(NPObject *npobj, NPIdentifier name) {
    GGL_UNUSED(npobj);
    ENSURE_MAIN_THREAD(false);
    return GetIdentifierName(name) == "href";
  }

  static bool LocationGetProperty(NPObject *npobj, NPIdentifier name,
                                       NPVariant *result) {
    ENSURE_MAIN_THREAD(false);
    if (GetIdentifierName(name) == "href") {
      Impl *owner = reinterpret_cast<OwnedNPObject *>(npobj)->owner;
      NewNPVariantString(owner->location_, result);
      return true;
    }
    return false;
  }

  std::string mime_type_;
  BasicElement *element_;
  PluginLibraryInfo *library_info_;
  NPP_t instance_;
  ScriptableNPObject *plugin_root_;
  void *top_window_;
  NPWindow window_;
  bool windowless_;
  bool transparent_;
  NPError init_error_;
  Rectangle dirty_rect_;

  Signal1<void, const char *> on_new_message_handler_;
  Signal0<void> abort_streams_;
  static const NPNetscapeFuncs kContainerFuncs;
#ifdef MOZ_X11
  // Stores the XDisplay pointer to make it available during plugin library
  // initialization. nspluginwrapper requires this.
  static Display *display_;
#endif

  std::string location_;
  OwnedNPObject browser_window_npobject_;
  OwnedNPObject location_npobject_;
  static NPClass kBrowserWindowClass;
  static NPClass kLocationClass;
};

NPClass Plugin::Impl::kBrowserWindowClass = {
  NP_CLASS_STRUCT_VERSION,
  NULL, NULL, NULL, NULL, NULL, NULL,
  BrowserWindowHasProperty,
  BrowserWindowGetProperty,
  NULL, NULL, NULL, NULL
};

NPClass Plugin::Impl::kLocationClass = {
  NP_CLASS_STRUCT_VERSION,
  NULL, NULL, NULL,
  LocationHasMethod,
  LocationInvoke,
  NULL,
  LocationHasProperty,
  LocationGetProperty,
  NULL, NULL, NULL, NULL
};

const NPNetscapeFuncs Plugin::Impl::kContainerFuncs = {
  static_cast<uint16>(sizeof(NPNetscapeFuncs)),
  static_cast<uint16>((NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR),
  NPN_GetURL,
  NPN_PostURL,
  NPN_RequestRead,
  NPN_NewStream,
  NPN_Write,
  NPN_DestroyStream,
  NPN_Status,
  NPN_UserAgent,
  MemAlloc,
  MemFree,
  NPN_MemFlush,
  NPN_ReloadPlugins,
  NPN_GetJavaEnv,
  NPN_GetJavaPeer,
  NPN_GetURLNotify,
  NPN_PostURLNotify,
  NPN_GetValue,
  NPN_SetValue,
  NPN_InvalidateRect,
  NPN_InvalidateRegion,
  NPN_ForceRedraw,
  GetStringIdentifier,
  NPN_GetStringIdentifiers,
  GetIntIdentifier,
  IdentifierIsString,
  UTF8FromIdentifier,
  IntFromIdentifier,
  CreateNPObject,
  RetainNPObject,
  ReleaseNPObject,
  NPN_Invoke,
  NPN_InvokeDefault,
  NPN_Evaluate,
  NPN_GetProperty,
  NPN_SetProperty,
  NPN_RemoveProperty,
  NPN_HasProperty,
  NPN_HasMethod,
  ReleaseNPVariant,
  NPN_SetException,
  NPN_PushPopupsEnabledState,
  NPN_PopPopupsEnabledState,
  NPN_Enumerate,
  NPN_PluginThreadAsyncCall,
  NPN_Construct,
};

#ifdef MOZ_X11
Display *Plugin::Impl::display_ = NULL;
#endif

const Rectangle Plugin::kWholePluginRect(-1, -1, -1, -1);

Plugin::Plugin() : impl_(NULL) {
  // impl_ should be set in Plugin::Create().
}

Plugin::~Plugin() {
  delete impl_;
}

void Plugin::Destroy() {
  delete this;
}

std::string Plugin::GetName() const {
  return impl_->library_info_->name;
}

std::string Plugin::GetDescription() const {
  return impl_->library_info_->description;
}

bool Plugin::IsWindowless() const {
  return impl_->windowless_;
}

bool Plugin::SetWindow(void *top_window, const NPWindow &window) {
  return impl_->SetWindow(top_window, window);
}

bool Plugin::IsTransparent() const {
  return impl_->transparent_;
}

bool Plugin::HandleEvent(void *event) {
  return impl_->HandleEvent(event);
}

void Plugin::SetSrc(const char *src) {
  // Start the initial data stream.
  impl_->location_ = IsAbsolutePath(src) ? std::string("file://") + src : src;
  impl_->abort_streams_();
  impl_->HandleURL("GET", impl_->location_.c_str(), NULL, "", false, NULL);
}

Connection *Plugin::ConnectOnNewMessage(Slot1<void, const char *> *handler) {
  return impl_->on_new_message_handler_.Connect(handler);
}

ScriptableInterface *Plugin::GetScriptablePlugin() {
  return impl_->GetScriptablePlugin();
}

Rectangle Plugin::GetDirtyRect() const {
  return impl_->dirty_rect_;
}

void Plugin::ResetDirtyRect() {
  impl_->dirty_rect_.Reset();
}

Plugin *Plugin::Create(const char *mime_type, BasicElement *element,
                       void *top_window, const NPWindow &window,
                       const StringMap &parameters) {
#ifdef MOZ_X11
  // Set it early because it may be used during library initialization.
  if (!Impl::display_ && window.ws_info)
    Impl::display_ = static_cast<NPSetWindowCallbackStruct *>(
        window.ws_info)->display;
#endif
  PluginLibraryInfo *library_info = GetPluginLibrary(mime_type,
                                                     &Impl::kContainerFuncs);
  if (!library_info)
    return NULL;

  Plugin *plugin = new Plugin();
  plugin->impl_ = new Plugin::Impl(mime_type, element, library_info,
                                   top_window, window, parameters);

  if (plugin->impl_->init_error_ != NPERR_NO_ERROR) {
    plugin->Destroy();
    return NULL;
  }

  return plugin;
}

} // namespace npapi
} // namespace ggadget
