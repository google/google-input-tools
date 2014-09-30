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

#ifndef EXTENSIONS_GTKMOZ_BROWSER_ELEMENT_H__
#define EXTENSIONS_GTKMOZ_BROWSER_ELEMENT_H__

#include <stdlib.h>
#include <ggadget/basic_element.h>

namespace ggadget {
namespace gtkmoz {

class BrowserElementImpl;

class BrowserElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xa4fae95864ae4d89, BasicElement);

  BrowserElement(View *view, const char *name);
  virtual ~BrowserElement();

 protected:
  virtual void DoClassRegister();

 public:
  /**
   * Gets and sets the content type (in MIME format).
   * Default content type is "text/html".
   * Change to it doesn't effect the current content, but later SetContent().
   */
  std::string GetContentType() const;
  void SetContentType(const char *content_type);

  /** Sets the content displayed in this element. */
  void SetContent(const std::string &content);

  /**
   * Sets the "external" object that can be accessed from browser's global
   * window scope.
   */
  void SetExternalObject(ScriptableInterface *object);

  /**
   * Gets and sets whether always open a new browser window for each OpenURL
   * request. It's true by default. If set to false, only links with target
   * other than the current window will open a new browser window.
   */
  bool IsAlwaysOpenNewWindow() const;
  void SetAlwaysOpenNewWindow(bool always_open_new_window);

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  virtual void Layout();
  virtual void DoDraw(CanvasInterface *canvas);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(BrowserElement);

  BrowserElementImpl *impl_;
};

} // namespace gtkmoz
} // namespace ggadget

#endif // EXTENSIONS_GTKMOZ_BROWSER_ELEMENT_H__
