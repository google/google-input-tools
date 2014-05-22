/*
  Copyright 2014 Google Inc.

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

#ifndef GOOPY_SKIN_SKIN_H_
#define GOOPY_SKIN_SKIN_H_

#include <string>

// Include basictypes.h before headers of ggadget to avoid macro conflicts.
#include "base/basictypes.h"
#include "third_party/google_gadgets_for_linux/ggadget/basic_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/common.h"
#include "third_party/google_gadgets_for_linux/ggadget/gadget_base.h"
#include "third_party/google_gadgets_for_linux/ggadget/scoped_ptr.h"
#include "third_party/google_gadgets_for_linux/ggadget/string_utils.h"

namespace ggadget {

class ButtonElement;
class Connection;
class MenuInterface;
class Permissions;

template <typename R> class Slot0;
template <typename R, typename P1> class Slot1;
template <typename R, typename P1, typename P2> class Slot2;

}  // namespace ggadget

namespace ime_goopy {
namespace skin {

class CompositionElement;
class CandidateListElement;
class ToolbarElement;

class Skin : public ggadget::GadgetBase {
 public:
  DEFINE_GADGET_TYPE_ID(0xda4561b2c4964559, ggadget::GadgetBase);

  // Types of views owned by a Skin object.
  enum ViewType {
    TOOLBAR_VIEW = 0,
    COMPOSING_VIEW,
    VIRTUAL_KEYBOARD_VIEW,
    VIRTUAL_KEYBOARD_102_VIEW,

    // For counting the number of view types.
    VIEW_TYPE_COUNT,
  };

  // |ui_locale| is the locale name, which will be used when loading localized
  // strings and other resources from the skin package. If it's NULL or empty
  // then the current system locale will be used.
  Skin(ggadget::HostInterface* host,
       const char* base_path,
       const char* options_name,
       const char* ui_locale,
       int instance_id,
       const ggadget::Permissions& global_permissions,
       bool vertical_candidate_layout,
       bool right_to_left_layout);

  virtual ~Skin();

  // Overridden from GadgetInterface:
  virtual void RemoveMe(bool save_data);
  virtual bool IsSafeToRemove() const;
  virtual bool IsValid() const;
  virtual ggadget::FileManagerInterface* GetFileManager() const;
  virtual ggadget::OptionsInterface* GetOptions();
  virtual std::string GetManifestInfo(const char* key) const;
  virtual bool ParseLocalizedXML(const std::string& xml,
                                 const char* filename,
                                 ggadget::DOMDocumentInterface* xmldoc) const;
  // Returns the toolbar view.
  virtual ggadget::View* GetMainView() const;
  // Shows the toolbar view.
  virtual bool ShowMainView();
  // Hides the toolbar view.
  virtual void CloseMainView();

  virtual bool HasAboutDialog() const;
  virtual void ShowAboutDialog();
  virtual bool HasOptionsDialog() const;
  virtual bool ShowOptionsDialog();
  virtual void OnAddCustomMenuItems(ggadget::MenuInterface* menu);
  virtual const ggadget::Permissions* GetPermissions() const;

  // Additional methods for IME features.

  void SetVerticalCandidateLayout(bool vertical_candidate_layout);
  void SetRightToLeftLayout(bool right_to_left);

  // Gets/shows/closes the composing view.
  ggadget::View* GetComposingView() const;
  bool ShowComposingView();
  void CloseComposingView();

  // Gets the virtual keyboard view.
  ggadget::View* GetVirtualKeyboardView() const;
  ggadget::View* GetVirtualKeyboard102View() const;

  void CloseAllViews();

  // Gets a named element in a specified view.
  ggadget::BasicElement* GetElementByName(ViewType view_type, const char* name);

  template<typename T>
  T* GetElementByNameAndType(ViewType view_type, const char* name);

  enum ImageResult {
    // No image found.
    IMAGE_NOT_FOUND = 0,
    // The image is found in the skin package.
    IMAGE_FOUND_IN_SKIN,
    // The image is found in the global resource package.
    IMAGE_FOUND_IN_GLOBAL,
  };

  // Gets an image path by its name. This method first searches in the skin
  // package to see if it provides an alternative image file for the name,
  // then searches in the global resource package. If the image is found, then
  // its patch will be stored into |*image_path| and either IMAGE_FOUND_IN_SKIN
  // or IMAGE_FOUND_IN_GLOBAL will be returned, otherwise IMAGE_NOT_FOUND will
  // be returned.
  ImageResult GetImagePathByName(const char* name,
                                 std::string* image_path) const;

  // A convenient method to set a named button's images by given image names.
  // Returns true if success.
  bool SetNamedButtonImagesByNames(ViewType view_type,
                                   const char* button_name,
                                   const char* image,
                                   const char* down_image,
                                   const char* over_image,
                                   const char* disabled_image);

  // A convenient method to set a button's images by given image names.
  // Returns true if success.
  bool SetButtonImagesByNames(ggadget::ButtonElement* button,
                              const char* image,
                              const char* down_image,
                              const char* over_image,
                              const char* disabled_image);

  // Sets a named element's visible state.
  void SetElementVisibleByName(ViewType view_type,
                               const char* name,
                               bool visible);

  // Sets a named element's enable state.
  void SetElementEnabledByName(ViewType view_type,
                               const char* name,
                               bool enabled);

  // Sets the message to be shown in the help message label.
  // TODO(suzhe): Investigate how to handle it in a more generic way.
  void SetHelpMessage(ViewType view, const char* message);

  // Connects a slot to the signal that will be fired when the input method's
  // global menu should be shown. The parameter is a MenuInterface object to be
  // filled with the menu items.
  ggadget::Connection* ConnectOnShowImeMenu(
      ggadget::Slot1<void, ggadget::MenuInterface*>* handler);

  typedef ggadget::Slot2<void, ggadget::BasicElement*, ggadget::MenuInterface*>
      ElementContextMenuEventSlot;

  // Connects a slot to the signal that will be fired when the given element's
  // context menu event gets fired. The parameter is a MenuInterface object to
  // be filled with the menu items.
  static ggadget::Connection* ConnectOnElementContextMenuEvent(
      ggadget::BasicElement* element,
      ElementContextMenuEventSlot* handler);

  // A utility to get the manifest infomation of an input method skin without
  // constructing an Skin object. Returns true if the content of manifest
  // file is successfully loaded into the given StringMap |*data|.
  static bool GetSkinManifest(const char* base_path,
                              ggadget::StringMap* data);

  // Locale version of GetSkinManifest. You can specify a different locale
  // than the system locale.
  static bool GetSkinManifestForLocale(const char* base_path,
                                       const char* locale,
                                       ggadget::StringMap* data);

  // A utility to get an FileManagerInterface of an input method skin without
  // constructing an Skin object. If |*locale| is NULL or empty, then the
  // system locale will be used.
  static ggadget::FileManagerInterface* GetSkinFileManagerForLocale(
      const char* base_path,
      const char* locale);

  // Checks if a skin package is valid or not by actually loading it.
  static bool ValidateSkinPackage(const char* base_path, const char* ui_locale);

 private:
  // Gets/shows/closes specified view.
  ggadget::View* GetView(ViewType view_type) const;
  bool ShowView(ViewType view_type);
  void CloseView(ViewType view_type);

  class Impl;
  ggadget::scoped_ptr<Impl> impl_;

  DISALLOW_COPY_AND_ASSIGN(Skin);
};

template<typename T>
T* Skin::GetElementByNameAndType(ViewType view_type, const char* name) {
  ggadget::BasicElement* e = GetElementByName(view_type, name);
  return (e && e->IsInstanceOf(T::CLASS_ID)) ? ggadget::down_cast<T*>(e) : NULL;
}

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_SKIN_H_
