/*
  Copyright 2009 Google Inc.

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

#include <cmath>
#include <string>
#include <cstring>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#include "browser_element.h"
#include <ggadget/element_factory.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/view.h>
#include <ggadget/digest_utils.h>
#include <ggadget/system_utils.h>

#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
#include <ggadget/script_runtime_manager.h>
#include "../webkit_script_runtime/js_script_runtime.h"
#include "../webkit_script_runtime/js_script_context.h"
#endif

#define Initialize gtkwebkit_browser_element_LTX_Initialize
#define Finalize gtkwebkit_browser_element_LTX_Finalize
#define RegisterElementExtension \
    gtkwebkit_browser_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize gtkwebkit_browser_element extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize gtkwebkit_browser_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOGI("Register gtkwebkit_browser_element extension, "
         "using name \"_browser\".");
    if (factory) {
      factory->RegisterElementClass(
          "_browser", &ggadget::gtkwebkit::BrowserElement::CreateInstance);
    }
    return true;
  }
}

using namespace ggadget::webkit;

namespace ggadget {
namespace gtkwebkit {

static const char kTempFileName[] = "content.html";

class BrowserElement::Impl {
 public:
  Impl(BrowserElement *owner)
    : content_type_("text/html"),
      owner_(owner),
      web_view_(NULL),
      minimized_connection_(owner_->GetView()->ConnectOnMinimizeEvent(
          NewSlot(this, &Impl::OnViewMinimized))),
      restored_connection_(owner_->GetView()->ConnectOnRestoreEvent(
          NewSlot(this, &Impl::OnViewRestored))),
      popout_connection_(owner_->GetView()->ConnectOnPopOutEvent(
          NewSlot(this, &Impl::OnViewPoppedOut))),
      popin_connection_(owner_->GetView()->ConnectOnPopInEvent(
          NewSlot(this, &Impl::OnViewPoppedIn))),
      dock_connection_(owner_->GetView()->ConnectOnDockEvent(
          NewSlot(this, &Impl::OnViewDockUndock))),
      undock_connection_(owner_->GetView()->ConnectOnUndockEvent(
          NewSlot(this, &Impl::OnViewDockUndock))),
      x_(0),
      y_(0),
      width_(0),
      height_(0),
      popped_out_(false),
      minimized_(false),
      always_open_new_window_(true) {
  }

  ~Impl() {
    // Indicates it's being destroyed.
    owner_ = NULL;

    minimized_connection_->Disconnect();
    restored_connection_->Disconnect();
    popout_connection_->Disconnect();
    popin_connection_->Disconnect();
    dock_connection_->Disconnect();
    undock_connection_->Disconnect();

    GtkWidget *web_view = web_view_;
    // To prevent WebViewDestroyed() from destroying it again.
    web_view_ = NULL;
    if (GTK_IS_WIDGET(web_view)) {
      // Due to a bug of webkit (https://bugs.webkit.org/show_bug.cgi?id=25042)
      // Destroying web view widget directly will cause crash.
      // It must be removed from it's parent before destroying.
      GtkWidget *container = gtk_widget_get_parent(web_view);
      if (container)
        gtk_container_remove(GTK_CONTAINER(container), web_view);

      g_object_run_dispose(G_OBJECT(web_view));
      g_object_unref(web_view);
    }

    if (temp_path_.length()) {
      ggadget::RemoveDirectory(temp_path_.c_str(), true);
    }
  }

  void GetWidgetExtents(gint *x, gint *y, gint *width, gint *height) {
    double widget_x0, widget_y0;
    double widget_x1, widget_y1;
    owner_->SelfCoordToViewCoord(0, 0, &widget_x0, &widget_y0);
    owner_->SelfCoordToViewCoord(owner_->GetPixelWidth(),
                                 owner_->GetPixelHeight(),
                                 &widget_x1, &widget_y1);

    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x0, widget_y0,
                                                    &widget_x0, &widget_y0);
    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x1, widget_y1,
                                                    &widget_x1, &widget_y1);

    *x = static_cast<gint>(round(widget_x0));
    *y = static_cast<gint>(round(widget_y0));
    *width = static_cast<gint>(ceil(widget_x1 - widget_x0));
    *height = static_cast<gint>(ceil(widget_y1 - widget_y0));
  }

  void EnsureBrowser() {
    if (!web_view_) {
      GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
      if (!GTK_IS_FIXED(container)) {
        LOG("BrowserElement needs a GTK_FIXED parent. Actual type: %s",
            G_OBJECT_TYPE_NAME(container));
        return;
      }

      web_view_ = GTK_WIDGET(webkit_web_view_new());
      g_object_ref(web_view_);
      ASSERT(web_view_);

      g_signal_connect(G_OBJECT(web_view_), "destroy",
                       G_CALLBACK(WebViewDestroyed), this);
      g_signal_connect(G_OBJECT(web_view_), "console-message",
                       G_CALLBACK(WebViewConsoleMessage), this);
      g_signal_connect(G_OBJECT(web_view_), "load-started",
                       G_CALLBACK(WebViewLoadStarted), this);
      g_signal_connect(G_OBJECT(web_view_), "load-committed",
                       G_CALLBACK(WebViewLoadCommitted), this);
      g_signal_connect(G_OBJECT(web_view_), "load-progress-changed",
                       G_CALLBACK(WebViewLoadProgressChanged), this);
      g_signal_connect(G_OBJECT(web_view_), "load-finished",
                       G_CALLBACK(WebViewLoadFinished), this);
      g_signal_connect(G_OBJECT(web_view_), "hovering-over-link",
                       G_CALLBACK(WebViewHoveringOverLink), this);
#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
      g_signal_connect(G_OBJECT(web_view_), "window-object-cleared",
                       G_CALLBACK(WebViewWindowObjectCleared), this);
#endif

      WebKitWebWindowFeatures *features =
          webkit_web_view_get_window_features(WEBKIT_WEB_VIEW(web_view_));
      ASSERT(features);
      g_signal_connect(G_OBJECT(features), "notify::width",
                       G_CALLBACK(WebViewWindowWidthNotify), this);
      g_signal_connect(G_OBJECT(features), "notify::height",
                       G_CALLBACK(WebViewWindowHeightNotify), this);

      g_signal_connect(G_OBJECT(web_view_), "create-web-view",
                       G_CALLBACK(WebViewCreateWebView), this);
      g_signal_connect(G_OBJECT(web_view_),
                       "navigation-policy-decision-requested",
                       G_CALLBACK(WebViewNavigationPolicyDecisionRequested),
                       this);

      GetWidgetExtents(&x_, &y_, &width_, &height_);

      gtk_fixed_put(GTK_FIXED(container), web_view_, x_, y_);
      gtk_widget_set_size_request(GTK_WIDGET(web_view_), width_, height_);
      gtk_widget_show(web_view_);

      if (content_.length()) {
        webkit_web_view_load_html_string(WEBKIT_WEB_VIEW(web_view_),
                                         content_.c_str(), "");
      }
    }
  }

  void Layout() {
    EnsureBrowser();
    GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    if (GTK_IS_FIXED(container) && WEBKIT_IS_WEB_VIEW(web_view_)) {
      bool force_layout = false;
      // check if the contain has changed.
      if (gtk_widget_get_parent(web_view_) != container) {
        gtk_widget_reparent(GTK_WIDGET(web_view_), container);
        force_layout = true;
      }

      gint x, y, width, height;
      GetWidgetExtents(&x, &y, &width, &height);

      if (x != x_ || y != y_ || force_layout) {
        x_ = x;
        y_ = y;
        gtk_fixed_move(GTK_FIXED(container), GTK_WIDGET(web_view_), x, y);
      }
      if (width != width_ || height != height_ || force_layout) {
        DLOG("Layout: w:%d, h:%d", width, height);
        width_ = width;
        height_ = height;
        gtk_widget_set_size_request(GTK_WIDGET(web_view_), width, height);
      }
      if (owner_->IsReallyVisible() && (!minimized_ || popped_out_))
        gtk_widget_show(web_view_);
      else
        gtk_widget_hide(web_view_);
    }
  }

  void SetContent(const std::string &content) {
    DLOG("SetContent: %s\n%s", content_type_.c_str(), content.c_str());
    content_ = content;
    if (GTK_IS_WIDGET(web_view_)) {
      std::string url;
      if (content_type_ == "text/html") {
        // Let the browser load the HTML content from local file, to raise its
        // privilege so that the content can access local resources.
        if (!EnsureTempDirectory()) {
          LOG("Failed to create temporary directory.");
          return;
        }
        url = ggadget::BuildFilePath(temp_path_.c_str(), kTempFileName, NULL);
        if (!ggadget::WriteFileContents(url.c_str(), content)) {
          LOG("Failed to write content to file.");
          return;
        }
        url = "file://" + url;
      } else {
        std::string data;
        if (!ggadget::EncodeBase64(content, false, &data)) {
          LOG("Unable to convert content to base64.");
          return;
        }
        url = std::string("data:");
        url.append(content_type_);
        url.append(";base64,");
        url.append(data);
      }
      DLOG("Content URL: %.80s...", url.c_str());
      webkit_web_view_load_uri(WEBKIT_WEB_VIEW(web_view_), url.c_str());
    }
  }

  void SetExternalObject(ScriptableInterface *object) {
    DLOG("SetExternalObject(%p, CLSID=%ju)",
         object, object ? object->GetClassId() : 0);
    external_object_.Reset(object);
    // Changing external object after loading the content is not supported.
    // External object must be set before setting content, so that it can
    // be injected into webpage's javascript context in window-object-cleared
    // signal handler.
  }

  void OnViewMinimized() {
    // The browser widget must be hidden when the view is minimized.
    if (GTK_IS_WIDGET(web_view_) && !popped_out_) {
      gtk_widget_hide(web_view_);
    }
    minimized_ = true;
  }

  void OnViewRestored() {
    if (GTK_IS_WIDGET(web_view_) && owner_->IsReallyVisible() && !popped_out_) {
      gtk_widget_show(web_view_);
    }
    minimized_ = false;
  }

  void OnViewPoppedOut() {
    popped_out_ = true;
    Layout();
  }

  void OnViewPoppedIn() {
    popped_out_ = false;
    Layout();
  }

  void OnViewDockUndock() {
    // The toplevel window might be changed, so it's necessary to reparent the
    // browser widget.
    Layout();
  }

  bool OpenURL(const char *url) {
    GadgetInterface *gadget = owner_->GetView()->GetGadget();
    bool result = false;
    if (gadget) {
      // Let the gadget allow this OpenURL gracefully.
      bool old_interaction = gadget->SetInUserInteraction(true);
      result = gadget->OpenURL(url);
      gadget->SetInUserInteraction(old_interaction);
    }
    return result;
  }

  bool HandleNavigationRequest(const char *old_uri, const char *new_uri) {
    bool result = false;
    if (always_open_new_window_) {
      size_t new_len = strlen(new_uri);
      size_t old_len = strlen(old_uri);
      const char *new_end = strrchr(new_uri, '#');
      if (new_end)
        new_len = new_end - new_uri;
      const char *old_end = strrchr(old_uri, '#');
      if (old_end)
        old_len = old_end - old_uri;
      // Treats urls with the same base url but different refs as equal.
      if (new_len != old_len || strncmp(new_uri, old_uri, new_len) != 0) {
        result = ongotourl_signal_(new_uri, true) || OpenURL(new_uri);
      }
    }
    return result;
  }

  bool EnsureTempDirectory() {
    if (temp_path_.length()) {
      return ggadget::EnsureDirectories(temp_path_.c_str());
    }
    return ggadget::CreateTempDirectory("browser-element", &temp_path_);
  }

  static void WebViewDestroyed(GtkWidget *widget, Impl *impl) {
    DLOG("WebViewDestroyed(Impl=%p, web_view=%p)", impl, widget);

    if (impl->web_view_) {
      g_object_unref(impl->web_view_);
      impl->web_view_ = NULL;
    }
  }

  static gboolean WebViewConsoleMessage(WebKitWebView *web_view,
                                        gchar *message, gint line,
                                        gchar *source_id, Impl *impl) {
    if (!impl->owner_) return FALSE;
    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    LOGI("WebViewConsoleMessage(%s:%d): %s", source_id, line, message);
    return TRUE;
  }

  static void WebViewLoadStarted(WebKitWebView *web_view,
                                 WebKitWebFrame *web_frame,
                                 Impl *impl) {
    if (!impl->owner_) return;
    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    DLOG("WebViewLoadStarted(Impl=%p, web_view=%p, web_frame=%p)",
         impl, web_view, web_frame);
  }

  static void WebViewLoadCommitted(WebKitWebView *web_view,
                                   WebKitWebFrame *web_frame,
                                   Impl *impl) {
    if (!impl->owner_) return;
    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    DLOG("WebViewLoadCommitted(Impl=%p, web_view=%p, web_frame=%p)",
         impl, web_view, web_frame);

    // It's ok to delete the temporary file here, because the file has been
    // opened by webkit.
    if (impl->temp_path_.length()) {
      ggadget::RemoveDirectory(impl->temp_path_.c_str(), true);
      impl->temp_path_.clear();
    }
  }

  static void WebViewLoadProgressChanged(WebKitWebView *web_view,
                                         gint progress,
                                         Impl *impl) {
    if (!impl->owner_) return;
    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    DLOG("WebViewLoadProgressChanged(Impl=%p, web_view=%p, progress=%d)",
         impl, web_view, progress);
  }

  static void WebViewLoadFinished(WebKitWebView *web_view,
                                  WebKitWebFrame *web_frame,
                                  Impl *impl) {
    if (!impl->owner_) return;
    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    DLOG("WebViewLoadFinished(Impl=%p, web_view=%p, web_frame=%p)",
         impl, web_view, web_frame);

    // Seems that webkit won't fire window.resize event after loading the page,
    // but GMail gadget depends on this behavior to layout its compose window.
    static const char kFireWindowResizeEventScript[] =
        "var evtObj_ = document.createEvent('HTMLEvents');"
        "evtObj_.initEvent('resize', false, false);"
        "window.dispatchEvent(evtObj_);";

    webkit_web_view_execute_script(web_view, kFireWindowResizeEventScript);
  }

  static void WebViewHoveringOverLink(WebKitWebView *web_view,
                                      const char *title,
                                      const char *uri,
                                      Impl *impl) {
    if (!impl->owner_) return;
    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    DLOG("WebViewHoveringOverLink(Impl=%p, web_view=%p, title=%s, uri=%s)",
         impl, web_view, title, uri);

    impl->hovering_over_uri_ = uri ? uri : "";
  }

#ifdef GGL_GTK_WEBKIT_SUPPORT_JSC
  static void DestroyJSScriptContext(gpointer context) {
    DLOG("DestroyJSScriptContext(%p)", context);
    delete static_cast<JSScriptContext *>(context);
  }

  static void WebViewWindowObjectCleared(WebKitWebView *web_view,
                                         WebKitWebFrame *web_frame,
                                         JSGlobalContextRef js_context,
                                         JSObjectRef window_object,
                                         Impl *impl) {
    if (!impl->owner_) return;
    DLOG("WebViewWindowObjectCleared(Impl=%p, web_view=%p, web_frame=%p,"
         "js_context=%p, window_object=%p", impl, web_view, web_frame,
         js_context, window_object);
    JSScriptRuntime *runtime = down_cast<JSScriptRuntime *>(
        ScriptRuntimeManager::get()->GetScriptRuntime("webkitjs"));

    if (runtime) {
      ASSERT(webkit_web_frame_get_global_context(web_frame) == js_context);

      JSScriptContext *wrapper = static_cast<JSScriptContext *>(
          g_object_get_data(G_OBJECT(web_frame), "js-context-wrapper"));
      if (!wrapper || wrapper->GetContext() != js_context) {
        wrapper = runtime->WrapExistingContext(js_context);
        DLOG("Create JSScriptContext wrapper: %p", wrapper);
        g_object_set_data_full(G_OBJECT(web_frame), "js-context-wrapper",
                               wrapper, DestroyJSScriptContext);
      }

      wrapper->AssignFromNative(NULL, "window", "external",
                                Variant(impl->external_object_.Get()));
    } else {
      LOGE("webkit-script-runtime is not loaded.");
    }
  }
#endif

  static WebKitWebView* WebViewCreateWebView(WebKitWebView *web_view,
                                             WebKitWebFrame *web_frame,
                                             Impl *impl) {
    if (!impl->owner_) return NULL;
    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    DLOG("WebViewCreateWebView(Impl=%p, web_view=%p, web_frame=%p)",
         impl, web_view, web_frame);

    // FIXME: is it necessary to create a hidden new webview and handle
    // navigation policy of the new webview?
    const char *url = impl->hovering_over_uri_.c_str();
    if (IsValidURL(url) && !impl->ongotourl_signal_(url, true))
      impl->OpenURL(url);

    return NULL;
  }

  static gboolean WebViewNavigationPolicyDecisionRequested(
      WebKitWebView *web_view, WebKitWebFrame *web_frame,
      WebKitNetworkRequest *request, WebKitWebNavigationAction *action,
      WebKitWebPolicyDecision *decision, Impl *impl) {
    if (!impl->owner_) return FALSE;
    const char *new_uri =
        webkit_network_request_get_uri(request);

    // original uri in action is not reliable, especially when the original
    // content has no uri.
    const char *original_uri = impl->loaded_uri_.c_str();

    WebKitWebNavigationReason reason =
        webkit_web_navigation_action_get_reason(action);

    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    DLOG("WebViewNavigationPolicyDecisionRequested"
         "(Impl=%p, web_view=%p, web_frame=%p):\n"
         "  New URI: %s\n"
         "  Reason: %d\n"
         "  Original URI: %s\n"
         "  Button: %d\n"
         "  Modifier: %d",
         impl, web_view, web_frame, new_uri, reason, original_uri,
         webkit_web_navigation_action_get_button(action),
         webkit_web_navigation_action_get_modifier_state(action));

    gboolean result = FALSE;
    if (reason == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED)
      result = impl->HandleNavigationRequest(original_uri, new_uri);

    // If the url was not opened in a new window, then give the gadget a chance
    // to handle the url.
    if (!result)
      result = impl->ongotourl_signal_(new_uri, false);

    if (result)
      webkit_web_policy_decision_ignore(decision);
    else
      impl->loaded_uri_ = new_uri ? new_uri : "";

    return result;
  }

  static void WebViewWindowWidthNotify(WebKitWebWindowFeatures *features,
                                       GParamSpec *param,
                                       Impl *impl) {
    if (!impl->owner_) return;
    gint width = 0;
    g_object_get(features, "width", &width, NULL);
    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    DLOG("WebViewWindowWidthNotify(Impl=%p, width=%d)", impl, width);
  }

  static void WebViewWindowHeightNotify(WebKitWebWindowFeatures *features,
                                        GParamSpec *param,
                                        Impl *impl) {
    if (!impl->owner_) return;
    gint height = 0;
    g_object_get(features, "height", &height, NULL);
    ScopedLogContext log_context(impl->owner_->GetView()->GetGadget());
    DLOG("WebViewWindowHeightNotify(Impl=%p, width=%d)", impl, height);
  }

  std::string content_type_;
  std::string content_;
  std::string hovering_over_uri_;
  std::string loaded_uri_;
  std::string temp_path_;

  BrowserElement *owner_;
  GtkWidget *web_view_;

  Connection *minimized_connection_;
  Connection *restored_connection_;
  Connection *popout_connection_;
  Connection *popin_connection_;
  Connection *dock_connection_;
  Connection *undock_connection_;

  ScriptableHolder<ScriptableInterface> external_object_;

  Signal2<bool, const char *, bool> ongotourl_signal_;

  gint x_;
  gint y_;
  gint width_;
  gint height_;

  bool popped_out_ : 1;
  bool minimized_ : 1;
  bool always_open_new_window_ : 1;
};

BrowserElement::BrowserElement(View *view, const char *name)
  : BasicElement(view, "browser", name, true),
    impl_(new Impl(this)) {
}

BrowserElement::~BrowserElement() {
  delete impl_;
  impl_ = NULL;
}

std::string BrowserElement::GetContentType() const {
  return impl_->content_type_;
}

void BrowserElement::SetContentType(const char *content_type) {
  impl_->content_type_ =
      content_type && *content_type ? content_type : "text/html";
}

void BrowserElement::SetContent(const std::string &content) {
  impl_->SetContent(content);
}

void BrowserElement::SetExternalObject(ScriptableInterface *object) {
  impl_->SetExternalObject(object);
}

bool BrowserElement::IsAlwaysOpenNewWindow() const {
  return impl_->always_open_new_window_;
}

void BrowserElement::SetAlwaysOpenNewWindow(bool always_open_new_window) {
  impl_->always_open_new_window_ = always_open_new_window;
}

void BrowserElement::Layout() {
  BasicElement::Layout();
  impl_->Layout();
}

void BrowserElement::DoDraw(CanvasInterface *canvas) {
}

BasicElement *BrowserElement::CreateInstance(View *view, const char *name) {
  return new BrowserElement(view, name);
}

void BrowserElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("contentType",
                   NewSlot(&BrowserElement::GetContentType),
                   NewSlot(&BrowserElement::SetContentType));
  RegisterProperty("innerText", NULL,
                   NewSlot(&BrowserElement::SetContent));
  RegisterProperty("external", NULL,
                   NewSlot(&BrowserElement::SetExternalObject));
  RegisterProperty("alwaysOpenNewWindow",
                   NewSlot(&BrowserElement::IsAlwaysOpenNewWindow),
                   NewSlot(&BrowserElement::SetAlwaysOpenNewWindow));
  RegisterClassSignal("ongotourl",
                      &Impl::ongotourl_signal_,
                      &BrowserElement::impl_);
}

} // namespace gtkwebkit
} // namespace ggadget
