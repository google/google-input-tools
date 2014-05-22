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

#include "skin/skin.h"

#include "skin/candidate_list_element.h"
#include "skin/composition_element.h"
#include "skin/skin_consts.h"
#include "skin/toolbar_element.h"

#include "third_party/google_gadgets_for_linux/ggadget/button_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/element_factory.h"
#include "third_party/google_gadgets_for_linux/ggadget/event.h"
#include "third_party/google_gadgets_for_linux/ggadget/file_manager_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/file_manager_factory.h"
#include "third_party/google_gadgets_for_linux/ggadget/file_manager_wrapper.h"
#include "third_party/google_gadgets_for_linux/ggadget/gadget_consts.h"
#include "third_party/google_gadgets_for_linux/ggadget/host_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/label_element.h"
#include "third_party/google_gadgets_for_linux/ggadget/locales.h"
#include "third_party/google_gadgets_for_linux/ggadget/localized_file_manager.h"
#include "third_party/google_gadgets_for_linux/ggadget/logger.h"
#include "third_party/google_gadgets_for_linux/ggadget/menu_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/messages.h"
#include "third_party/google_gadgets_for_linux/ggadget/options_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/permissions.h"
#include "third_party/google_gadgets_for_linux/ggadget/script_context_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_event.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_helper.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_menu.h"
#include "third_party/google_gadgets_for_linux/ggadget/scriptable_view.h"
#include "third_party/google_gadgets_for_linux/ggadget/signals.h"
#include "third_party/google_gadgets_for_linux/ggadget/system_utils.h"
#include "third_party/google_gadgets_for_linux/ggadget/text_frame.h"
#include "third_party/google_gadgets_for_linux/ggadget/view.h"
#include "third_party/google_gadgets_for_linux/ggadget/view_host_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/xml_dom.h"
#include "third_party/google_gadgets_for_linux/ggadget/xml_parser_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/xml_utils.h"

namespace ime_goopy {
namespace skin {

// TODO(haicsun): Separate virtual keyboard UI and skin UI.
namespace {
// Types of views owned by a Skin object.
enum InternalViewType {
  LTR_TOOLBAR_VIEW = 0,
  LTR_HORIZONTAL_COMPOSING_VIEW,
  LTR_VERTICAL_COMPOSING_VIEW,
  RTL_TOOLBAR_VIEW,
  RTL_HORIZONTAL_COMPOSING_VIEW,
  RTL_VERTICAL_COMPOSING_VIEW,
  LTR_VIRTUAL_KEYBOARD_VIEW,
  LTR_VIRTUAL_KEYBOARD_102_VIEW,

  // For counting the number of view types.
  LAST_VIEW_TYPE = LTR_VIRTUAL_KEYBOARD_102_VIEW,
};

// A dummy ViewHostInterface implementation for validating a skin package.
class DummyViewHost : public ggadget::ViewHostInterface {
 public:
  explicit DummyViewHost(ggadget::ViewHostInterface::Type type)
      : type_(type),
        view_(NULL) {
  }
  virtual ~DummyViewHost() { }
  virtual Type GetType() const { return type_; }
  virtual void Destroy() { delete this; }
  virtual void SetView(ggadget::ViewInterface* view) { view_ = view; }
  virtual ggadget::ViewInterface* GetView() const { return view_; }
  virtual ggadget::GraphicsInterface* NewGraphics() const { return NULL; }
  virtual void* GetNativeWidget() const { return NULL; }
  virtual void ViewCoordToNativeWidgetCoord(double, double,
                                            double* , double*) const { }
  virtual void NativeWidgetCoordToViewCoord(double, double,
                                            double* , double*) const { }
  virtual void QueueDraw() { }
  virtual void QueueResize() { }
  virtual void EnableInputShapeMask(bool) { }
  virtual void SetResizable(ggadget::ViewInterface::ResizableMode) { }
  virtual void SetCaption(const std::string&) { }
  virtual void SetShowCaptionAlways(bool) { }
  virtual void SetCursor(ggadget::ViewInterface::CursorType) { }
  virtual void ShowTooltip(const std::string&) { }
  virtual void ShowTooltipAtPosition(const std::string&, double, double) { }
  virtual bool ShowView(bool, int, ggadget::Slot1<bool, int>* handler) {
    delete handler;
    return false;
  }
  virtual void CloseView() { }
  virtual bool ShowContextMenu(int) { return false; }
  virtual void Alert(const ggadget::ViewInterface*, const char*) { }
  virtual ConfirmResponse Confirm(const ggadget::ViewInterface*,
                                  const char*, bool) {
    return CONFIRM_NO;
  }
  virtual std::string Prompt(const ggadget::ViewInterface*,
                             const char*, const char*) {
    return std::string();
  }
  virtual int GetDebugMode() const {
    return ggadget::ViewInterface::DEBUG_DISABLED;
  }
  virtual void SetWindowPosition(int x, int y) { }
  virtual void GetWindowPosition(int* x, int* y) { }
  virtual void GetWindowSize(int* width, int* height) { }
  virtual void SetFocusable(bool focusable) { }
  virtual void SetOpacity(double opacity) { }
  virtual void SetFontScale(double scale) { }
  virtual void SetZoom(double zoom) { }
  virtual ggadget::Connection* ConnectOnEndMoveDrag(
      ggadget::Slot2<void, int, int>* handler) {
    return NULL;
  }
  virtual ggadget::Connection* ConnectOnShowContextMenu(
      ggadget::Slot1<bool, ggadget::MenuInterface*>* handler) {
    return NULL;
  }
  virtual void BeginResizeDrag(int, ggadget::ViewInterface::HitTest) { }
  virtual void BeginMoveDrag(int) { }

 private:
  Type type_;
  ggadget::ViewInterface* view_;

  DISALLOW_EVIL_CONSTRUCTORS(DummyViewHost);
};

// A dummy HostInterface implementation for validating a skin package.
class DummyHost : public ggadget::HostInterface {
 public:
  DummyHost() {}
  virtual ~DummyHost() { }
  virtual ggadget::ViewHostInterface* NewViewHost(
      ggadget::GadgetInterface*, ggadget::ViewHostInterface::Type type) {
    return new DummyViewHost(type);
  }
  virtual ggadget::GadgetInterface* LoadGadget(
      const char*, const char*, int, bool) {
    return NULL;
  }
  virtual void RemoveGadget(ggadget::GadgetInterface* gadget, bool) {
    delete gadget;
  }
  virtual bool LoadFont(const char*) { return false; }
  virtual void ShowGadgetDebugConsole(ggadget::GadgetInterface*) { }
  virtual int GetDefaultFontSize() { return 0; }
  virtual bool OpenURL(const ggadget::GadgetInterface*, const char*) {
    return false;
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(DummyHost);
};

const char* kSupportedImageSuffixes[] = {
  ".png",
  ".jpg",
  ".bmp",
};

const char kValidationOptionsName[] = "skin-validation-options";

struct ViewInfo {
  InternalViewType type;
  const char* xml;
  const char* fallback_xml;
};

const ViewInfo kViewsInfo[] = {
  { LTR_TOOLBAR_VIEW, kToolbarViewXML, NULL },
  { LTR_HORIZONTAL_COMPOSING_VIEW,
    kHorizontalComposingViewXML, kComposingViewXML },
  { LTR_VERTICAL_COMPOSING_VIEW,
    kVerticalComposingViewXML, kComposingViewXML },
  { RTL_TOOLBAR_VIEW, kRTLToolbarViewXML, NULL },
  { RTL_HORIZONTAL_COMPOSING_VIEW,
    kRTLHorizontalComposingViewXML, kRTLComposingViewXML },
  { RTL_VERTICAL_COMPOSING_VIEW,
    kRTLVerticalComposingViewXML, kRTLComposingViewXML },
  { LTR_VIRTUAL_KEYBOARD_VIEW,
    kVirtualKeyboardViewXML, NULL },
  { LTR_VIRTUAL_KEYBOARD_102_VIEW,
    kVirtualKeyboard102ViewXML, kVirtualKeyboardViewXML },
};

struct ElementInfo {
  Skin::ViewType view_type;
  const char* name;
  uint64_t class_id;
};

const ElementInfo kMandatoryElementsInfo[] = {
  { Skin::COMPOSING_VIEW, kCompositionElement,
    CompositionElement::CLASS_ID },
  { Skin::COMPOSING_VIEW, kCandidateListElement,
    CandidateListElement::CLASS_ID },
  { Skin::COMPOSING_VIEW, kHelpMessageLabel,
    ggadget::LabelElement::CLASS_ID },
};

}  // namespace

class Skin::Impl : public ggadget::ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x4871833db6f246cd, ggadget::ScriptableInterface);

  // A class to bundle View, ScriptableView and ScriptContextInterface together.
  class ViewBundle {
   public:
    ViewBundle(ggadget::ViewHostInterface* view_host,
               Skin* skin,
               ggadget::ElementFactory* element_factory)
        : view_(new ggadget::View(view_host, skin, element_factory, NULL)),
          scriptable_(new ggadget::ScriptableView(view_.get(), NULL, NULL)) {
    }

    ~ViewBundle() {
    }

    ggadget::View* view() { return view_.get(); }
    ggadget::ScriptableView* scriptable() { return scriptable_.get(); }

   private:
    ggadget::scoped_ptr<ggadget::View> view_;
    ggadget::scoped_ptr<ggadget::ScriptableView> scriptable_;

    DISALLOW_EVIL_CONSTRUCTORS(ViewBundle);
  };

  // A special event slot implementation to forward context menu event to a slot
  // that accepts a ggadget::MenuInterface* parameter.
  class ContextMenuEventSlot : public ggadget::Slot0<void> {
   public:
    ContextMenuEventSlot(
        ggadget::BasicElement* element,
        ElementContextMenuEventSlot* handler)
        : element_(element),
          handler_(handler) {
      ASSERT(element_);
      ASSERT(handler_);
    }

    virtual ~ContextMenuEventSlot() {
      delete handler_;
    }

    virtual ggadget::ResultVariant Call(ScriptableInterface* object,
                                        int argc,
                                        const ggadget::Variant argv[]) const {
      ASSERT(argc == 0);

      ggadget::ScriptableEvent* scriptable_event =
          element_->GetView()->GetEvent();
      ASSERT(scriptable_event);
      ASSERT(scriptable_event->GetEvent()->GetType() ==
             ggadget::Event::EVENT_CONTEXT_MENU);

      const ggadget::Event* event = scriptable_event->GetEvent();
      const ggadget::ContextMenuEvent* menu_event =
          ggadget::down_cast<const ggadget::ContextMenuEvent*>(event);
      (*handler_)(element_, menu_event->GetMenu()->GetMenu());

      // Prevent Skin::OnAddCustomMenuItems() from being called, so that we
      // won't mix IME menu items with specific context menus, such as soft
      // keyboard context menu.
      scriptable_event->SetReturnValue(ggadget::EVENT_RESULT_CANCELED);
      return ggadget::ResultVariant();
    }

    virtual bool operator==(const Slot& another) const {
      const ContextMenuEventSlot* a =
          ggadget::down_cast<const ContextMenuEventSlot*>(&another);
      return a && a == this && a->element_ == element_ &&
          *a->handler_ == *handler_;
    }

   private:
    ggadget::BasicElement* element_;
    ElementContextMenuEventSlot* handler_;

    DISALLOW_EVIL_CONSTRUCTORS(ContextMenuEventSlot);
  };

  Impl(Skin* owner,
       const char* base_path,
       const char* options_name,
       const char* locale,
       const ggadget::Permissions& global_permissions,
       bool vertical_candidate_layout,
       bool right_to_left_layout)
      : owner_(owner),
        base_path_(base_path ? base_path : ""),
        locale_(locale && *locale ? locale : ggadget::GetSystemLocaleName()),
        permissions_(global_permissions),
        element_factory_(new ggadget::ElementFactory()),
        file_manager_(new ggadget::FileManagerWrapper()),
        options_(ggadget::CreateOptions(options_name)),
        initialized_(false),
        vertical_candidate_layout_(vertical_candidate_layout),
        right_to_left_layout_(right_to_left_layout) {
    // Checks if necessary objects are created successfully.
    ASSERT(element_factory_.get());
    ASSERT(file_manager_.get());
    ASSERT(options_.get());

    // Register IME Skin related elements.
    element_factory_->RegisterElementClass(
        "composition", &CompositionElement::CreateInstance);
    element_factory_->RegisterElementClass(
        "candidatelist", &CandidateListElement::CreateInstance);
    element_factory_->RegisterElementClass(
        "toolbar", &ToolbarElement::CreateInstance);
  }

  ~Impl() {
  }

  ggadget::HostInterface* GetHost() const {
    return owner_->GetHost();
  }

  // Loads a view from specified xml files.
  bool LoadView(ViewBundle* view, const char* xml, const char* fallback_xml) {
    std::string view_xml;
    const char* file = NULL;
    if (file_manager_->ReadFile(xml, &view_xml))
      file = xml;
    else if (fallback_xml && file_manager_->ReadFile(fallback_xml, &view_xml))
      file = fallback_xml;

    if (file && view->scriptable()->InitFromXML(view_xml, file))
      return true;

    return false;
  }

  bool LoadView(ViewType view_type) {
    InternalViewType internal_view_type = LTR_TOOLBAR_VIEW;
    if (view_type == TOOLBAR_VIEW) {
      if (right_to_left_layout_)
        internal_view_type = RTL_TOOLBAR_VIEW;
      else
        internal_view_type = LTR_TOOLBAR_VIEW;
    } else if (view_type == COMPOSING_VIEW) {
      if (right_to_left_layout_) {
        if (vertical_candidate_layout_)
          internal_view_type = RTL_VERTICAL_COMPOSING_VIEW;
        else
          internal_view_type = RTL_HORIZONTAL_COMPOSING_VIEW;
      } else {
        if (vertical_candidate_layout_)
          internal_view_type = LTR_VERTICAL_COMPOSING_VIEW;
        else
          internal_view_type = LTR_HORIZONTAL_COMPOSING_VIEW;
      }
    } else if (view_type == VIRTUAL_KEYBOARD_VIEW) {
      internal_view_type = LTR_VIRTUAL_KEYBOARD_VIEW;
    } else if (view_type == VIRTUAL_KEYBOARD_102_VIEW) {
      internal_view_type = LTR_VIRTUAL_KEYBOARD_102_VIEW;
    }

    ggadget::scoped_ptr<ViewBundle> view(new ViewBundle(
        GetHost()->NewViewHost(
        owner_, ggadget::ViewHostInterface::VIEW_HOST_MAIN),
            owner_, element_factory_.get()));
    if (!LoadView(view.get(), kViewsInfo[internal_view_type].xml,
                  kViewsInfo[internal_view_type].fallback_xml) &&
        internal_view_type != LTR_VIRTUAL_KEYBOARD_VIEW &&
        internal_view_type != LTR_VIRTUAL_KEYBOARD_102_VIEW) {
      view->view()->Alert(ggadget::StringPrintf(
          GML_("IME_SKIN_LOAD_FAILURE", locale_.c_str()),
            base_path_.c_str()).c_str());
      return false;
    }
    views_[view_type].reset(view.release());
    // Checks mandatory elements.
    for (size_t i = 0; i < arraysize(kMandatoryElementsInfo); ++i) {
      if (kMandatoryElementsInfo[i].view_type != view_type)
        continue;
      ggadget::BasicElement* e =
          views_[kMandatoryElementsInfo[i].view_type]->view()->GetElementByName(
          kMandatoryElementsInfo[i].name);
      if (!e || !e->IsInstanceOf(kMandatoryElementsInfo[i].class_id)) {
        views_[TOOLBAR_VIEW]->view()->Alert(ggadget::StringPrintf(
            GML_("IME_SKIN_LOAD_FAILURE", locale_.c_str()),
            base_path_.c_str()).c_str());
        return false;
      }
    }
    if (view_type == COMPOSING_VIEW) {
      // Adjust candidate list elements' orientation.
      CandidateListElement* candidates =
          owner_->GetElementByNameAndType<CandidateListElement>(
          COMPOSING_VIEW, kCandidateListElement);
      if (candidates) {
        candidates->SetOrientation(
            vertical_candidate_layout_ ?
                CandidateListElement::ORIENTATION_VERTICAL :
                CandidateListElement::ORIENTATION_HORIZONTAL);
      }
    }
    return true;
  }

  bool LoadAllViews() {
    // Virtual keyboard view is not necessary.
    LoadView(VIRTUAL_KEYBOARD_VIEW);
    LoadView(VIRTUAL_KEYBOARD_102_VIEW);
    return LoadView(TOOLBAR_VIEW) && LoadView(COMPOSING_VIEW);
  }

  // Do real initialize.
  bool Initialize() {
    // Create gadget FileManager
    ggadget::FileManagerInterface* fm = ggadget::GadgetBase::CreateFileManager(
        kImeSkinManifest, base_path_.c_str(), locale_.c_str());
    if (fm == NULL)
      return false;
    file_manager_->RegisterFileManager("", fm);

    std::string error_msg;
    // Load strings and manifest.
    if (!ggadget::GadgetBase::ReadStringsAndManifest(
        file_manager_.get(), kImeSkinManifest, kImeSkinTag,
        &strings_map_, &manifest_info_map_)) {
      error_msg = ggadget::StringPrintf(
          GML_("IME_SKIN_LOAD_FAILURE", locale_.c_str()), base_path_.c_str());
    }

    // Create a view early to allow Alert() during initialization.
    ggadget::scoped_ptr<ViewBundle> view(new ViewBundle(
        GetHost()->NewViewHost(
            owner_, ggadget::ViewHostInterface::VIEW_HOST_MAIN),
        owner_, element_factory_.get()));

    if (!error_msg.empty()) {
      view->view()->Alert(error_msg.c_str());
      return false;
    }

    std::string gadget_name = GetManifestInfo(ggadget::kManifestName);
    view->view()->SetCaption(gadget_name);

    std::string min_version = GetManifestInfo(kImeSkinManifestMinVersion);
    int compare_result = 0;
    if (!ggadget::CompareVersion(
        min_version.c_str(), kImeSkinAPIVersion, &compare_result) ||
        compare_result > 0) {
      view->view()->Alert(
          ggadget::StringPrintf(
              GML_("IME_SKIN_REQUIRE_API_VERSION", locale_.c_str()),
              min_version.c_str(),
              base_path_.c_str()).c_str());
      return false;
    }

    // For now, we only allow a skin to open web urls, no other permission is
    // allowed. So we do not need to support permissions tag in manifest file.
    // We only require NETWORK permission here, the permission should be granted
    // in the |global_permissions|.
    permissions_.SetRequired(ggadget::Permissions::NETWORK, true);

    // Load fonts.
    for (ggadget::StringMap::const_iterator i = manifest_info_map_.begin();
         i != manifest_info_map_.end(); ++i) {
      const std::string& key = i->first;
      if (ggadget::SimpleMatchXPath(key.c_str(),
                                    ggadget::kManifestInstallFontSrc)) {
        const char* font_name = i->second.c_str();
        std::string path;
        // ignore return, error not fatal
        if (ggadget::GadgetBase::ExtractFileFromFileManager(
            file_manager_.get(), font_name, &path)) {
          GetHost()->LoadFont(path.c_str());
        }
      }
    }

    // Initialize views.
    if (!LoadAllViews())
      return false;

    return true;
  }

  std::string GetManifestInfo(const char* key) {
    ggadget::StringMap::const_iterator it = manifest_info_map_.find(key);
    if (it == manifest_info_map_.end())
      return std::string();
    return it->second;
  }

  bool GetLocalImagePathByName(const char* name,
                               std::string* image_path) const {
    for (size_t i = 0; i < arraysize(kSupportedImageSuffixes); ++i) {
      std::string filename(name);
      filename.append(kSupportedImageSuffixes[i]);
      if (file_manager_->FileExists(filename.c_str(), NULL)) {
        image_path->swap(filename);
        return true;
      }
    }
    return false;
  }

  bool GetGlobalImagePathByName(const char* name,
                                std::string* image_path) const {
    // Then check global file manager.
    ggadget::FileManagerInterface* global_fm = ggadget::GetGlobalFileManager();
    if (!global_fm)
      return false;

    for (size_t i = 0; i < arraysize(kSupportedImageSuffixes); ++i) {
      std::string filename =
          GetFilePathInGlobalResources(name, kSupportedImageSuffixes[i]);
      if (global_fm->FileExists(filename.c_str(), NULL)) {
        image_path->swap(filename);
        return true;
      }
    }
    return false;
  }

  Skin* owner_;

  ggadget::StringMap manifest_info_map_;
  ggadget::StringMap strings_map_;

  std::string base_path_;
  std::string locale_;
  ggadget::Permissions permissions_;

  ggadget::scoped_ptr<ggadget::ElementFactory> element_factory_;
  ggadget::scoped_ptr<ggadget::FileManagerWrapper> file_manager_;
  ggadget::scoped_ptr<ggadget::OptionsInterface> options_;

  // 0: composing view, 1: toolbar view.
  ggadget::scoped_ptr<ViewBundle> views_[VIEW_TYPE_COUNT];

  ggadget::Signal1<void, ggadget::MenuInterface*> on_show_ime_menu_signal_;

  bool initialized_;

  bool vertical_candidate_layout_;
  bool right_to_left_layout_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Impl);
};

Skin::Skin(ggadget::HostInterface* host,
           const char* base_path,
           const char* options_name,
           const char* ui_locale,
           int instance_id,
           const ggadget::Permissions& global_permissions,
           bool vertical_candidate_layout,
           bool right_to_left_layout)
    : ggadget::GadgetBase(host, instance_id),
      impl_(new Impl(this, base_path, options_name,
                     ui_locale, global_permissions,
                     vertical_candidate_layout, right_to_left_layout)) {
  // Initialize the gadget.
  ggadget::ScopedLogContext log_context(this);
  impl_->initialized_ = impl_->Initialize();
}

Skin::~Skin() {
}

void Skin::RemoveMe(bool save_data) {
  GetHost()->RemoveGadget(this, save_data);
}

bool Skin::IsSafeToRemove() const {
  return true;
}

bool Skin::IsValid() const {
  return impl_->initialized_;
}

ggadget::FileManagerInterface* Skin::GetFileManager() const {
  return impl_->file_manager_.get();
}

ggadget::OptionsInterface* Skin::GetOptions() {
  return impl_->options_.get();
}

std::string Skin::GetManifestInfo(const char* key) const {
  return impl_->GetManifestInfo(key);
}

bool Skin::ParseLocalizedXML(const std::string& xml,
                             const char* filename,
                             ggadget::DOMDocumentInterface* xmldoc) const {
  return ggadget::GetXMLParser()->ParseContentIntoDOM(
      xml, &impl_->strings_map_, filename, NULL, NULL,
      ggadget::kEncodingFallback, xmldoc, NULL, NULL);
}

ggadget::View* Skin::GetMainView() const {
  return GetView(TOOLBAR_VIEW);
}

bool Skin::ShowMainView() {
  return ShowView(TOOLBAR_VIEW);
}

void Skin::CloseMainView() {
  CloseView(TOOLBAR_VIEW);
}

bool Skin::HasAboutDialog() const {
  // TODO(suzhe): We may support skin specific about dialog in the next version.
  return false;
}

void Skin::ShowAboutDialog() {
}

bool Skin::HasOptionsDialog() const {
  // TODO(suzhe): We may support skin specific options dialog when we support
  // dynamic skin.
  return false;
}

bool Skin::ShowOptionsDialog() {
  return false;
}

void Skin::OnAddCustomMenuItems(ggadget::MenuInterface* menu) {
  impl_->on_show_ime_menu_signal_(menu);
}

const ggadget::Permissions* Skin::GetPermissions() const {
  return &impl_->permissions_;
}

void Skin::SetVerticalCandidateLayout(bool vertical_candidate_layout) {
  if (impl_->vertical_candidate_layout_ != vertical_candidate_layout) {
    impl_->vertical_candidate_layout_ = vertical_candidate_layout;
    impl_->LoadView(COMPOSING_VIEW);
  }
}

void Skin::SetRightToLeftLayout(bool right_to_left) {
  if (impl_->right_to_left_layout_ != right_to_left) {
    impl_->right_to_left_layout_ = right_to_left;
    impl_->LoadAllViews();
  }
}

ggadget::View* Skin::GetView(Skin::ViewType view_type) const {
  Impl::ViewBundle* view = impl_->views_[view_type].get();
  return view ? view->view() : NULL;
}

bool Skin::ShowView(Skin::ViewType view_type) {
  ggadget::View* view = GetView(view_type);
  return view && view->ShowView(false, 0, NULL);
}

void Skin::CloseView(Skin::ViewType view_type) {
  ggadget::View* view = GetView(view_type);
  if (view)
    view->CloseView();
}

ggadget::View* Skin::GetComposingView() const {
  return GetView(COMPOSING_VIEW);
}

bool Skin::ShowComposingView() {
  return ShowView(COMPOSING_VIEW);
}

void Skin::CloseComposingView() {
  CloseView(COMPOSING_VIEW);
}

ggadget::View* Skin::GetVirtualKeyboardView() const {
  return GetView(VIRTUAL_KEYBOARD_VIEW);
}

ggadget::View* Skin::GetVirtualKeyboard102View() const {
  return GetView(VIRTUAL_KEYBOARD_102_VIEW);
}

void Skin::CloseAllViews() {
  for (size_t i = 0; i < arraysize(impl_->views_); ++i) {
    if (impl_->views_[i].get())
      impl_->views_[i]->view()->CloseView();
  }
}

ggadget::BasicElement* Skin::GetElementByName(ViewType view_type,
                                              const char* name) {
  ggadget::View* view = GetView(view_type);
  return view ? view->GetElementByName(name) : NULL;
}

Skin::ImageResult Skin::GetImagePathByName(const char* name,
                                           std::string* image_path) const {
  if (!name || !*name || !image_path)
    return IMAGE_NOT_FOUND;

  if (impl_->GetLocalImagePathByName(name, image_path))
    return IMAGE_FOUND_IN_SKIN;
  if (impl_->GetGlobalImagePathByName(name, image_path))
    return IMAGE_FOUND_IN_GLOBAL;
  return IMAGE_NOT_FOUND;
}

bool Skin::SetNamedButtonImagesByNames(ViewType view_type,
                                       const char* button_name,
                                       const char* image,
                                       const char* down_image,
                                       const char* over_image,
                                       const char* disabled_image) {
  ggadget::View* view = GetView(view_type);
  if (!view)
    return false;

  ggadget::BasicElement* e = view->GetElementByName(button_name);
  return e && e->IsInstanceOf(ggadget::ButtonElement::CLASS_ID) &&
      SetButtonImagesByNames(ggadget::down_cast<ggadget::ButtonElement*>(e),
                             image, down_image, over_image, disabled_image);
}

bool Skin::SetButtonImagesByNames(ggadget::ButtonElement* button,
                                  const char* image,
                                  const char* down_image,
                                  const char* over_image,
                                  const char* disabled_image) {
  ASSERT(button);
  ASSERT(button->GetView()->GetGadget() == this);

  if (!image || !*image)
    return false;

  std::string path;
  ImageResult result = GetImagePathByName(image, &path);

  // The normal image file must be specified.
  if (result == IMAGE_NOT_FOUND)
    return false;

  button->SetImage(ggadget::Variant(path));

  if (down_image && *down_image &&
      (result == IMAGE_FOUND_IN_SKIN ?
       impl_->GetLocalImagePathByName(down_image, &path) :
       impl_->GetGlobalImagePathByName(down_image, &path))) {
    button->SetDownImage(ggadget::Variant(path));
  } else {
    button->SetDownImage(ggadget::Variant());
  }

  if (over_image && *over_image &&
      (result == IMAGE_FOUND_IN_SKIN ?
       impl_->GetLocalImagePathByName(over_image, &path) :
       impl_->GetGlobalImagePathByName(over_image, &path))) {
    button->SetOverImage(ggadget::Variant(path));
  } else {
    button->SetOverImage(ggadget::Variant());
  }

  if (disabled_image && *disabled_image &&
      (result == IMAGE_FOUND_IN_SKIN ?
       impl_->GetLocalImagePathByName(disabled_image, &path) :
       impl_->GetGlobalImagePathByName(disabled_image, &path))) {
    button->SetDisabledImage(ggadget::Variant(path));
  } else {
    button->SetDisabledImage(ggadget::Variant());
  }

  return true;
}

void Skin::SetElementVisibleByName(ViewType view_type,
                                   const char* name,
                                   bool visible) {
  ggadget::BasicElement* element = GetElementByName(view_type, name);
  if (element)
    element->SetVisible(visible);
}

void Skin::SetElementEnabledByName(ViewType view_type,
                                   const char* name,
                                   bool enabled) {
  ggadget::BasicElement* element = GetElementByName(view_type, name);
  if (element)
    element->SetEnabled(enabled);
}

void Skin::SetHelpMessage(ViewType view_type, const char* message) {
  ggadget::BasicElement* label = GetElementByName(view_type, kHelpMessageLabel);
  if (!label || !label->IsInstanceOf(ggadget::LabelElement::CLASS_ID))
    return;

  if (!message || !*message)
    label->SetVisible(false);

  ggadget::TextFrame* text_frame =
      ggadget::down_cast<ggadget::LabelElement*>(label)->GetTextFrame();
  text_frame->SetText(message);
  label->SetVisible(true);
  label->ResetWidthToDefault();
  label->ResetHeightToDefault();
}

ggadget::Connection* Skin::ConnectOnShowImeMenu(
    ggadget::Slot1<void, ggadget::MenuInterface*>* handler) {
  return impl_->on_show_ime_menu_signal_.Connect(handler);
}

// static
ggadget::Connection* Skin::ConnectOnElementContextMenuEvent(
    ggadget::BasicElement* element,
    ElementContextMenuEventSlot* handler) {
  return element ? element->ConnectOnContextMenuEvent(
      new Impl::ContextMenuEventSlot(element, handler)) : NULL;
}

// static
bool Skin::GetSkinManifest(const char* base_path,
                           ggadget::StringMap* data) {
  return GetSkinManifestForLocale(base_path, NULL, data);
}

// static
bool Skin::GetSkinManifestForLocale(const char* base_path,
                                    const char* locale,
                                    ggadget::StringMap* data) {
  return GetManifestForLocale(
      kImeSkinManifest, kImeSkinTag, base_path, locale, data);
}

// static
ggadget::FileManagerInterface* Skin::GetSkinFileManagerForLocale(
    const char* base_path,
    const char* locale) {
  return CreateFileManager(kImeSkinManifest, base_path, locale);
}

// static
bool Skin::ValidateSkinPackage(const char* base_path, const char* ui_locale) {
  if (!base_path || !*base_path)
    return false;

  DummyHost host;
  ggadget::Permissions permissions;
  ggadget::scoped_ptr<Skin> skin(new Skin(
      &host, base_path, kValidationOptionsName, ui_locale, 0, permissions,
      false, false));
  ASSERT(skin.get());
  skin->GetOptions()->DeleteStorage();
  return skin->IsValid();
}

}  // namespace skin
}  // namespace ime_goopy
