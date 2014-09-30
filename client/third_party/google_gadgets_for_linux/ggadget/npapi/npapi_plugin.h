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

#ifndef GGADGET_NPAPI_NPAPI_PLUGIN_H__
#define GGADGET_NPAPI_NPAPI_PLUGIN_H__

#include <string>
#include <ggadget/common.h>
#include <ggadget/math_utils.h>
#include <ggadget/string_utils.h>
#include <ggadget/small_object.h>

struct _NPWindow;

namespace ggadget {

class BasicElement;
class ScriptableInterface;
class Connection;
template <typename R, typename P1> class Slot1;

namespace npapi {

/**
 * @defgroup NPAPILibrary libggadget-npapi - the NPAPI wrapper
 * @ingroup SharedLibraries
 *
 * This shared library contains a wrapper for NPAPI plugins, so that C++ and
 * javascript code can access them easily. The most important usage is to
 * support adobe flash plugin.
 */

/**
 * @ingroup NPAPILibrary
 * Represents a npapi plugin instance.
 */
class Plugin : public SmallObject<> {
 public:
  /**
   * Destroys the plugin instance.
   */
  void Destroy();

  /** Get the name of the plugin. */
  std::string GetName() const;

  /** Get the description of the plugin. */
  std::string GetDescription() const;

  /**
   * Get if the plugin is in windowlesss or windowed mode.
   * The Host should call this first to determine the window type before
   * calling SetWindow.
   */
  bool IsWindowless() const;

  /**
   * Setup the plugin window.
   * The host should reset the window if window meta changes, such as resize,
   * changing view, etc.. This class will make a copy of the window object
   * passed in.
   *
   * @param top_window the native handle of the top level window. On X, this
   *     should be a X Window ID.
   * @param window NPAPI window structure.
   * @return false if the windowless mode of the window parameter doesn't match
   *     the current windowless state of the plugin.
   */
  bool SetWindow(void *top_window, const _NPWindow &window);

  /** Set URL of the stream that will consumed by the plugin. */
//  bool SetURL(const char *url);

  /**
   * Returns true if the plugin is in transparent mode, otherwise returns false.
   */
  bool IsTransparent() const;

  /**
   * Set the data source of the plugin.
   * @param src a http/https or local file URL or a local file path.
   */
  void SetSrc(const char *src);

  /**
   * Delegates a native event to the plugin. Only use this interface for
   * windowless mode, as X server sends events to the plugin directly if the
   * plugin has its own window.
   *
   * @param event point to the native event. On GTK2 platform, the event should
   *     be a pointer to a XEvent struct.
   * @return @c true if the event has been handled by the plugin.
   */
  bool HandleEvent(void *event);

  /**
   * Set handler that will be called when plugin wants to show some
   * status message.
   */
  Connection *ConnectOnNewMessage(Slot1<void, const char *> *handler);

  /**
   * Scriptable entry for the plugin. The host should register this
   * root object as a constant that can be accessed from script such
   * as Javascript.
   */
  ScriptableInterface *GetScriptablePlugin();

  /**
   * Gets and resets the current dirty rectangle that needs to be redrawn next
   * time. If the Rectangle is kWholePluginRect, the whole plugin need to be
   * redrawn.
   */
  Rectangle GetDirtyRect() const;
  void ResetDirtyRect();

  static const Rectangle kWholePluginRect;

 public:
  /**
   * Creates a new Plugin instance.
   * @param mime_type MIME type of the content.
   * @param element the container element.
   * @param top_window see SetWindow().
   * @param window see SetWindow().
   * @param parameters initial arguments.
   */
  static Plugin *Create(const char *mime_type, BasicElement *element,
                        void *top_window, const _NPWindow &window,
                        const StringMap &parameters);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Plugin);
  Plugin();
  ~Plugin();

  class Impl;
  Impl *impl_;
};

} // namespace npapi
} // namespace ggadget

#endif // GGADGET_NPAPI_NPAPI_PLUGIN_H__
