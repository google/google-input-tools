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
#include <QtCore/QtGlobal>
#if QT_VERSION < 0x040400
#include <QtWebKit/qwebview.h>
#include <QtWebKit/qwebpage.h>
#else
#include <QtWebKit/QWebView>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebSettings>
#endif
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/string_utils.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/view.h>
#include <ggadget/element_factory.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/qt/qt_view_widget.h>

#include "browser_element.h"
#include "browser_element_internal.h"

#define Initialize qtwebkit_browser_element_LTX_Initialize
#define Finalize qtwebkit_browser_element_LTX_Finalize
#define RegisterElementExtension \
    qtwebkit_browser_element_LTX_RegisterElementExtension

namespace ggadget {
namespace qt {

BrowserElement::BrowserElement(View *view, const char *name)
    : BasicElement(view, "browser", name, true),
      impl_(new Impl(this)) {
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
}

BrowserElement::~BrowserElement() {
  delete impl_;
  impl_ = NULL;
}

std::string BrowserElement::GetContentType() const {
  return impl_->content_type_;
}

void BrowserElement::SetContentType(const char *content_type) {
  impl_->content_type_ = content_type && *content_type ? content_type :
                         "text/html";
}

void BrowserElement::SetContent(const std::string &content) {
  impl_->SetContent(content);
}

void BrowserElement::SetExternalObject(ScriptableInterface *object) {
  impl_->external_object_.Reset(object);
}

bool BrowserElement::IsAlwaysOpenNewWindow() const {
  return impl_->always_open_new_window_;
}

void BrowserElement::SetAlwaysOpenNewWindow(bool always_open_new_window) {
  impl_->SetAlwaysOpenNewWindow(always_open_new_window);
}

void BrowserElement::Layout() {
  BasicElement::Layout();
  impl_->Layout();
}

void BrowserElement::DoDraw(CanvasInterface *) {
}

BasicElement *BrowserElement::CreateInstance(View *view, const char *name) {
  return new BrowserElement(view, name);
}

} // namespace gtkmoz
} // namespace ggadget

extern "C" {
  bool Initialize() {
    LOGI("Initialize qtwebkit_browser_element extension.");
#if QT_VERSION >= 0x040400
    QWebSettings *setting = QWebSettings::globalSettings();
    setting->setAttribute(QWebSettings::PluginsEnabled, true);
#endif
    return true;
  }

  void Finalize() {
    LOGI("Finalize qtwebkit_browser_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOGI("Register qtwebkit_browser_element extension, using name "
         "\"_browser\".");
    if (factory) {
      factory->RegisterElementClass(
          "_browser", &ggadget::qt::BrowserElement::CreateInstance);
    }
    return true;
  }
}
#include "browser_element_internal.moc"
