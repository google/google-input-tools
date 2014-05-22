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

#include "gadget.h"
#include "contentarea_element.h"
#include "content_item.h"
#include "details_view_data.h"
#include "display_window.h"
#include "element_factory.h"
#include "extension_manager.h"
#include "file_manager_interface.h"
#include "file_manager_factory.h"
#include "file_manager_wrapper.h"
#include "localized_file_manager.h"
#include "gadget_consts.h"
#include "logger.h"
#include "main_loop_interface.h"
#include "menu_interface.h"
#include "messages.h"
#include "host_interface.h"
#include "options_interface.h"
#include "permissions.h"
#include "script_context_interface.h"
#include "script_runtime_manager.h"
#include "scriptable_array.h"
#include "scriptable_binary_data.h"
#include "scriptable_framework.h"
#include "scriptable_helper.h"
#include "scriptable_map.h"
#include "scriptable_menu.h"
#include "scriptable_options.h"
#include "scriptable_view.h"
#include "system_utils.h"
#include "view_host_interface.h"
#include "view.h"
#include "xml_dom.h"
#include "xml_http_request_interface.h"
#include "xml_parser_interface.h"
#include "xml_utils.h"

namespace ggadget {

class Gadget::Impl : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x6a3c396b3a544148, ScriptableInterface);

  /**
   * A class to bundles View, ScriptableView, ScriptContext, and
   * DetailsViewData together.
   */
  class ViewBundle {
   public:
    ViewBundle(ViewHostInterface *view_host,
               Gadget *gadget,
               ElementFactory *element_factory,
               ScriptableInterface *prototype,
               DetailsViewData *details,
               bool support_script)
      : context_(NULL),
        view_(NULL),
        scriptable_(NULL),
        details_(details) {
      if (support_script) {
        // Only xml based views have standalone script context.
        // FIXME: ScriptContext instance should be created on-demand, according
        // to the type of script files shipped in the gadget.
        // Or maybe we should add an option in gadget.gmanifest to specify what
        // ScriptRuntime implementation is required.
        // We may support multiple different script languages later.
        context_ = ScriptRuntimeManager::get()->CreateScriptContext("js");
        if (context_) {
          context_->ConnectScriptBlockedFeedback(
              NewSlot(this, &ViewBundle::OnScriptBlocked));
          ConnectContextLogListener(
              context_, NewSlot(gadget->impl_, &Impl::OnContextLog, context_));
        }
      }

      view_ = new View(view_host, gadget, element_factory, context_);
      if (details_)
        details_->Ref();
      if (context_) {
        scriptable_ = new ScriptableView(view_, prototype, context_);

        context_->RegisterClass(
            "DOMDocument", NewSlot(this, &ViewBundle::CreateDOMDocument));
        context_->RegisterClass(
            "XMLHttpRequest",
            NewSlot(view_->GetGadget(),
                    &GadgetInterface::CreateXMLHttpRequest));
        context_->RegisterClass(
            "DetailsView", NewSlot(DetailsViewData::CreateInstance));
        context_->RegisterClass(
            "ContentItem", NewSlot(ContentItem::CreateInstance, view_));

        // Execute common.js to initialize global constants and compatibility
        // adapters.
        std::string common_js_contents;
        if (GetGlobalFileManager()->ReadFile(kCommonJS, &common_js_contents)) {
          std::string path = GetGlobalFileManager()->GetFullPath(kCommonJS);
          context_->Execute(common_js_contents.c_str(), path.c_str(), 1);
        } else {
          LOG("Failed to load %s.", kCommonJS);
        }
      }
    }

    ~ViewBundle() {
      if (details_) {
        details_->Unref();
        details_ = NULL;
      }
      delete scriptable_;
      scriptable_ = NULL;
      delete view_;
      view_ = NULL;
      if (context_) {
        RemoveLogContext(context_);
        context_->Destroy();
        context_ = NULL;
      }
    }

    ScriptContextInterface *context() { return context_; }
    View *view() { return view_; }
    ScriptableView *scriptable() { return scriptable_; }
    DetailsViewData *details() { return details_; }

    bool OnScriptBlocked(const char *filename, int lineno) {
      return view_->Confirm(StringPrintf(GM_("SCRIPT_BLOCKED_MESSAGE"),
                                         filename, lineno).c_str(),
                            false) == ViewHostInterface::CONFIRM_NO;
    }

    // Create a customized DOMDocument object with optional "load()" method,
    // for microsoft compatibility.
    DOMDocumentInterface *CreateDOMDocument() {
      const Permissions *perm = view_->GetGadget()->GetPermissions();
      return ::ggadget::CreateDOMDocument(
          GetXMLParser(),
          (perm && perm->IsRequiredAndGranted(Permissions::NETWORK)),
          (perm && perm->IsRequiredAndGranted(Permissions::FILE_READ)));
    }

   private:
    ScriptContextInterface *context_;
    View *view_;
    ScriptableView *scriptable_;
    DetailsViewData *details_;
  };

  Impl(Gadget *owner,
       const char *base_path,
       const char *options_name,
       const Permissions &global_permissions,
       DebugConsoleConfig debug_console_config)
      : owner_(owner),
        element_factory_(new ElementFactory()),
        extension_manager_(ExtensionManager::CreateExtensionManager()),
        file_manager_(new FileManagerWrapper()),
        options_(CreateOptions(options_name)),
        scriptable_options_(new ScriptableOptions(options_, false)),
        main_view_(NULL),
        details_view_(NULL),
        permissions_(global_permissions),
        base_path_(base_path),
        remove_me_timer_(0),
        destroy_details_view_timer_(0),
        display_target_(TARGET_FLOATING_VIEW),
        plugin_flags_(PLUGIN_FLAG_NONE),
        debug_console_config_(debug_console_config),
        initialized_(false),
        has_options_xml_(false),
        safe_to_remove_(true) {
    // Checks if necessary objects are created successfully.
    ASSERT(element_factory_);
    ASSERT(extension_manager_);
    ASSERT(file_manager_);
    ASSERT(options_);
    ASSERT(scriptable_options_);
  }

  ~Impl() {
    if (remove_me_timer_)
      GetGlobalMainLoop()->RemoveWatch(remove_me_timer_);
    if (destroy_details_view_timer_)
      GetGlobalMainLoop()->RemoveWatch(destroy_details_view_timer_);

    delete details_view_;
    details_view_ = NULL;
    delete main_view_;
    main_view_ = NULL;
    delete scriptable_options_;
    scriptable_options_ = NULL;
    delete options_;
    options_ = NULL;
    delete file_manager_;
    file_manager_ = NULL;
    if (extension_manager_) {
      extension_manager_->Destroy();
      extension_manager_ = NULL;
    }
    delete element_factory_;
    element_factory_ = NULL;
  }

  HostInterface *GetHost() const {
    return owner_->GetHost();
  }

  // Do real initialize.
  bool Initialize() {
    if (!element_factory_ || !file_manager_ || !options_ ||
        !scriptable_options_) {
      return false;
    }

    // Create gadget FileManager
    FileManagerInterface *fm = GadgetBase::CreateFileManager(
        kGadgetGManifest, base_path_.c_str(), NULL);
    if (fm == NULL)
      return false;
    file_manager_->RegisterFileManager("", fm);

    // Create system FileManager
    fm = ::ggadget::CreateFileManager(kDirSeparatorStr);
    if (fm) file_manager_->RegisterFileManager(kDirSeparatorStr, fm);

    std::string error_msg;
    // Load strings and manifest.
    if (!GadgetBase::ReadStringsAndManifest(
        file_manager_, kGadgetGManifest, kGadgetTag,
        &strings_map_, &manifest_info_map_)) {
      error_msg = StringPrintf(GM_("GADGET_LOAD_FAILURE"), base_path_.c_str());
    }
    // Create main view early to allow Alert() during initialization.
    main_view_ = new ViewBundle(
        GetHost()->NewViewHost(owner_, ViewHostInterface::VIEW_HOST_MAIN),
        owner_, element_factory_, &global_, NULL, true);
    ASSERT(main_view_);

    if (!error_msg.empty()) {
      main_view_->view()->Alert(error_msg.c_str());
      return false;
    }

    main_view_->view()->SetCaption(GetManifestInfo(kManifestName));

    std::string min_version = GetManifestInfo(kManifestMinVersion);
    DLOG("Gadget min version: %s", min_version.c_str());
    DLOG("Gadget id: %s", GetManifestInfo(kManifestId).c_str());
    DLOG("Gadget name: %s", GetManifestInfo(kManifestName).c_str());
    DLOG("Gadget description: %s",
         GetManifestInfo(kManifestDescription).c_str());

    int compare_result = 0;
    if (!CompareVersion(min_version.c_str(), GGL_API_VERSION,
                        &compare_result) ||
        compare_result > 0) {
      main_view_->view()->Alert(
          StringPrintf(GM_("GADGET_REQUIRE_API_VERSION"),
                       min_version.c_str(), base_path_.c_str()).c_str());
      return false;
    }

    // Load permissions information at very beginning, in case following
    // initialization code requires it.
    // permissions_ contains the global permissions.
    Permissions global_permissions = permissions_;

    // Clear permissions_.
    permissions_ = Permissions();
    Variant value = options_->GetInternalValue(kPermissionsOption);
    if (value.type() == Variant::TYPE_STRING) {
      permissions_.FromString(VariantValue<const char *>()(value));
    }
    GetGadgetRequiredPermissions(&manifest_info_map_, &permissions_);

    // Denies all permissions which are denied explicitly in global
    // permissions.
    permissions_.SetGrantedByPermissions(global_permissions, false);
    DLOG("Gadget permissions: %s", permissions_.ToString().c_str());

    if (debug_console_config_ == DEBUG_CONSOLE_INITIAL)
      GetHost()->ShowGadgetDebugConsole(owner_);

    // Register strings names as global variables at first, so they have the
    // lowest priority.
    RegisterStrings(&strings_map_, &global_);
    RegisterStrings(&strings_map_, &strings_);
    // Register scriptable properties.
    RegisterProperties();

    // load fonts and objects and check platform
    for (StringMap::const_iterator i = manifest_info_map_.begin();
         i != manifest_info_map_.end(); ++i) {
      const std::string &key = i->first;
      if (SimpleMatchXPath(key.c_str(), kManifestInstallFontSrc)) {
        const char *font_name = i->second.c_str();
        std::string path;
        // ignore return, error not fatal
        if (GadgetBase::ExtractFileFromFileManager(
            file_manager_, font_name, &path)) {
          GetHost()->LoadFont(path.c_str());
        }
      } else if (SimpleMatchXPath(key.c_str(), kManifestInstallObjectSrc) &&
                 extension_manager_) {
#ifdef GGL_DISABLE_SHARED
        LOG("Loading external module is not supported by "
            "statically linked host.");
#else
        if (permissions_.IsRequired(Permissions::ALL_ACCESS) &&
            permissions_.IsGranted(Permissions::ALL_ACCESS)) {
          // Only trusted gadget can load local extensions.
          const char *module_name = i->second.c_str();
          std::string path;
          if (GadgetBase::ExtractFileFromFileManager(
              file_manager_, module_name, &path)) {
            extension_manager_->LoadExtension(path.c_str(), false);
          }
        } else {
          LOG("Local extension module is forbidden for untrusted gadgets.");
        }
#endif
      } else if (SimpleMatchXPath(key.c_str(), kManifestPlatformSupported)) {
        if (i->second == "no") {
          main_view_->view()->Alert(
              StringPrintf(GM_("GADGET_PLATFORM_NOT_SUPPORTED"),
                           base_path_.c_str()).c_str());
          return false;
        }
      } else if (SimpleMatchXPath(key.c_str(), kManifestPlatformMinVersion)) {
        if (!CompareVersion(i->second.c_str(), GGL_VERSION, &compare_result) ||
            compare_result > 0) {
          main_view_->view()->Alert(
              StringPrintf(GM_("GADGET_REQUIRE_HOST_VERSION"),
                           i->second.c_str(), base_path_.c_str()).c_str());
          return false;
        }
      }
    }

    // Register extensions
    const ExtensionManager *global_manager =
        ExtensionManager::GetGlobalExtensionManager();
    MultipleExtensionRegisterWrapper register_wrapper;
    ElementExtensionRegister element_register(element_factory_);
    FrameworkExtensionRegister framework_register(&framework_, owner_);
    FileManagerExtensionRegister fm_register(file_manager_);

    register_wrapper.AddExtensionRegister(&element_register);
    register_wrapper.AddExtensionRegister(&framework_register);
    register_wrapper.AddExtensionRegister(&fm_register);

    if (global_manager)
      global_manager->RegisterLoadedExtensions(&register_wrapper);
    if (extension_manager_)
      extension_manager_->RegisterLoadedExtensions(&register_wrapper);

    // Initialize main view.
    std::string main_xml;
    if (!file_manager_->ReadFile(kMainXML, &main_xml)) {
      LOG("Failed to load main.xml.");
      main_view_->view()->Alert(StringPrintf(GM_("GADGET_LOAD_FAILURE"),
                                             base_path_.c_str()).c_str());
      return false;
    }

    RegisterScriptExtensions(main_view_->context());

    if (!main_view_->scriptable()->InitFromXML(main_xml, kMainXML)) {
      LOG("Failed to setup the main view");
      main_view_->view()->Alert(StringPrintf(GM_("GADGET_LOAD_FAILURE"),
                                             base_path_.c_str()).c_str());
      return false;
    }

    has_options_xml_ = file_manager_->FileExists(kOptionsXML, NULL);
    DLOG("Initialized View(%p) size: %f x %f", main_view_->view(),
         main_view_->view()->GetWidth(), main_view_->view()->GetHeight());

    // Connect signals to monitor display state change.
    main_view_->view()->ConnectOnMinimizeEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_MINIMIZED)));
    main_view_->view()->ConnectOnRestoreEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_RESTORED)));
    main_view_->view()->ConnectOnPopOutEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_POPPED_OUT)));
    // FIXME: Is it correct to send RESTORED when popped in?
    main_view_->view()->ConnectOnPopInEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_RESTORED)));
    main_view_->view()->ConnectOnSizeEvent(
        NewSlot(this, &Impl::OnDisplayStateChanged,
                static_cast<int>(TILE_DISPLAY_STATE_RESIZED)));

    // Let gadget know the initial display target.
    ondisplaytargetchange_signal_(display_target_);
    return true;
  }

  void OnDisplayStateChanged(int state) {
    ondisplaystatechange_signal_(state);
  }

  // Register script extensions for a specified script context.
  // This method shall be called for all views' script contexts.
  void RegisterScriptExtensions(ScriptContextInterface *context) {
    ASSERT(context);
    const ExtensionManager *global_manager =
        ExtensionManager::GetGlobalExtensionManager();
    ScriptExtensionRegister script_register(context, owner_);

    if (global_manager)
      global_manager->RegisterLoadedExtensions(&script_register);
    if (extension_manager_)
      extension_manager_->RegisterLoadedExtensions(&script_register);
  }

  // Register all scriptable properties.
  void RegisterProperties() {
    RegisterConstant("debug", &debug_);
    RegisterConstant("storage", &storage_);

    // Register properties of gadget.debug.
    debug_.RegisterMethod("trace", NewSlot(this, &Impl::ScriptLog, LOG_TRACE));
    debug_.RegisterMethod("info", NewSlot(this, &Impl::ScriptLog, LOG_INFO));
    debug_.RegisterMethod("warning", NewSlot(this, &Impl::ScriptLog,
                                             LOG_WARNING));
    debug_.RegisterMethod("error", NewSlot(this, &Impl::ScriptLog, LOG_ERROR));

    // Register properties of gadget.storage.
    storage_.RegisterMethod("extract", NewSlot(this, &Impl::ExtractFile));
    storage_.RegisterMethod("openText", NewSlot(this, &Impl::OpenTextFile));

    // Register properties of plugin.
    plugin_.RegisterProperty("plugin_flags", NULL, // No getter.
                             NewSlot(this, &Impl::SetPluginFlags));
    plugin_.RegisterProperty("title", NULL, // No getter.
                             NewSlot(main_view_->view(), &View::SetCaption));
    plugin_.RegisterProperty("window_width",
                             NewSlot(main_view_->view(), &View::GetWidth),
                             NULL);
    plugin_.RegisterProperty("window_height",
                             NewSlot(main_view_->view(), &View::GetHeight),
                             NULL);

    plugin_.RegisterMethod("RemoveMe",
                           NewSlot(this, &Impl::RemoveMe));
    plugin_.RegisterMethod("ShowDetailsView",
                           NewSlot(this, &Impl::ShowDetailsViewProxy));
    plugin_.RegisterMethod("CloseDetailsView",
                           NewSlot(this, &Impl::CloseDetailsView));
    plugin_.RegisterMethod("ShowOptionsDialog",
                           NewSlot(this, &Impl::ShowOptionsDialog));

    plugin_.RegisterSignal("onShowOptionsDlg",
                           &onshowoptionsdlg_signal_);
    plugin_.RegisterSignal("onAddCustomMenuItems",
                           &onaddcustommenuitems_signal_);
    plugin_.RegisterSignal("onCommand",
                           &oncommand_signal_);
    plugin_.RegisterSignal("onDisplayStateChange",
                           &ondisplaystatechange_signal_);
    plugin_.RegisterSignal("onDisplayTargetChange",
                           &ondisplaytargetchange_signal_);

    // Deprecated or unofficial properties and methods.
    plugin_.RegisterProperty("about_text", NULL, // No getter.
                             NewSlot(this, &Impl::SetAboutText));
    plugin_.RegisterMethod("SetFlags", NewSlot(this, &Impl::SetFlags));
    plugin_.RegisterMethod("SetIcons", NewSlot(this, &Impl::SetIcons));

    // Register properties and methods for content area.
    plugin_.RegisterProperty("contant_flags", NULL, // Write only.
                             NewSlot(this, &Impl::SetContentFlags));
    plugin_.RegisterProperty("max_content_items",
                             NewSlot(this, &Impl::GetMaxContentItems),
                             NewSlot(this, &Impl::SetMaxContentItems));
    plugin_.RegisterProperty("content_items",
                             NewSlot(this, &Impl::GetContentItems),
                             NewSlot(this, &Impl::SetContentItems));
    plugin_.RegisterProperty("pin_images",
                             NewSlot(this, &Impl::GetPinImages),
                             NewSlot(this, &Impl::SetPinImages));
    plugin_.RegisterMethod("AddContentItem",
                           NewSlot(this, &Impl::AddContentItem));
    plugin_.RegisterMethod("RemoveContentItem",
                           NewSlot(this, &Impl::RemoveContentItem));
    plugin_.RegisterMethod("RemoveAllContentItems",
                           NewSlot(this, &Impl::RemoveAllContentItems));

    // Register global properties.
    global_.RegisterConstant("gadget", this);
    global_.RegisterConstant("options", scriptable_options_);
    global_.RegisterConstant("strings", &strings_);
    global_.RegisterConstant("plugin", &plugin_);
    global_.RegisterConstant("pluginHelper", &plugin_);

    // As an unofficial feature, "gadget.debug" and "gadget.storage" can also
    // be accessed as "debug" and "storage" global objects.
    global_.RegisterConstant("debug", &debug_);
    global_.RegisterConstant("storage", &storage_);

    // Properties and methods of framework can also be accessed directly as
    // globals.
    global_.RegisterConstant("framework", &framework_);
    global_.SetInheritsFrom(&framework_);

    // OpenURL will check permissions by itself.
    framework_.RegisterMethod("openUrl",
                              NewSlot(static_cast<GadgetInterface*>(owner_),
                                      &GadgetInterface::OpenURL));
  }

  class RemoveMeWatchCallback : public WatchCallbackInterface {
   public:
    RemoveMeWatchCallback(Gadget *owner, bool save_data)
      : owner_(owner), save_data_(save_data) {
    }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      owner_->impl_->remove_me_timer_ = 0;
      if (owner_->impl_->IsSafeToRemove())
        owner_->GetHost()->RemoveGadget(owner_, save_data_);
      return false;
    }
    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      delete this;
    }
   private:
    Gadget *owner_;
    bool save_data_;
  };

  void RemoveMe(bool save_data) {
    if (!remove_me_timer_) {
      if (!save_data) {
        options_->DeleteStorage();
      }
      remove_me_timer_ = GetGlobalMainLoop()->AddTimeoutWatch(
          0, new RemoveMeWatchCallback(owner_, save_data));
    }
  }

  bool IsSafeToRemove() {
    return safe_to_remove_ &&
        (!main_view_ || main_view_->view()->IsSafeToDestroy()) &&
        (!details_view_ || details_view_->view()->IsSafeToDestroy());
  }

  void DebugConsoleMenuCallback(const char *) {
    GetHost()->ShowGadgetDebugConsole(owner_);
  }

  void FeedbackMenuCallback(const char *, const std::string &url) {
    GetHost()->OpenURL(NULL, url.c_str());
  }

  void OnAddCustomMenuItems(MenuInterface *menu) {
    ScriptableMenu *smenu = new ScriptableMenu(owner_, menu);
    smenu->Ref();
    onaddcustommenuitems_signal_(smenu);
    // Some of the menu handler slots may still hold the reference.
    smenu->Unref();
    if (debug_console_config_ != DEBUG_CONSOLE_DISABLED) {
      menu->AddItem(GM_("MENU_ITEM_DEBUG_CONSOLE"), 0, 0,
                    NewSlot(this, &Impl::DebugConsoleMenuCallback),
                    MenuInterface::MENU_ITEM_PRI_GADGET);
    }

    if (ongetfeedbackurl_signal_.HasActiveConnections()) {
      std::string url = ongetfeedbackurl_signal_();
      if (url.length()) {
        menu->AddItem(GM_("MENU_ITEM_FEEDBACK"), 0, 0,
                      NewSlot(this, &Impl::FeedbackMenuCallback, url),
                      MenuInterface::MENU_ITEM_PRI_GADGET);
      }
    }
    // Remove item is added in view decorator.
  }

  void SetDisplayTarget(DisplayTarget target) {
    // Fire the signal no matter whether the target is changed or not.
    // gtkmoz browser element relies on this behaviour.
    display_target_ = target;
    ondisplaytargetchange_signal_(target);
  }

  void SetPluginFlags(int flags) {
    bool changed = (flags != plugin_flags_);
    // Casting to PluginFlags to avoid conversion warning when
    // compiling by the latest gcc.
    plugin_flags_ = static_cast<PluginFlags>(flags);
    if (changed)
      onpluginflagschanged_signal_(flags);
  }

  void SetFlags(int plugin_flags, int content_flags) {
    SetPluginFlags(plugin_flags);
    SetContentFlags(content_flags);
  }

  void SetIcons(const Variant &param1, const Variant &param2) {
    GGL_UNUSED(param1);
    GGL_UNUSED(param2);
    LOG("pluginHelper.SetIcons is no longer supported. "
        "Please specify icons in the manifest file.");
  }

  void SetContentFlags(int flags) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->SetContentFlags(flags);
  }

  size_t GetMaxContentItems() {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    return content_area ? content_area->GetMaxContentItems() : 0;
  }

  void SetMaxContentItems(size_t max_content_items) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->SetMaxContentItems(max_content_items);
  }

  ScriptableArray *GetContentItems() {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    return content_area ? content_area->ScriptGetContentItems() : NULL;
  }

  void SetContentItems(ScriptableInterface *array) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->ScriptSetContentItems(array);
  }

  ScriptableArray *GetPinImages() {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    return content_area ? content_area->ScriptGetPinImages() : NULL;
  }

  void SetPinImages(ScriptableInterface *array) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->ScriptSetPinImages(array);
  }

  void AddContentItem(ContentItem *item,
                      ContentAreaElement::DisplayOptions options) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->AddContentItem(item, options);
  }

  void RemoveContentItem(ContentItem *item) {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->RemoveContentItem(item);
  }

  void RemoveAllContentItems() {
    ContentAreaElement *content_area =
        main_view_->view()->GetContentAreaElement();
    if (content_area) content_area->RemoveAllContentItems();
  }

  void SetAboutText(const char *about_text) {
    manifest_info_map_[kManifestAboutText] = about_text;
  }

  std::string OnContextLog(LogLevel level,
                           const char *filename,
                           int line,
                           const std::string &message,
                           ScriptContextInterface *context) {
    return owner_->OnContextLog(level, filename, line, message, context);
  }

  void ScriptLog(const char *message, LogLevel level) {
    LogHelper(level, NULL, 0)("%s", message);
  }

  // ExtractFile and OpenTextFile only allow accessing gadget local files.
  static bool FileNameIsLocal(const char *filename) {
    return filename && filename[0] != '/' && filename[0] != '\\' &&
           strchr(filename, ':') == NULL;
  }

  std::string ExtractFile(const char *filename) {
    std::string extracted_file;
    return FileNameIsLocal(filename) &&
           file_manager_->ExtractFile(filename, &extracted_file) ?
        extracted_file : "";
  }

  std::string OpenTextFile(const char *filename) {
    std::string data;
    std::string result;
    if (FileNameIsLocal(filename) &&
        file_manager_->ReadFile(filename, &data) &&
        !DetectAndConvertStreamToUTF8(data, &result, NULL)) {
      LOG("gadget.storage.openText() failed to read text file: %s", filename);
    }
    return result;
  }

  std::string GetManifestInfo(const char *key) {
    StringMap::const_iterator it = manifest_info_map_.find(key);
    if (it == manifest_info_map_.end())
      return std::string();
    return it->second;
  }

  bool HasOptionsDialog() {
    return has_options_xml_ || onshowoptionsdlg_signal_.HasActiveConnections();
  }

  static bool OptionsDialogCallback(int flag, ViewBundle *options_view) {
    if (options_view) {
      SimpleEvent event((flag == ViewInterface::OPTIONS_VIEW_FLAG_OK) ?
                        Event::EVENT_OK : Event::EVENT_CANCEL);
      return options_view->view()->OnOtherEvent(event) != EVENT_RESULT_CANCELED;
    }
    return true;
  }

  bool ShowOptionsDialog() {
    bool ret = false;
    int flags = ViewInterface::OPTIONS_VIEW_FLAG_OK |
                ViewInterface::OPTIONS_VIEW_FLAG_CANCEL;

    safe_to_remove_ = false;
    if (onshowoptionsdlg_signal_.HasActiveConnections()) {
      ViewBundle options_view(
          GetHost()->NewViewHost(owner_, ViewHostInterface::VIEW_HOST_OPTIONS),
          owner_, element_factory_, NULL, NULL, false);
      View *view = options_view.view();
      DisplayWindow *window = new DisplayWindow(view);
      Variant result = onshowoptionsdlg_signal_(window);
      if ((result.type() != Variant::TYPE_BOOL ||
           VariantValue<bool>()(result)) && window->AdjustSize()) {
        view->SetResizable(ViewInterface::RESIZABLE_FALSE);
        if (view->GetCaption().empty())
          view->SetCaption(main_view_->view()->GetCaption());
        ret = view->ShowView(true, flags,
                             NewSlot(OptionsDialogCallback, &options_view));
      } else {
        DLOG("gadget cancelled the options dialog.");
      }
      delete window;
    } else if (has_options_xml_) {
      ret = ShowXMLOptionsDialog(flags, kOptionsXML, NULL);
    } else {
      LOG("Failed to show options dialog because there is neither options.xml"
          "nor OnShowOptionsDlg handler");
    }
    safe_to_remove_ = true;
    return ret;
  }

  bool ShowXMLOptionsDialog(int flags, const char *xml_file,
                            ScriptableInterface *param) {
    bool ret = false;
    std::string xml;
    safe_to_remove_ = false;
    if (file_manager_->ReadFile(xml_file, &xml) ||
        GetGlobalFileManager()->ReadFile(xml_file, &xml)) {
      ViewBundle options_view(
          GetHost()->NewViewHost(owner_, ViewHostInterface::VIEW_HOST_OPTIONS),
          owner_, element_factory_, &global_, NULL, true);
      View *view = options_view.view();
      RegisterScriptExtensions(options_view.context());
      std::string full_path = file_manager_->GetFullPath(xml_file);
      if (param) {
        // Set up the param variable in the opened options view.
        options_view.context()->AssignFromNative(NULL, "", "optionsViewData",
                                                 Variant(param));
      }
      if (options_view.scriptable()->InitFromXML(xml, full_path.c_str())) {
        // Allow XML options dialog to resize, but not zoom.
        if (view->GetResizable() == ViewInterface::RESIZABLE_ZOOM)
          view->SetResizable(ViewInterface::RESIZABLE_FALSE);
        if (view->GetCaption().empty())
          view->SetCaption(main_view_->view()->GetCaption());

        ret = view->ShowView(true, flags,
                             NewSlot(OptionsDialogCallback, &options_view));
      } else {
        LOG("Failed to setup the XML view: %s", xml_file);
      }
    } else {
      LOG("Failed to load %s file from gadget package.", xml_file);
    }
    safe_to_remove_ = true;
    return ret;
  }

  class DetailsViewCallbackProxy : public Slot1<bool, int> {
   public:
    explicit DetailsViewCallbackProxy(Slot *callback) : callback_(callback) {}
    ~DetailsViewCallbackProxy() { delete callback_; }
    virtual ResultVariant Call(ScriptableInterface *object,
                               int argc, const Variant argv[]) const {
      ASSERT(argc == 1);
      bool result = true;
      callback_->Call(object, argc, argv).v().ConvertToBool(&result);
      return ResultVariant(Variant(result));
    }
    virtual bool operator==(const Slot &another) const {
      GGL_UNUSED(another);
      return false;
    }
   private:
    Slot *callback_;
  };

  bool ShowDetailsViewProxy(DetailsViewData *details_view_data,
                            const char *title, int flags,
                            Slot *callback) {
    // Can't use SlotProxy<bool, int> here, because it can't handle return
    // value type conversion.
    Slot1<bool, int> *feedback_handler =
        callback ? new DetailsViewCallbackProxy(callback) : NULL;
    return ShowDetailsView(details_view_data, title, flags, feedback_handler);
  }

  bool ShowDetailsView(DetailsViewData *details_view_data,
                       const char *title, int flags,
                       Slot1<bool, int> *feedback_handler) {
    // Reference details_view_data to prevent it from being destroyed by
    // JavaScript GC.
    if (details_view_data)
      details_view_data->Ref();

    CloseDetailsView();
    details_view_ = new ViewBundle(
        GetHost()->NewViewHost(owner_, ViewHostInterface::VIEW_HOST_DETAILS),
        owner_, element_factory_, &global_, details_view_data, true);

    // details_view_data is now referenced by details_view_, so it's safe to
    // remove the reference.
    if (details_view_data)
      details_view_data->Unref();

    ScriptContextInterface *context = details_view_->context();
    ScriptableOptions *scriptable_data = details_view_->details()->GetData();
    OptionsInterface *data = scriptable_data->GetOptions();

    // Register script extensions.
    RegisterScriptExtensions(context);

    // Set up the detailsViewData variable in the opened details view.
    context->AssignFromNative(NULL, "", "detailsViewData",
                              Variant(scriptable_data));

    std::string xml;
    std::string xml_file;
    if (details_view_data->GetContentIsHTML() ||
        !details_view_data->GetContentIsView()) {
      if (details_view_data->GetContentIsHTML()) {
        xml_file = kHTMLDetailsView;
        ScriptableInterface *ext_obj = details_view_data->GetExternalObject();
        context->AssignFromNative(NULL, "", "external", Variant(ext_obj));
        data->PutValue("contentType", Variant("text/html"));
      } else {
        xml_file = kTextDetailsView;
        data->PutValue("contentType", Variant("text/plain"));
      }
      data->PutValue("content", Variant(details_view_data->GetText()));
      GetGlobalFileManager()->ReadFile(xml_file.c_str(), &xml);
    } else {
      xml_file = details_view_data->GetText();
      file_manager_->ReadFile(xml_file.c_str(), &xml);
    }

    if (xml.empty() ||
        !details_view_->scriptable()->InitFromXML(xml, xml_file.c_str())) {
      LOG("Failed to load details view from %s", xml_file.c_str());
      delete details_view_;
      details_view_ = NULL;
      return false;
    }

    // For details view, the caption set in xml file will be discarded.
    if (title && *title) {
      details_view_->view()->SetCaption(title);
    } else if (details_view_->view()->GetCaption().empty()) {
      details_view_->view()->SetCaption(main_view_->view()->GetCaption());
    }

    details_view_->view()->ShowView(false, flags, feedback_handler);
    return true;
  }

  class DestroyDetailsViewWatchCallback : public WatchCallbackInterface {
   public:
    DestroyDetailsViewWatchCallback(Impl *impl, ViewBundle *details_view)
        : impl_(impl), details_view_(details_view) { }
    virtual ~DestroyDetailsViewWatchCallback() {
      delete details_view_;
    }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      impl_->destroy_details_view_timer_ = 0;
      // Let the destructor do the actual thing, because this callback may
      // be removed before it is fired if it is scheduled just before the
      // gadget is to be destroyed.
      return false;
    }
    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      delete this;
    }
   private:
    Impl *impl_;
    ViewBundle *details_view_;
  };

  void CloseDetailsView() {
    if (details_view_) {
      details_view_->view()->CloseView();

      // The details view can't be destroyed now, because this function may be
      // called from the view's script and must return to it.
      if (destroy_details_view_timer_)
        GetGlobalMainLoop()->RemoveWatch(destroy_details_view_timer_);
      destroy_details_view_timer_ = GetGlobalMainLoop()->AddTimeoutWatch(
          0, new DestroyDetailsViewWatchCallback(this, details_view_));
      details_view_ = NULL;
    }
  }

  static void RegisterStrings(const StringMap *strings,
                              ScriptableHelperNativeOwnedDefault *scriptable) {
    for (StringMap::const_iterator it = strings->begin();
         it != strings->end(); ++it) {
      scriptable->RegisterConstant(it->first.c_str(), it->second);
    }
  }

  NativeOwnedScriptable<UINT64_C(0x4edfd94b70f04da6)> global_;
  NativeOwnedScriptable<UINT64_C(0xb13b9595da304041)> debug_;
  NativeOwnedScriptable<UINT64_C(0xaf77f40a271f41d4)> storage_;
  NativeOwnedScriptable<UINT64_C(0x3f7cd747988f4ad0)> plugin_;
  NativeOwnedScriptable<UINT64_C(0x50bbf15b460e48c5)> framework_;
  NativeOwnedScriptable<UINT64_C(0x8ef87d420c384a46)> strings_;

  Signal1<Variant, DisplayWindow *> onshowoptionsdlg_signal_;
  Signal1<void, ScriptableMenu *> onaddcustommenuitems_signal_;
  Signal1<void, int> oncommand_signal_;
  Signal1<void, int> ondisplaystatechange_signal_;
  Signal1<void, int> ondisplaytargetchange_signal_;
  Signal1<void, int> onpluginflagschanged_signal_;

  Signal0<std::string> ongetfeedbackurl_signal_;

  StringMap manifest_info_map_;
  StringMap strings_map_;

  Gadget *owner_;
  ElementFactory *element_factory_;
  ExtensionManager *extension_manager_;
  FileManagerWrapper *file_manager_;
  OptionsInterface *options_;
  ScriptableOptions *scriptable_options_;

  ViewBundle *main_view_;
  ViewBundle *details_view_;

  Permissions permissions_;

  std::string base_path_;

  int remove_me_timer_;
  int destroy_details_view_timer_;

  DisplayTarget display_target_            : 2;
  PluginFlags plugin_flags_                : 2;
  DebugConsoleConfig debug_console_config_ : 2;
  bool initialized_                        : 1;
  bool has_options_xml_                    : 1;
  bool safe_to_remove_                     : 1;
};

Gadget::Gadget(HostInterface *host,
               const char *base_path,
               const char *options_name,
               int instance_id,
               const Permissions &initial_permissions,
               DebugConsoleConfig debug_console_config)
    : GadgetBase(host, instance_id),
      impl_(new Impl(this, base_path, options_name,
                     initial_permissions, debug_console_config)) {
  // Initialize the gadget.
  ScopedLogContext log_context(this);
  impl_->initialized_ = impl_->Initialize();
}

Gadget::~Gadget() {
  delete impl_;
  impl_ = NULL;
}

void Gadget::RemoveMe(bool save_data) {
  impl_->RemoveMe(save_data);
}

bool Gadget::IsSafeToRemove() const {
  return impl_->IsSafeToRemove();
}

bool Gadget::IsValid() const {
  return impl_->initialized_;
}

int Gadget::GetPluginFlags() const {
  return impl_->plugin_flags_;
}

Gadget::DisplayTarget Gadget::GetDisplayTarget() const {
  return impl_->display_target_;
}

void Gadget::SetDisplayTarget(DisplayTarget target) {
  impl_->SetDisplayTarget(target);
}

FileManagerInterface *Gadget::GetFileManager() const {
  return impl_->file_manager_;
}

OptionsInterface *Gadget::GetOptions() {
  return impl_->options_;
}

View *Gadget::GetMainView() const {
  return impl_->main_view_ ? impl_->main_view_->view() : NULL;
}

std::string Gadget::GetManifestInfo(const char *key) const {
  return impl_->GetManifestInfo(key);
}

bool Gadget::ParseLocalizedXML(const std::string &xml,
                               const char *filename,
                               DOMDocumentInterface *xmldoc) const {
  return GetXMLParser()->ParseContentIntoDOM(xml, &impl_->strings_map_,
                                             filename, NULL,
                                             NULL, kEncodingFallback,
                                             xmldoc, NULL, NULL);
}

bool Gadget::ShowMainView() {
  ASSERT(IsValid());
  return impl_->main_view_->view()->ShowView(false, 0, NULL);
}

void Gadget::CloseMainView() {
  impl_->main_view_->view()->CloseView();
}

bool Gadget::HasOptionsDialog() const {
  return impl_->HasOptionsDialog();
}

bool Gadget::ShowOptionsDialog() {
  return impl_->ShowOptionsDialog();
}

bool Gadget::ShowXMLOptionsDialog(int flags, const char *xml_file,
                                  ScriptableInterface *param) {
  return impl_->ShowXMLOptionsDialog(flags, xml_file, param);
}

bool Gadget::ShowDetailsView(DetailsViewData *details_view_data,
                             const char *title, int flags,
                             Slot1<bool, int> *feedback_handler) {
  return impl_->ShowDetailsView(details_view_data, title, flags,
                                feedback_handler);
}

void Gadget::CloseDetailsView() {
  return impl_->CloseDetailsView();
}

void Gadget::OnAddCustomMenuItems(MenuInterface *menu) {
  impl_->OnAddCustomMenuItems(menu);
}

void Gadget::OnCommand(Command command) {
  impl_->oncommand_signal_(command);
}

Connection *Gadget::ConnectOnDisplayStateChanged(Slot1<void, int> *handler) {
  return impl_->ondisplaystatechange_signal_.Connect(handler);
}

Connection *Gadget::ConnectOnDisplayTargetChanged(Slot1<void, int> *handler) {
  return impl_->ondisplaytargetchange_signal_.Connect(handler);
}

Connection *Gadget::ConnectOnPluginFlagsChanged(Slot1<void, int> *handler) {
  return impl_->onpluginflagschanged_signal_.Connect(handler);
}

Connection *Gadget::ConnectOnGetFeedbackURL(Slot0<std::string> *handler) {
  return impl_->ongetfeedbackurl_signal_.Connect(handler);
}

const Permissions* Gadget::GetPermissions() const {
  return &impl_->permissions_;
}

bool Gadget::HasAboutDialog() const {
  return !GetManifestInfo(kManifestAboutText).empty() ||
      impl_->oncommand_signal_.HasActiveConnections();
}

void Gadget::ShowAboutDialog() {
  std::string about = TrimString(GetManifestInfo(kManifestAboutText));
  if (about.empty()) {
    OnCommand(Gadget::CMD_ABOUT_DIALOG);
    return;
  }

  std::string title;
  std::string copyright;
  if (!SplitString(about, "\n", &title, &about)) {
    about = title;
    title = GetManifestInfo(kManifestName);
  }
  title = TrimString(title);
  about = TrimString(about);

  if (!SplitString(about, "\n", &copyright, &about)) {
    about = copyright;
    copyright = GetManifestInfo(kManifestCopyright);
  }
  copyright = TrimString(copyright);
  about = TrimString(about);

  // Remove HTML tags from the text because this dialog can't render them.
  if (ContainsHTML(title.c_str()))
    title = ExtractTextFromHTML(title.c_str());
  if (ContainsHTML(copyright.c_str()))
    copyright = ExtractTextFromHTML(copyright.c_str());
  if (ContainsHTML(about.c_str()))
    about = ExtractTextFromHTML(about.c_str());

  LightMap<std::string, Variant> params;
  params["title"] = Variant(title);
  params["copyright"] = Variant(copyright);
  params["about"] = Variant(about);

  std::string icon_name = GetManifestInfo(kManifestIcon);
  std::string icon_data;
  GetFileManager()->ReadFile(icon_name.c_str(), &icon_data);
  ScriptableHolder<ScriptableBinaryData> icon_data_holder;
  if (!icon_data.empty()) {
    icon_data_holder.Reset(new ScriptableBinaryData(icon_data));
    params["icon"] = Variant(icon_data_holder.Get());
  }

  ShowXMLOptionsDialog(ViewInterface::OPTIONS_VIEW_FLAG_OK, kGadgetAboutView,
                       NewScriptableMap(params));
}

// static methods
bool Gadget::GetGadgetManifest(const char *base_path, StringMap *data) {
  return GetGadgetManifestForLocale(base_path, NULL, data);
}

bool Gadget::GetGadgetManifestForLocale(const char *base_path,
                                        const char *locale,
                                        StringMap *data) {
  return GetManifestForLocale(
      kGadgetGManifest, kGadgetTag, base_path, locale, data);
}

FileManagerInterface *Gadget::GetGadgetFileManagerForLocale(
    const char *base_path,
    const char *locale) {
  return CreateFileManager(kGadgetGManifest, base_path, locale);
}

bool Gadget::GetGadgetRequiredPermissions(const StringMap *manifest,
                                          Permissions *required) {
  ASSERT(manifest);
  ASSERT(required);
  StringMap::const_iterator it = manifest->begin();
  bool has_permissions = false;
  size_t prefix_length = arraysize(kManifestPermissions) - 1;

  required->RemoveAllRequired();
  for (; it != manifest->end(); ++it) {
    if (GadgetStrNCmp(it->first.c_str(), kManifestPermissions,
                      prefix_length) == 0) {
      if (it->first[prefix_length] == 0) {
        has_permissions = true;
      } else if (has_permissions && it->first[prefix_length] == '/') {
        int permission =
            Permissions::GetByName(it->first.c_str() + prefix_length + 1);
        if (permission >= 0) {
          required->SetRequired(permission, true);
        } else {
          DLOG("Invalid permission node: %s", it->first.c_str());
        }
      } else {
        DLOG("Invalid permission node: %s", it->first.c_str());
      }
    }
  }

  // If there is no permissions node in manifest, then requires <allaccess/>
  // explicitly.
  if (!has_permissions)
    required->SetRequired(Permissions::ALL_ACCESS, true);

  return has_permissions;
}

bool Gadget::SaveGadgetInitialPermissions(const char *options_path,
                                          const Permissions &permissions) {
  ASSERT(options_path && *options_path);
  bool result = false;
  OptionsInterface *options = CreateOptions(options_path);
  ASSERT(options);
  if (options) {
    Permissions tmp = permissions;
    tmp.RemoveAllRequired();
    options->PutInternalValue(kPermissionsOption, Variant(tmp.ToString()));
    options->Flush();
    result = true;
    delete options;
  }
  return result;
}

bool Gadget::LoadGadgetInitialPermissions(const char *options_path,
                                          Permissions *permissions) {
  ASSERT(options_path && *options_path);
  ASSERT(permissions);
  bool result = false;
  OptionsInterface *options = CreateOptions(options_path);
  ASSERT(options);
  if (options) {
    Variant value = options->GetInternalValue(kPermissionsOption);
    if (value.type() == Variant::TYPE_STRING) {
      Permissions granted_permissions;
      granted_permissions.FromString(VariantValue<const char*>()(value));
      permissions->SetGrantedByPermissions(granted_permissions, true);
      permissions->SetGrantedByPermissions(granted_permissions, false);
      result = true;
    }
    delete options;
  }
  return result;
}

} // namespace ggadget
