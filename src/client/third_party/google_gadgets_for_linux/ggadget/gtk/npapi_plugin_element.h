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

#ifndef GGADGET_GTK_NPAPI_PLUGIN_ELEMENT_H__
#define GGADGET_GTK_NPAPI_PLUGIN_ELEMENT_H__

#include <ggadget/basic_element.h>
#include <ggadget/string_utils.h>

namespace ggadget {
namespace gtk {

class NPAPIPluginElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0xc1303db46eb1469b, BasicElement);

 protected:
  /**
   * Constructor. This class must be subclassed.
   * @param view
   * @param name
   * @param mime_type the plugin mime_type, used to search for appropriate
   *     NPAPI plugin module.
   * @param default_parameters default pairs of parameter names and values.
   * @param in_object_element whether this object is to be used within an
   *     @c ObjectElement. This parameter must not differ among objects of
   *     the same class. You must derive different classes for pure element
   *     and element used in ObjectElement.
   */
  NPAPIPluginElement(View *view, const char *name,
                     const char *mime_type,
                     const StringMap &default_parameters,
                     bool in_object_element);
  virtual ~NPAPIPluginElement();

 public:
  /**
   * Gets and sets the data source of the plugin.
   * Source is a http/https or local file URL or a local file path.
   */
  std::string GetSrc() const;
  void SetSrc(const char *src);

 protected:
  virtual void DoClassRegister();
  virtual void DoRegister();
  virtual void Layout();
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);
  virtual EventResult HandleOtherEvent(const Event &event);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(NPAPIPluginElement);

  class Impl;
  Impl *impl_;
};

} // namespace gtk
} // namespace ggadget

#endif // GGADGET_GTK_NPAPI_PLUGIN_ELEMENT_H__
