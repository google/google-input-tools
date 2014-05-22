/*
  Copyright 2008 Google Inc.

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

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <gtk/gtk.h>

#define MOZILLA_CLIENT
#include <mozilla-config.h>

#ifdef XPCOM_GLUE
#include <nsXPCOMGlue.h>
#include <gtkmozembed_glue.cpp>
#include "../smjs_script_runtime/libmozjs_glue.h"
#endif

#include <gtkmozembed.h>
#include <gtkmozembed_internal.h>
#include <jsapi.h>

#ifdef HAVE_JSVERSION_H
#include <jsversion.h>
#else
#include <jsconfig.h>
#endif

#include <nsComponentManagerUtils.h>
#include <nsCOMPtr.h>
#ifdef XPCOM_GLUE
#include <nsCRTGlue.h>
#else
#include <nsCRT.h>
#endif
#include <nsEvent.h>
#include <nsICategoryManager.h>
#include <nsIChannel.h>
#include <nsIChannelEventSink.h>
#include <nsIComponentRegistrar.h>
#include <nsIContentPolicy.h>
#include <nsIDOMAbstractView.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentView.h>
#include <nsIDOMNode.h>
#include <nsIDOMWindow.h>
#include <nsIGenericFactory.h>
#include <nsIInterfaceRequestor.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIScriptExternalNameSet.h>
#include <nsIScriptGlobalObject.h>
#include <nsIScriptNameSpaceManager.h>
#include <nsIURI.h>
#include <nsIWebProgress.h>
#include <nsIWebProgressListener.h>
#include <nsIXPConnect.h>
#include <nsIXPCScriptable.h>
#include <nsServiceManagerUtils.h>
#include <nsStringAPI.h>
#include <nsXPCOMCID.h>

#include <ggadget/common.h>
#include <ggadget/digest_utils.h>
#include <ggadget/light_map.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>

using ggadget::LightMap;

#define NO_NAMESPACE
#include "browser_child.h"
#include "browser_child_internal.h"

#if JS_VERSION < 180
extern "C" {
  struct JSTracer;
}
#endif

static const size_t kInvalidBrowserId = static_cast<size_t>(-1);

// Default down and ret fds are standard input and up fd is standard output.
// The default values are useful when browser child is tested independently.
static int g_down_fd = 0, g_up_fd = 1, g_log_fd = 2;

struct BrowserObjectInfo {
  BrowserObjectInfo()
      : browser_id(0), object_id(0), js_context(NULL), js_object(NULL) { }
  size_t browser_id;
  size_t object_id;
  JSContext *js_context;
  JSObject *js_object;
};

typedef LightMap<size_t, BrowserObjectInfo> BrowserObjectMap;
class HostObjectWrapper;
typedef LightMap<size_t, HostObjectWrapper *> HostObjectMap;

struct BrowserInfo {
  BrowserInfo()
      : embed(NULL), browser_id(0), check_load_timer(0),
        always_open_new_window(true) {
  }
  GtkMozEmbed *embed;
  size_t browser_id;
  BrowserObjectMap browser_objects;
  HostObjectMap host_objects;
  int check_load_timer;
  bool always_open_new_window;
};

static size_t g_browser_object_seq = 0;

typedef LightMap<size_t, BrowserInfo> BrowserMap;
static BrowserMap g_browsers;

// The singleton GtkMozEmbed instance for temporary use when a new window
// is requested. Though we don't actually allow new window, we still need
// this widget to allow us get the url of the window and open it in the
// browser.
static GtkMozEmbed *g_embed_for_new_window = NULL;
// The parent window of the above widget.
static GtkWidget *g_popup_for_new_window = NULL;
// The GtkMozEmbed instance which just fired the new window request.
static GtkMozEmbed *g_main_embed_for_new_window = NULL;

static const size_t kMaxBrowsers = 64;

static std::string g_down_buffer, g_reply;

#define EXTNAMESET_CLASSNAME  "ExternalNameSet"
#define EXTNAMESET_CONTRACTID "@google.com/ggl/extnameset;1"
#define EXTNAMESET_CID { \
    0x224fb7b5, 0x6db0, 0x48db, \
    { 0xb8, 0x1e, 0x85, 0x15, 0xe7, 0x9f, 0x00, 0x30 } \
}
// The name of the global "external" object that allows the browser to
// communicate with the host.
#define EXTOBJ_PROPERTY_NAME "external"

#define CONTENT_POLICY_CLASSNAME "ContentPolicy"
#define CONTENT_POLICY_CONTRACTID "@google.com/ggl/content-policy;1"
#define CONTENT_POLICY_CID { \
    0x74d0deec, 0xb36b, 0x4b03, \
    { 0xb0, 0x09, 0x36, 0xe3, 0x07, 0x68, 0xc8, 0x2c } \
}

static const char kDataURLPrefix[] = "data:";

static const nsIID kIScriptGlobalObjectIID = NS_ISCRIPTGLOBALOBJECT_IID;

static const std::string kUndefinedStr("undefined");
static const std::string kNullStr("null");
static const std::string kTrueStr("true");
static const std::string kFalseStr("false");

static void SendLog(const char *format, ...) PRINTF_ATTRIBUTE(1, 2);
static void SendLog(const char *format, ...) {
#ifdef _DEBUG
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "browser_child: ");
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  fflush(stderr);
  va_end(ap);
#endif
}

static BrowserInfo *GetBrowserInfo(size_t browser_id) {
  BrowserMap::iterator it = g_browsers.find(browser_id);
  return it == g_browsers.end() ? NULL : &it->second;
}

static BrowserInfo *FindBrowserByJSContext(JSContext *cx) {
  JSObject *js_global = JS_GetGlobalObject(cx);
  if (!js_global) {
    SendLog("No global object\n");
    return NULL;
  }

  JSClass *cls = JS_GET_CLASS(cx, js_global);
  if (!cls || ((~cls->flags) & (JSCLASS_HAS_PRIVATE |
                                JSCLASS_PRIVATE_IS_NSISUPPORTS))) {
    SendLog("Global object is not a nsISupports\n");
    return NULL;
  }
  nsIXPConnectWrappedNative *global_wrapper =
    reinterpret_cast<nsIXPConnectWrappedNative *>(JS_GetPrivate(cx, js_global));
  nsISupports *global = global_wrapper->Native();

  nsresult rv;
  for (BrowserMap::iterator it = g_browsers.begin(); it != g_browsers.end();
       ++it) {
    nsCOMPtr<nsIWebBrowser> browser;
    gtk_moz_embed_get_nsIWebBrowser(it->second.embed, getter_AddRefs(browser));
    nsCOMPtr<nsIInterfaceRequestor> req(do_QueryInterface(browser, &rv));
    NS_ENSURE_SUCCESS(rv, NULL);
    nsISupports *global1 = NULL;
    void *temp = global1;
    rv = req->GetInterface(kIScriptGlobalObjectIID, &temp);
    global1 = static_cast<nsISupports *>(temp);
    NS_ENSURE_SUCCESS(rv, NULL);
    global1->Release();
    if (global1 == global)
      return &it->second;
  }
  SendLog("Can't find GtkMozEmbed from JS context\n");
  return NULL;
}

static void ForceQuit(const char *message) {
  g_log_fd = 2;
  SendLog("%s. Exiting...", message);
  gtk_main_quit();
  exit(1);
}

static std::string SendFeedbackBuffer(const std::string &buffer) {
  if (write(g_up_fd, buffer.c_str(), buffer.size()) !=
      static_cast<ssize_t>(buffer.size())) {
    // Might already be forced quit on SIGPIPE.
    ForceQuit("Failed to send feedback buffer");
    return "";
  }
  SendLog("<-- SendFeedback: %.80s...", buffer.c_str());

  g_reply.clear();
  while (!gtk_main_iteration() && g_reply.empty());

  if (g_reply.length() < kReplyPrefixLength + 1)
    ForceQuit(("Failed to read feedback reply: " + g_reply).c_str());
  // Remove the reply prefix and ending '\n'.
  std::string reply = g_reply.substr(kReplyPrefixLength,
                                     g_reply.size() - 1 - kReplyPrefixLength);
  SendLog("--> SendFeedback reply: %.40s...", reply.c_str());
  g_reply.clear();
  return reply;
}

static std::string SendFeedback(size_t browser_id, const char *type, ...) {
  va_list ap;
  va_start(ap, type);
  std::string buffer(type);
  buffer += ggadget::StringPrintf("\n%zu", browser_id);

  const char *param;
  while ((param = va_arg(ap, const char *)) != NULL) {
    buffer += '\n';
    buffer += param;
  }
  buffer += kEndOfMessageFull;
  va_end(ap);
  return SendFeedbackBuffer(buffer);
}

static size_t AddBrowserObject(BrowserInfo *browser_info, JSContext *cx,
                               JSObject *js_object) {
  BrowserObjectInfo *info =
      &browser_info->browser_objects[++g_browser_object_seq];
  info->browser_id = browser_info->browser_id;
  info->js_context = cx;
  info->js_object = js_object;
  info->object_id = g_browser_object_seq;
  JS_AddRoot(cx, &info->js_object);
  return g_browser_object_seq;
}

static jsval AddOrGetHostObject(BrowserInfo *browser_info, JSContext *cx,
                                size_t object_id);

static jsval DecodeJSValue(BrowserInfo *browser_info, JSContext *cx,
                           const char *str) {
  char first_char = str[0];
  jsval value = JSVAL_VOID;
  if (isdigit(first_char)) {
    double double_value = strtod(str, NULL);
    if (ceil(double_value) - double_value == 0 &&
        double_value >= JSVAL_INT_MIN && double_value <= JSVAL_INT_MAX) {
      value = INT_TO_JSVAL(static_cast<int32>(double_value));
    } else {
      value = DOUBLE_TO_JSVAL(JS_NewDouble(cx, double_value));
    }
  } else if (first_char == '"' || first_char == '\'') {
    ggadget::UTF16String s;
    if (ggadget::DecodeJavaScriptString(str, &s))
      value = STRING_TO_JSVAL(JS_NewUCStringCopyN(cx, s.c_str(), s.size()));
  } else if (strncmp(str, "hobj ", 5) == 0) {
    size_t object_id = static_cast<size_t>(strtol(str + 5, NULL, 10));
    value = AddOrGetHostObject(browser_info, cx, object_id);
  } else if (kTrueStr == str) {
    value = JSVAL_TRUE;
  } else if (kFalseStr == str) {
    value = JSVAL_FALSE;
  } else if (kNullStr == str) {
    value = JSVAL_NULL;
  }
  return value;
}

static JSBool DecodeJSValueCheckingException(BrowserInfo *browser_info,
                                             JSContext *cx, const char *str,
                                             jsval *result) {
  *result = DecodeJSValue(browser_info, cx, str);
  if (JSVAL_IS_VOID(*result) && strncmp(str, "exception: ", 11) == 0) {
    JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, str)));
    return JS_FALSE;
  }
  return JS_TRUE;
}

static std::string EncodeJSValue(BrowserInfo *browser_info, JSContext *cx,
                                 jsval value) {
  if (JSVAL_IS_VOID(value))
    return kUndefinedStr;
  if (JSVAL_IS_NULL(value))
    return kNullStr;
  if (JSVAL_IS_BOOLEAN(value))
    return JSVAL_TO_BOOLEAN(value) ? kTrueStr : kFalseStr;
  if (JSVAL_IS_INT(value))
    return ggadget::StringPrintf("%d", static_cast<int>(JSVAL_TO_INT(value)));
  if (JSVAL_IS_DOUBLE(value)) {
    jsdouble double_val = 0;
    JS_ValueToNumber(cx, value, &double_val);
    return ggadget::StringPrintf("%g", static_cast<double>(double_val));
  }
  if (JSVAL_IS_STRING(value)) {
    JSString *js_string = JS_ValueToString(cx, value);
    if (!js_string)
      return kNullStr;
    jschar *chars = JS_GetStringChars(js_string);
    return chars ? ggadget::EncodeJavaScriptString(chars, '"') : kNullStr;
  }
  if (JSVAL_IS_OBJECT(value)) {
    return ggadget::StringPrintf("wobj %zu",
        AddBrowserObject(browser_info, cx, JSVAL_TO_OBJECT(value)));
  }
  return kUndefinedStr;
}

static JSBool GetHostObjectProperty(BrowserInfo *browser_info, JSContext *cx,
                                    const char *object_id_str,
                                    jsval property_id, jsval *value) {
  // TODO: Handle toString() and valueOf() locally.
  std::string property = EncodeJSValue(browser_info, cx, property_id);
  std::string result = SendFeedback(browser_info->browser_id,
                                    kGetPropertyFeedback, object_id_str,
                                    property.c_str(), NULL);
  return DecodeJSValueCheckingException(browser_info, cx, result.c_str(),
                                        value);
}

static JSBool SetHostObjectProperty(BrowserInfo *browser_info, JSContext *cx,
                                    const char *object_id_str,
                                    jsval property_id, jsval value) {
  std::string property = EncodeJSValue(browser_info, cx, property_id);
  std::string value_str = EncodeJSValue(browser_info, cx, value);
  std::string result = SendFeedback(browser_info->browser_id,
                                    kSetPropertyFeedback, object_id_str,
                                    property.c_str(), value_str.c_str(), NULL);
  // kSetPropertyFeedback returns empty string or an exception.
  jsval dummy;
  return result.empty() ||
         DecodeJSValueCheckingException(browser_info, cx, result.c_str(),
                                        &dummy);
}

static JSBool CallHostObject(BrowserInfo *browser_info, JSContext *cx,
                             const char *object_id_str,
                             const char *this_object_id_str,
                             uintN argc, jsval *argv, jsval *rval) {
  std::string buffer(kCallFeedback);
  ggadget::StringAppendPrintf(&buffer, "\n%zu\n%s\n%s",
                              browser_info->browser_id, object_id_str,
                              this_object_id_str);
  for (uintN i = 0; i < argc; i++) {
    buffer += '\n';
    buffer += EncodeJSValue(browser_info, cx, argv[i]);
  }
  buffer += kEndOfMessageFull;
  std::string result = SendFeedbackBuffer(buffer);
  return DecodeJSValueCheckingException(browser_info, cx, result.c_str(), rval);
}

class HostObjectWrapper {
 public:
  HostObjectWrapper(size_t browser_id, JSContext *cx, size_t object_id)
      : browser_id_(browser_id),
        object_id_(object_id),
        object_id_str_(ggadget::StringPrintf("%zu", object_id)),
        js_object_(JS_NewObject(cx, &kHostObjectClass, NULL, NULL)) {
    JS_SetPrivate(cx, js_object_, this);
  }

  // Get the HostObjectWrapper from a JS wrapped object.
  // The HostObjectWrapper pointer is stored in the object's private slot.
  static HostObjectWrapper *GetWrapperFromJS(JSContext *cx, JSObject *obj) {
    if (obj) {
      JSClass *cls = JS_GET_CLASS(cx, obj);
      if (cls && cls->getProperty == kHostObjectClass.getProperty &&
          cls->setProperty == kHostObjectClass.setProperty) {
        HostObjectWrapper *wrapper =
            reinterpret_cast<HostObjectWrapper *>(JS_GetPrivate(cx, obj));
        if (wrapper)
          ASSERT(wrapper->js_object_ == obj);
        return wrapper;
      }
    }
    return NULL;
  }

  static JSBool GetWrapperProperty(JSContext *cx, JSObject *obj, jsval id,
                                   jsval *vp) {
    HostObjectWrapper *wrapper = GetWrapperFromJS(cx, obj);
    BrowserInfo *browser_info;
    return wrapper && (browser_info = GetBrowserInfo(wrapper->browser_id_)) &&
           GetHostObjectProperty(browser_info, cx,
                                 wrapper->object_id_str_.c_str(), id, vp);
  }

  static JSBool SetWrapperProperty(JSContext *cx, JSObject *obj, jsval id,
                                   jsval *vp) {
    HostObjectWrapper *wrapper = GetWrapperFromJS(cx, obj);
    BrowserInfo *browser_info;
    return wrapper && (browser_info = GetBrowserInfo(wrapper->browser_id_)) &&
           SetHostObjectProperty(browser_info, cx,
                                 wrapper->object_id_str_.c_str(), id, *vp);
  }

  static JSBool CallWrapperSelf(JSContext *cx, JSObject *obj,
                                uintN argc, jsval *argv, jsval *rval) {
    if (JS_IsExceptionPending(cx))
      return JS_FALSE;
    HostObjectWrapper *this_wrapper = GetWrapperFromJS(cx, obj);
    const char *this_id_str = this_wrapper ?
                              this_wrapper->object_id_str_.c_str() : "";
    // In this case, the real self object being called is at argv[-2].
    JSObject *self_object = JSVAL_TO_OBJECT(argv[-2]);
    HostObjectWrapper *wrapper = GetWrapperFromJS(cx, self_object);
    BrowserInfo *browser_info;
    return wrapper && (browser_info = GetBrowserInfo(wrapper->browser_id_)) &&
           CallHostObject(browser_info, cx, wrapper->object_id_str_.c_str(),
                          this_id_str, argc, argv, rval);
  }

  static void FinalizeWrapper(JSContext *cx, JSObject *obj) {
    HostObjectWrapper *wrapper = GetWrapperFromJS(cx, obj);
    if (wrapper) {
      BrowserInfo *browser_info = GetBrowserInfo(wrapper->browser_id_);
      if (browser_info) {
        SendFeedback(wrapper->browser_id_, kUnrefFeedback,
                     wrapper->object_id_str_.c_str(), NULL);
        browser_info->host_objects.erase(wrapper->object_id_);
      }
      delete wrapper;
    }
  }

  size_t browser_id_;
  size_t object_id_;
  std::string object_id_str_;
  JSObject *js_object_;
  static JSClass kHostObjectClass;
};

// This JSClass is used to create wrapper JSObjects.
JSClass HostObjectWrapper::kHostObjectClass = {
  "NativeJSWrapper",
  // Use the private slot to store the wrapper.
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  GetWrapperProperty, SetWrapperProperty,
  JS_EnumerateStub, JS_ResolveStub,
  JS_ConvertStub, FinalizeWrapper,
  NULL, NULL, CallWrapperSelf, NULL,
  NULL, NULL, NULL, NULL,
};

static jsval AddOrGetHostObject(BrowserInfo *browser_info, JSContext *cx,
                                size_t object_id) {
  HostObjectMap::iterator it = browser_info->host_objects.find(object_id);
  if (it == browser_info->host_objects.end()) {
    HostObjectWrapper *wrapper = new HostObjectWrapper(browser_info->browser_id,
                                                       cx, object_id);
    browser_info->host_objects[object_id] = wrapper;
    return OBJECT_TO_JSVAL(wrapper->js_object_);
  }
  return OBJECT_TO_JSVAL(it->second->js_object_);
}

static void ReportJSError(JSContext *cx, const char *message,
                          JSErrorReport *report) {
  SendLog("browser script line %d: %s", report->lineno, message);
}

class ExternalNameSet : public nsIScriptExternalNameSet {
 public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD InitializeNameSet(nsIScriptContext *script_context) {
    JSContext *cx = GetJSContext(script_context);
    JSObject *global = JS_GetGlobalObject(cx);
    NS_ENSURE_TRUE(global, NS_ERROR_FAILURE);
    BrowserInfo *browser_info = FindBrowserByJSContext(cx);
    if (!browser_info) {
      // This context might me in a frame which needs no window.external.
      return NS_OK;
    }
    JS_SetErrorReporter(cx, ReportJSError);
    HostObjectWrapper *external_wrapper =
        new HostObjectWrapper(browser_info->browser_id, cx, 0);
    jsval js_val = OBJECT_TO_JSVAL(external_wrapper->js_object_);
    JS_SetProperty(cx, global, EXTOBJ_PROPERTY_NAME, &js_val);
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(ExternalNameSet, nsIScriptExternalNameSet)
NS_GENERIC_FACTORY_CONSTRUCTOR(ExternalNameSet)
static const nsModuleComponentInfo kExternalNameSetComponentInfo = {
  EXTNAMESET_CLASSNAME, EXTNAMESET_CID, EXTNAMESET_CONTRACTID,
  ExternalNameSetConstructor
};

static BrowserInfo *FindBrowserByContentPolicyContext(nsISupports *context,
                                                      PRBool *is_loading) {
  nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(context));
  nsresult rv = NS_OK;
  if (!window) {
    nsCOMPtr<nsIDOMDocument> document(do_QueryInterface(context));
    if (!document) {
      nsCOMPtr<nsIDOMNode> node(do_QueryInterface(context, &rv));
      NS_ENSURE_SUCCESS(rv, NULL);
      node->GetOwnerDocument(getter_AddRefs(document));
    }
    nsCOMPtr<nsIDOMDocumentView> docview(do_QueryInterface(document, &rv));
    NS_ENSURE_SUCCESS(rv, NULL);
    nsCOMPtr<nsIDOMAbstractView> view;
    rv = docview->GetDefaultView(getter_AddRefs(view));
    NS_ENSURE_SUCCESS(rv, NULL);
    window = do_QueryInterface(view);
  }

  nsCOMPtr<nsIDOMWindow> top_window;
  window->GetTop(getter_AddRefs(top_window));

  *is_loading = PR_FALSE;
  for (BrowserMap::iterator it = g_browsers.begin(); it != g_browsers.end();
       ++it) {
    nsCOMPtr<nsIWebBrowser> browser;
    gtk_moz_embed_get_nsIWebBrowser(it->second.embed, getter_AddRefs(browser));
    nsCOMPtr<nsIDOMWindow> window1;
    rv = browser->GetContentDOMWindow(getter_AddRefs(window1));
    NS_ENSURE_SUCCESS(rv, NULL);
    if (window == window1 || top_window == window1) {
      nsCOMPtr<nsIWebProgress> progress(do_GetInterface(browser));
      if (progress)
        progress->GetIsLoadingDocument(is_loading);
      return &it->second;
    }
  }
  SendLog("Can't find GtkMozEmbed from ContentPolicy context.");
  return NULL;
}

class ContentPolicy : public nsIContentPolicy, public nsIChannelEventSink {
 public:
  NS_DECL_ISUPPORTS

  static const PRUint32 kTypeRedirect = 999;

  NS_IMETHOD ShouldLoad(PRUint32 content_type, nsIURI *content_location,
                        nsIURI *request_origin, nsISupports *context,
                        const nsACString &mime_type_guess, nsISupports *extra,
                        PRInt16 *retval) {
    NS_ENSURE_ARG_POINTER(content_location);
    NS_ENSURE_ARG_POINTER(retval);
    nsCString url_spec, origin_spec;
    content_location->GetSpec(url_spec);
    if (content_type == TYPE_DOCUMENT && g_embed_for_new_window) {
      // Handle a new window request.
      gtk_widget_destroy(g_popup_for_new_window);
      g_popup_for_new_window = NULL;
      g_embed_for_new_window = NULL;
      for (BrowserMap::const_iterator it = g_browsers.begin();
           it != g_browsers.end(); ++it) {
        if (it->second.embed == g_main_embed_for_new_window) {
          SendFeedback(it->first, kOpenURLFeedback, url_spec.get(), NULL);
          break;
        }
      }
      // Reject this URL no matter if the controller has opened it.
      *retval = REJECT_OTHER;
      return NS_OK;
    }

    *retval = ACCEPT;
    PRBool is_loading = PR_FALSE;
    BrowserInfo *browser_info = FindBrowserByContentPolicyContext(
        context, &is_loading);
    if (!browser_info) {
      return NS_OK;
    }

    if (strncmp(url_spec.get(), "about:neterror", 14) == 0) {
      std::string r = SendFeedback(browser_info->browser_id,
                                   kNetErrorFeedback, url_spec.get(),
                                   NULL);
      if (r[0] != '0')
        *retval = REJECT_OTHER;
      return NS_OK;
    }

    if (content_type == TYPE_DOCUMENT || content_type == TYPE_SUBDOCUMENT ||
        content_type == kTypeRedirect) {
      if (content_type == kTypeRedirect ||
          !browser_info->always_open_new_window) {
        std::string r = SendFeedback(browser_info->browser_id,
                                     kGoToURLFeedback, url_spec.get(),
                                     NULL);
        // The controller should have opened the URL, so don't let the
        // embedded browser open it.
        if (r[0] != '0')
          *retval = REJECT_OTHER;
        return NS_OK;
      }

      if (request_origin)
        request_origin->GetSpec(origin_spec);

      SendLog("ShouldLoad: is_loading=%d\n"
              " origin: %s\n"
              "    url: %s", is_loading, origin_spec.get(), url_spec.get());

      // If the URL is opened the first time in a blank window or frame,
      // or the URL is dragged and dropped on the browser, request_origin
      // is NULL or "about:blank".
      if (!request_origin || origin_spec.Equals(nsCString("about:blank"))) {
        if (!browser_info->check_load_timer) {
          // Reject the request that is not initiated by the controller.
          // It may be initiated by user drag-and-drop.
          *retval = REJECT_OTHER;
        }
        // Otherwise let the URL loaded in place.
        return NS_OK;
      }

      nsCString url_scheme;
      content_location->GetScheme(url_scheme);
      if (url_scheme.Equals(nsCString("javascript"))) {
        // Also let javascript URLs handled in place.
        return NS_OK;
      }

      // Treats urls with the same base url but different refs as equal.
      std::string tmp_origin(origin_spec.get());
      std::string tmp_url(url_spec.get());
      tmp_origin = tmp_origin.substr(0, tmp_origin.find_last_of('#'));
      tmp_url = tmp_url.substr(0, tmp_url.find_last_of('#'));
      // Allow URLs opened during page loading to be opened in place, otherwise
      // call the kOpenURLFeedback to let the controller handle.
      if (tmp_origin != tmp_url && !is_loading) {
        std::string r = SendFeedback(browser_info->browser_id,
                                     kOpenURLFeedback, url_spec.get(),
                                     NULL);
        // The controller should have opened the URL, so don't let the
        // embedded browser open it.
        if (r[0] != '0')
          *retval = REJECT_OTHER;
      }
    }
    return NS_OK;
  }

  NS_IMETHOD ShouldProcess(PRUint32 content_type, nsIURI *content_location,
                           nsIURI *request_origin, nsISupports *context,
                           const nsACString & mime_type, nsISupports *extra,
                           PRInt16 *retval) {
    return ShouldLoad(content_type, content_location, request_origin,
                      context, mime_type, extra, retval);
  }

  NS_IMETHOD OnChannelRedirect(nsIChannel *old_channel, nsIChannel *new_channel,
                               PRUint32 flags) {
    nsCOMPtr<nsIURI> uri;
    nsresult rv = new_channel->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    rv = new_channel->GetNotificationCallbacks(getter_AddRefs(callbacks));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDOMWindow> window(do_GetInterface(callbacks));
    if (window) {
      PRInt16 should_load;
      rv = ShouldLoad(kTypeRedirect, uri, nsnull, window,
                      NS_LITERAL_CSTRING(""), nsnull, &should_load);
      NS_ENSURE_SUCCESS(rv, rv);
      return should_load == ACCEPT ? NS_OK : NS_ERROR_FAILURE;
    }
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS2(ContentPolicy, nsIContentPolicy, nsIChannelEventSink)

static ContentPolicy g_content_policy;
static ContentPolicy *GetContentPolicy() {
  g_content_policy.AddRef();
  return &g_content_policy;
}
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(ContentPolicy, GetContentPolicy)

static const nsModuleComponentInfo kContentPolicyComponentInfo = {
  CONTENT_POLICY_CLASSNAME, CONTENT_POLICY_CID, CONTENT_POLICY_CONTRACTID,
  ContentPolicyConstructor
};

static void OnNewWindow(GtkMozEmbed *embed, GtkMozEmbed **retval,
                        gint chrome_mask, gpointer data) {
  if (!GTK_IS_WIDGET(g_embed_for_new_window)) {
    // Create a hidden GtkMozEmbed widget.
    g_embed_for_new_window = GTK_MOZ_EMBED(gtk_moz_embed_new());
    // The GtkMozEmbed widget needs a parent window.
    g_popup_for_new_window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_container_add(GTK_CONTAINER(g_popup_for_new_window),
                      GTK_WIDGET(g_embed_for_new_window));
    gtk_window_resize(GTK_WINDOW(g_popup_for_new_window), 1, 1);
    gtk_window_move(GTK_WINDOW(g_popup_for_new_window), -10000, -10000);
    gtk_widget_realize(GTK_WIDGET(g_embed_for_new_window));
  }
  // Use the widget temporarily to let our ContentPolicy handle the request.
  *retval = g_embed_for_new_window;
  g_main_embed_for_new_window = embed;
}

static void RemoveBrowser(size_t id) {
  BrowserMap::iterator it = g_browsers.find(id);
  if (it == g_browsers.end()) {
    // Already removed.
    return;
  }

  SendLog("RemoveBrowser: %zu", id);
  for (BrowserObjectMap::iterator obj_it = it->second.browser_objects.begin();
       obj_it != it->second.browser_objects.end(); ++obj_it) {
    JS_RemoveRoot(obj_it->second.js_context, &obj_it->second.js_object);
  }
  it->second.browser_objects.clear();
  // Host object wrappers will be deleted when their references from browser
  // JS context are released.
  it->second.host_objects.clear();

  GtkMozEmbed *embed = it->second.embed;
  g_browsers.erase(it);
  if (GTK_IS_WIDGET(embed)) {
    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(embed));
    if (GTK_IS_WIDGET(parent)) {
      // In case of standalone testing.
      gtk_widget_destroy(parent);
    } else {
      gtk_widget_destroy(GTK_WIDGET(embed));
    }
  }
}

static void OnBrowserDestroy(GtkObject *object, gpointer user_data) {
  RemoveBrowser(reinterpret_cast<size_t>(user_data));
}

static void OnNetStop(GtkMozEmbed *embed, gpointer data) {
  size_t browser_id = reinterpret_cast<size_t>(data);
  SendLog("**** OnNetStop browser=%zu", browser_id);
  BrowserMap::iterator it = g_browsers.find(browser_id);
  if (it != g_browsers.end() && it->second.check_load_timer) {
    g_source_remove(it->second.check_load_timer);
    it->second.check_load_timer = 0;
  }
}

static void OnNetState(GtkMozEmbed *embed, gint state, guint, gpointer data) {
  static const gint kStateMask = nsIWebProgressListener::STATE_STOP |
                                 nsIWebProgressListener::STATE_IS_REQUEST;
  if ((state & kStateMask) == kStateMask) {
    // The current document itself has been loaded.
    OnNetStop(embed, data);
  }
}

static gboolean CheckContentLoaded(gpointer data) {
  size_t browser_id = reinterpret_cast<size_t>(data);
  BrowserMap::iterator it = g_browsers.find(browser_id);
  if (it != g_browsers.end() && it->second.check_load_timer) {
    it->second.check_load_timer = 0;
    // It's weird that sometimes gtk_moz_embed_load_url from local file may
    // cause the browser-child enter a state that no more url can be loaded.
    // FIXME: better solution than restarting the child.
    ForceQuit(ggadget::StringPrintf(
        "Load url from data/local file blocked: %zu", browser_id).c_str());
  }
  return FALSE;
}

static void NewBrowser(int param_count, const char **params, size_t id) {
  if (param_count != 3) {
    SendLog("Incorrect param count for %s: 3 expected, %d given.",
            kNewBrowserCommand, param_count);
    return;
  }

  // The new id can be less than or equals to the current size.
  if (g_browsers.size() >= kMaxBrowsers) {
    SendLog("Too many browsers: %zu.", id);
    return;
  }

  SendLog("NewBrowser: %zu", id);
  BrowserInfo *browser_info = &g_browsers[id];
  if (browser_info->embed) {
    SendLog("Warning: new browser id slot is not empty: %zu.", id);
    RemoveBrowser(id);
  }

  browser_info->browser_id = id;
  GdkNativeWindow socket_id = static_cast<GdkNativeWindow>(strtol(params[2],
                                                                  NULL, 0));
  GtkWidget *window = socket_id ? gtk_plug_new(socket_id) :
                      gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(window, "destroy", G_CALLBACK(OnBrowserDestroy),
                   reinterpret_cast<gpointer>(id));
  GtkMozEmbed *embed = GTK_MOZ_EMBED(gtk_moz_embed_new());
  browser_info->embed = embed;
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(embed));
  g_signal_connect(embed, "new_window", G_CALLBACK(OnNewWindow), NULL);

  // net_stop is not enough by itself, because if the loaded document contains
  // other sub contents or documents, net_stop is only called after all
  // sub contents/documents are loaded.
  // net_state can capture the event that this document itself has finished
  // loading.
  g_signal_connect(embed, "net_stop", G_CALLBACK(OnNetStop),
                   reinterpret_cast<gpointer>(id));
  g_signal_connect(embed, "net_state", G_CALLBACK(OnNetState),
                   reinterpret_cast<gpointer>(id));
  gtk_widget_show_all(window);
}

static GtkMozEmbed *GetGtkEmbedByBrowserId(size_t id) {
  BrowserMap::const_iterator it = g_browsers.find(id);
  if (it == g_browsers.end()) {
    SendLog("GetGtkEmbedByBrowserId: Invalid browser id %zu.", id);
    return NULL;
  }

  GtkMozEmbed *embed = it->second.embed;
  if (!GTK_IS_WIDGET(embed)) {
    SendLog("Invalid browser by id %zu.", id);
    RemoveBrowser(id);
    return NULL;
  }
  return embed;
}

static void SetContent(int param_count, const char **params, size_t id) {
  if (param_count != 4) {
    SendLog("Incorrect param count for %s: 4 expected, %d given.",
            kSetContentCommand, param_count);
    return;
  }

  GtkMozEmbed *embed = GetGtkEmbedByBrowserId(id);
  if (!embed)
    return;

  // params[2]: mime type; params[3]: JSON encoded content string.
  std::string content;
  if (!ggadget::DecodeJavaScriptString(params[3], &content)) {
    SendLog("Invalid JavaScript string: %s", params[3]);
    return;
  }

  std::string url;
  std::string temp_path;
  if (strcmp(params[2], "text/html") == 0) {
    // Let the browser load the HTML content from local file, to raise its
    // privilege so that the content can access local resources.
    if (!ggadget::CreateTempDirectory("browser-child", &temp_path)) {
      SendLog("Failed to create temporary directory");
      return;
    }
    url = ggadget::BuildFilePath(temp_path.c_str(), "content.html", NULL);
    if (!ggadget::WriteFileContents(url.c_str(), content)) {
      SendLog("Failed to write content to file");
      ggadget::RemoveDirectory(temp_path.c_str(), true);
      return;
    }
    url = "file://" + url;
  } else {
    std::string data;
    if (!ggadget::EncodeBase64(content, false, &data)) {
      SendLog("Unable to convert to base64: %s", content.c_str());
      return;
    }
    url = std::string(kDataURLPrefix) + params[2] + ";base64," + data;
  }
  SendLog("Content URL: %.80s...", url.c_str());
  if (g_browsers[id].check_load_timer) {
    g_source_remove(g_browsers[id].check_load_timer);
    g_browsers[id].check_load_timer = 0;
  }
  // Normally data: and file: urls should be loaded immediately, but weirdly
  // sometimes it fails (not related to removing the file). Schedule a timer
  // to check the failure. The timer is required because OnProgress isn't
  // called synchronously.
  if (!ggadget::TrimString(content).empty()) {
    g_browsers[id].check_load_timer =
        g_timeout_add(2000, CheckContentLoaded, reinterpret_cast<gpointer>(id));
  }
  gtk_moz_embed_load_url(embed, url.c_str());

  // The load should finish immediately, so it's safe to delete the file now.
  if (!temp_path.empty())
    ggadget::RemoveDirectory(temp_path.c_str(), true);
}

static void OpenURL(int param_count, const char **params, size_t id) {
  if (param_count != 3) {
    SendLog("Incorrect param count for %s: 3 expected, %d given.",
            kOpenURLCommand, param_count);
    return;
  }

  GtkMozEmbed *embed = GetGtkEmbedByBrowserId(id);
  if (!embed)
    return;
  // params[2]: URL.
  gtk_moz_embed_load_url(embed, params[2]);
}

static std::string GetBrowserObjectProperty(BrowserInfo *browser_info,
                                            int param_count,
                                            const char **params,
                                            const BrowserObjectInfo &info) {
  if (param_count != 4) {
    SendLog("Incorrect param count for %s: 4 expected, %d given.",
            kGetPropertyCommand, param_count);
    return "";
  }

  // params[3]: property key.
  ggadget::UTF16String name;
  jsval result;
  JSBool ret = JS_FALSE;
  if (ggadget::DecodeJavaScriptString(params[3], &name)) {
    // param[3] is a property name string.
    ret = JS_GetUCProperty(info.js_context, info.js_object,
                           name.c_str(), name.size(), &result);
  } else {
    // param[3] is a index integer.
    ret = JS_GetElement(info.js_context, info.js_object, atoi(params[3]),
                        &result);
  }
  if (ret)
    return EncodeJSValue(browser_info, info.js_context, result);
  return ggadget::StringPrintf(
      "exception: Failed to get browser object %zu property %s",
      info.object_id, params[3]);
}

static std::string SetBrowserObjectProperty(BrowserInfo *browser_info,
                                            int param_count,
                                            const char **params,
                                            const BrowserObjectInfo &info) {
  if (param_count != 5) {
    SendLog("Incorrect param count for %s: 5 expected, %d given.",
            kSetPropertyCommand, param_count);
    return "";
  }

  // params[3]: property key, params[4]: property value.
  jsval value = DecodeJSValue(browser_info, info.js_context, params[4]);
  ggadget::UTF16String name;
  JSBool ret = JS_FALSE;
  if (ggadget::DecodeJavaScriptString(params[3], &name)) {
    // param[3] is a property name string.
    ret = JS_SetUCProperty(info.js_context, info.js_object,
                           name.c_str(), name.size(), &value);
  } else {
    // param[3] is a index integer.
    ret = JS_SetElement(info.js_context, info.js_object, atoi(params[3]),
                        &value);
  }
  if (ret) return "";
  return ggadget::StringPrintf(
      "exception: Failed to set browser object %zu property %s",
      info.object_id, params[3]);
}

static std::string CallBrowserObject(BrowserInfo *browser_info,
                                     int param_count, const char **params,
                                     const BrowserObjectInfo &info) {
  if (param_count < 4) {
    SendLog("Incorrect param count for %s: at least 4 expected, %d given.",
            kCallCommand, param_count);
    return "";
  }

  size_t this_object_id = static_cast<size_t>(strtol(params[3], NULL, 0));
  BrowserObjectMap::iterator it =
      browser_info->browser_objects.find(this_object_id);
  JSObject *this_object = it == browser_info->browser_objects.end() ?
                          NULL : it->second.js_object;

  int argc = param_count - 4;
  jsval *argv = new jsval[argc];
  for (int i = 0; i < argc; i++)
    argv[i] = DecodeJSValue(browser_info, info.js_context, params[4 + i]);

  jsval result;
  if (JS_CallFunctionValue(info.js_context, this_object,
                           OBJECT_TO_JSVAL(info.js_object),
                           static_cast<uintN>(argc), argv, &result)) {
    delete [] argv;
    return EncodeJSValue(browser_info, info.js_context, result);
  }
  delete [] argv;
  return ggadget::StringPrintf("exception: Failed to call browser object %zu",
                               info.object_id);
}

static void SetAlwaysOpenNewWindow(int param_count, const char **params,
                                   size_t id) {
  if (param_count != 3) {
    SendLog("Incorrect param count for %s: 3 expected, %d given.",
            kSetAlwaysOpenNewWindowCommand, param_count);
    return;
  }

  BrowserInfo *info = GetBrowserInfo(id);
  if (!info)
    return;

  info->always_open_new_window = (params[2][0] == '1');
}

static void ProcessCommand(int param_count, const char **params) {
  std::string result(kReplyPrefix);
  if (strcmp(params[0], kQuitCommand) == 0) {
    g_log_fd = 2;
    gtk_main_quit();
    return;
  }

  if (param_count < 2) {
    SendLog("No enough command parameter\n");
  } else {
    size_t id = static_cast<size_t>(strtol(params[1], NULL, 0));
    if (strcmp(params[0], kNewBrowserCommand) == 0) {
      NewBrowser(param_count, params, id);
    } else {
      BrowserMap::iterator it = g_browsers.find(id);
      if (it == g_browsers.end()) {
        SendLog("Invalid browser id: %zu (command %s)", id, params[0]);
      } else {
        BrowserInfo *browser_info = &it->second;
        if (strcmp(params[0], kSetContentCommand) == 0) {
          SetContent(param_count, params, id);
        } else if (strcmp(params[0], kOpenURLCommand) == 0) {
          OpenURL(param_count, params, id);
        } else if (strcmp(params[0], kCloseBrowserCommand) == 0) {
          RemoveBrowser(id);
        } else if (strcmp(params[0], kSetAlwaysOpenNewWindowCommand) == 0) {
          SetAlwaysOpenNewWindow(param_count, params, id);
        } else if (param_count < 3) {
          SendLog("No enough command parameters or invalid command: %s\n",
                  params[0]);
        } else {
          size_t object_id = static_cast<size_t>(strtol(params[2], NULL, 0));
          BrowserObjectMap::iterator it =
              browser_info->browser_objects.find(object_id);
          if (it == browser_info->browser_objects.end()) {
            SendLog("Browser object not found: %zu", object_id);
          } else {
            const BrowserObjectInfo &info = it->second;
            ASSERT(info.js_object && info.js_context);
            ASSERT(info.object_id == object_id);
            if (info.browser_id != id) {
              SendLog("Browser id of browser object mismatch: %zu vs %zu",
                      id, info.browser_id);
            } else if (strcmp(params[0], kGetPropertyCommand) == 0) {
              result += GetBrowserObjectProperty(browser_info, param_count,
                                                 params, info);
            } else if (strcmp(params[0], kSetPropertyCommand) == 0) {
              result += SetBrowserObjectProperty(browser_info, param_count,
                                                 params, info);
            } else if (strcmp(params[0], kCallCommand) == 0) {
              result += CallBrowserObject(browser_info, param_count, params,
                                          info);
            } else if (strcmp(params[0], kUnrefCommand) == 0) {
              JS_RemoveRoot(info.js_context, &it->second.js_object);
              browser_info->browser_objects.erase(object_id);
            } else {
              SendLog("Invalid command: %s", params[0]);
            }
          }
        }
      }
    }
  }

  SendLog("ProcessCommand: %d %s(%.20s,%.20s,%.20s,%.20s,%.20s,%.20s) "
          "result: %.40s...", param_count, params[0],
          param_count > 1 ? params[1] : "", param_count > 2 ? params[2] : "",
          param_count > 3 ? params[3] : "", param_count > 4 ? params[4] : "",
          param_count > 5 ? params[5] : "", param_count > 6 ? params[6] : "",
          result.c_str());
  result += '\n';
  if (write(g_up_fd, result.c_str(), result.size()) !=
      static_cast<ssize_t>(result.size())) {
    SendLog("Failed to send back result.");
  }
}

// Read the down pipe.
// If any command received, processes it; if any reply received, save it into
// g_reply. If only partial command received, saves into g_down_buffer.
static gboolean OnDownFDReady(GIOChannel *channel, GIOCondition condition,
                              gpointer data) {
  if (condition != G_IO_IN) {
    ForceQuit("Down pipe error or hanged up");
    return FALSE;
  }

  char buffer[4096];
  ssize_t read_bytes;
  while ((read_bytes = read(g_down_fd, buffer, sizeof(buffer))) > 0) {
    g_down_buffer.append(buffer, read_bytes);
    if (read_bytes < static_cast<ssize_t>(sizeof(buffer)))
      break;
  }
  if (read_bytes <= 0) {
    // Because we ensure up_fd_ has data before calling OnDownFDReady(),
    // read() should not return 0.
    ForceQuit("Failed to read from down pipe");
    return FALSE;
  }

  // In rare cases that g_down_buffer can contain more than one messages.
  // For example, controller sends a command immediately after a ping reply.
  while (true) {
    if (strncmp(g_down_buffer.c_str(), kReplyPrefix, kReplyPrefixLength) == 0) {
      // This message is a reply message.
      size_t eom_pos = g_down_buffer.find('\n');
      if (eom_pos == g_down_buffer.npos)
        break;
      g_reply = g_down_buffer.substr(0, eom_pos + 1);
      g_down_buffer.erase(0, eom_pos + 1);
    } else {
      size_t eom_pos = g_down_buffer.find(kEndOfMessageFull);
      if (eom_pos == g_down_buffer.npos)
        break;

      std::string message(g_down_buffer, 0, eom_pos + kEOMFullLength);
      g_down_buffer.erase(0, eom_pos + kEOMFullLength);

      static const size_t kMaxParams = 20;
      size_t curr_pos = 0;
      size_t param_count = 0;
      const char *params[kMaxParams];
      while (curr_pos <= eom_pos) {
        size_t end_of_line_pos = message.find('\n', curr_pos);
        ASSERT(end_of_line_pos != message.npos);
        message[end_of_line_pos] = '\0';
        if (param_count < kMaxParams) {
          params[param_count] = message.c_str() + curr_pos;
          param_count++;
        } else {
          SendLog("Too many up message parameter");
          // Don't exit to recover from the error status.
        }
        curr_pos = end_of_line_pos + 1;
      }
      NS_ASSERTION(curr_pos = eom_pos + 1, "");
      if (param_count > 0)
        ProcessCommand(param_count, params);
    }
  }
  return TRUE;
}

static void OnSigPipe(int sig) {
  ForceQuit("SIGPIPE occurred");
}

static gboolean CheckController(gpointer data) {
  std::string buffer(kPingFeedback);
  buffer += kEndOfMessageFull;
  std::string result = SendFeedbackBuffer(buffer);
  if (result != kPingAck)
    ForceQuit("Ping failed");
  return TRUE;
}

static nsresult InitCustomComponents() {
  nsCOMPtr<nsIComponentRegistrar> registrar;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsICategoryManager> category_manager =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Register external object (Javascript window.external object).
  nsCOMPtr<nsIGenericFactory> factory;
  factory = do_CreateInstance ("@mozilla.org/generic-factory;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  factory->SetComponentInfo(&kExternalNameSetComponentInfo);
  rv = registrar->RegisterFactory(kExternalNameSetComponentInfo.mCID,
                                  EXTNAMESET_CLASSNAME, EXTNAMESET_CONTRACTID,
                                  factory);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = category_manager->AddCategoryEntry(
      JAVASCRIPT_GLOBAL_STATIC_NAMESET_CATEGORY, EXTNAMESET_CLASSNAME,
      EXTNAMESET_CONTRACTID, PR_FALSE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Register customized content policy.
  g_content_policy.AddRef();
  factory = do_CreateInstance ("@mozilla.org/generic-factory;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  factory->SetComponentInfo(&kContentPolicyComponentInfo);
  rv = registrar->RegisterFactory(kContentPolicyComponentInfo.mCID,
                                  CONTENT_POLICY_CLASSNAME,
                                  CONTENT_POLICY_CONTRACTID,
                                  factory);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = category_manager->AddCategoryEntry("content-policy",
                                          CONTENT_POLICY_CONTRACTID,
                                          CONTENT_POLICY_CONTRACTID,
                                          PR_FALSE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = category_manager->AddCategoryEntry("net-channel-event-sinks",
                                          CONTENT_POLICY_CONTRACTID,
                                          CONTENT_POLICY_CONTRACTID,
                                          PR_FALSE, PR_TRUE, nsnull);
  return rv;
}

static bool InitGecko() {
#ifdef XPCOM_GLUE
  nsresult rv;

  static const GREVersionRange kGREVersion = {
    "1.9a", PR_TRUE,
    "1.9.4", PR_FALSE,
  };

  char xpcom_location[4096];
  rv = GRE_GetGREPathWithProperties(&kGREVersion, 1, nsnull, 0,
                                    xpcom_location, 4096);
  if (NS_FAILED(rv)) {
    g_warning("Failed to find proper Gecko Runtime Environment!");
    return false;
  }

  printf("XPCOM location: %s\n", xpcom_location);

  // Startup the XPCOM Glue that links us up with XPCOM.
  rv = XPCOMGlueStartup(xpcom_location);
  if (NS_FAILED(rv)) {
    g_warning("Failed to startup XPCOM Glue!");
    return false;
  }

  rv = GTKEmbedGlueStartup();
  if (NS_FAILED(rv)) {
    g_warning("Failed to startup Gtk Embed Glue!");
    return false;
  }

  rv = GTKEmbedGlueStartupInternal();
  if (NS_FAILED(rv)) {
    g_warning("Failed to startup Gtk Embed Glue (internal)!");
    return false;
  }

  rv = ggadget::libmozjs::LibmozjsGlueStartupWithXPCOM();
  if (NS_FAILED(rv)) {
    g_warning("Failed to startup SpiderMonkey Glue!");
    return false;
  }

  char *last_slash = strrchr(xpcom_location, '/');
  if (last_slash)
    *last_slash = '\0';

  gtk_moz_embed_set_path(xpcom_location);
#elif defined(MOZILLA_FIVE_HOME)
  gtk_moz_embed_set_comp_path(MOZILLA_FIVE_HOME);
#endif
  return true;
}

int main(int argc, char **argv) {
  if (!g_thread_supported())
    g_thread_init(NULL);

  gtk_init(&argc, &argv);

  if (!InitGecko()) {
    g_warning("Failed to initialize Gecko.");
    return 1;
  }

  signal(SIGPIPE, OnSigPipe);
  if (argc >= 2)
    g_down_fd = atoi(argv[1]);
  if (argc >= 3)
    g_up_fd = g_log_fd = atoi(argv[2]);
  SendLog("BrowserChild fds: %d %d", g_down_fd, g_up_fd);

  // Set the down FD to non-blocking mode to make the gtk main loop happy.
  int down_fd_flags = fcntl(g_down_fd, F_GETFL);
  down_fd_flags |= O_NONBLOCK;
  fcntl(g_down_fd, F_SETFL, down_fd_flags);

  GIOChannel *channel = g_io_channel_unix_new(g_down_fd);
  int down_fd_watch = g_io_add_watch(
      // Though the document says G_IO_HUP and G_IO_ERR are only for output fds,
      // I found that sometimes the read side of a pipe still generates HUP
      // events, which causes the main loop to use up 100% CPU if I don't
      // watch the events.
      channel, static_cast<GIOCondition>(G_IO_IN | G_IO_HUP | G_IO_ERR),
      OnDownFDReady, NULL);
  GSource *source = g_main_context_find_source_by_id(NULL, down_fd_watch);
  g_source_set_can_recurse(source, TRUE);
  g_io_channel_unref(channel);

  gtk_moz_embed_push_startup();
  InitCustomComponents();

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    // To let mozilla display appropriate error messages on network errors.
    prefs->SetBoolPref("browser.xul.error_pages.enabled", PR_TRUE);
  }

  if (g_down_fd != 0) {
    // Only start ping timer in actual environment to ease testing.
    // Use high priority to ensure the callback is called even if the main
    // loop is busy.
    g_timeout_add_full(G_PRIORITY_HIGH, kPingInterval, CheckController,
                       NULL, NULL);
  }

  gtk_main();
  g_source_remove(down_fd_watch);
  gtk_moz_embed_pop_startup();
  return 0;
}
