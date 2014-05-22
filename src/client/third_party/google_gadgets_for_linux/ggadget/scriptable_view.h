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

#ifndef GGADGET_SCRIPTABLE_VIEW_H__
#define GGADGET_SCRIPTABLE_VIEW_H__

#include <string>
#include <ggadget/view.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_interface.h>

namespace ggadget {

class ScriptContextInterface;
class FileManagerInterface;
class Elements;

/**
 * @ingroup View
 *
 * A scriptable wrapper for View class.
 */
class ScriptableView : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xdac9be17eceb47ec, ScriptableInterface);

  /**
   * Construct a ScriptableView instance for a specified View instance, using
   * an optional Scriptable object as prototype and an optional ScriptContext.
   *
   * The newly created ScriptableView instance doesn't own the specified view,
   * prototype and script_context, and it must be destroyed before destroying
   * view, prototype and script_context.
   *
   * @param view A View instance to be wrapped.
   * @param prototype An optional Scriptable object used as the prototype of
   *        the newly created ScriptableView instance.
   * @param script_context An optional ScriptContext instance that can be used
   *        to execute script codes, only required for script based Gadgets.
   */
  ScriptableView(View *view, ScriptableInterface *prototype,
                 ScriptContextInterface *script_context);

  virtual ~ScriptableView();

  /**
   * Initialize the View from specified XML file content.
   *
   * @param xml content of the xml file.
   * @param filename name of the xml file, only for logging purpose.
   * @return true when succeeded.
   */
  bool InitFromXML(const std::string &xml, const char *filename);

  View *view();

 protected:
  virtual void DoRegister();

 private:
  class Impl;
  Impl *impl_;
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableView);
};

} // namespace ggadget

#endif // GGADGET_SCRIPTABLE_VIEW_H__
