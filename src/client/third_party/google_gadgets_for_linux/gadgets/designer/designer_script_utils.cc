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

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <ggadget/basic_element.h>
#include <ggadget/color.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/host_interface.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/options_interface.h>
#include <ggadget/permissions.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_menu.h>
#include <ggadget/scriptable_view.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/uuid.h>
#include <ggadget/view_interface.h>

namespace ggadget {
namespace designer {

static const char kGadgetFileManagerPrefix[] = "gadget://";
static FileManagerInterface *g_gadget_file_manager = NULL;
static FileManagerWrapper *g_designer_file_manager = NULL;
static Gadget *g_designer_gadget = NULL;
static GadgetInterface *g_designee_gadget = NULL;
static Connection *g_designee_close_connection = NULL;

class ScriptableFileManager : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x5a03aafca3094f1c, ScriptableInterface);

  ScriptableFileManager(FileManagerInterface *fm)
      : fm_(fm) {
  }

  virtual void DoRegister() {
    RegisterMethod("read",
                   NewSlot(this, &ScriptableFileManager::ReadFile));
    RegisterMethod("readBinary",
                   NewSlot(this, &ScriptableFileManager::ReadBinaryFile));
    RegisterMethod("write",
                   NewSlot(fm_, &FileManagerInterface::WriteFile));
    RegisterMethod("writeBinary",
                   NewSlot(this, &ScriptableFileManager::WriteBinaryFile));
    RegisterMethod("remove",
                   NewSlot(fm_, &FileManagerInterface::RemoveFile));
    RegisterMethod("extract",
                   NewSlot(this, &ScriptableFileManager::ExtractFile));
    RegisterMethod("exists",
                   NewSlot(this, &ScriptableFileManager::FileExists));
    RegisterMethod("isDirectlyAccessible",
                   NewSlot(this, &ScriptableFileManager::IsDirectlyAccessible));
    RegisterMethod("getFullPath",
                   NewSlot(fm_, &FileManagerInterface::GetFullPath));
    RegisterMethod("getLastModifiedTime",
                   NewSlot(this, &ScriptableFileManager::GetLastModifiedTime));
    RegisterMethod("getAllFiles",
                   NewSlot(this, &ScriptableFileManager::GetAllFiles));
    RegisterMethod("copy", NewSlot(this, &ScriptableFileManager::CopyFile));
  }

  std::string ReadFile(const char *file) {
    std::string result;
    fm_->ReadFile(file, &result);
    return result;
  }

  ScriptableBinaryData *ReadBinaryFile(const char *file) {
    std::string result;
    return fm_->ReadFile(file, &result) ?
           new ScriptableBinaryData(result) : NULL;
  }

  bool WriteBinaryFile(const char *file, ScriptableBinaryData *data,
                       bool overwrite) {
    if (!data)
      return false;
    return fm_->WriteFile(file, data->data(), overwrite);
  }

  std::string ExtractFile(const char *file) {
    std::string into_file;
    fm_->ExtractFile(file, &into_file);
    return into_file;
  }

  bool FileExists(const char *file) {
    return fm_->FileExists(file, NULL);
  }

  bool IsDirectlyAccessible(const char *file) {
    return fm_->IsDirectlyAccessible(file, NULL);
  }

  Date GetLastModifiedTime(const char *file) {
    return Date(fm_->GetLastModifiedTime(file));
  }

  static bool GetFile(const char *file, std::vector<std::string> *files) {
    files->push_back(file);
    return true;
  }

  ScriptableArray *GetAllFiles() {
    std::vector<std::string> files;
    fm_->EnumerateFiles("", NewSlot(GetFile, &files));
    return ScriptableArray::Create(files.begin(), files.end());
  }

  bool CopyFile(const char *src_file, const char *dest_file, bool overwrite) {
    std::string contents;
    return fm_->ReadFile(src_file, &contents) &&
           fm_->WriteFile(dest_file, contents, overwrite);
  }

  FileManagerInterface *fm_;
};

class DesignerUtils : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xd83de55b392c4d56, ScriptableInterface);

  DesignerUtils() {
    StringAppendPrintf(&designee_options_name_, "designee-options-%p", this);
  }
  ~DesignerUtils() {
    RemoveGadget();
  }

  virtual void DoRegister() {
    RegisterMethod("elementCoordToAncestor",
                   NewSlot(this, &DesignerUtils::ElementCoordToAncestor));
    RegisterMethod("ancestorCoordToElement",
                   NewSlot(this, &DesignerUtils::AncestorCoordToElement));
    RegisterMethod("isPointIn",
                   NewSlot(this, &DesignerUtils::IsPointIn));
    RegisterMethod("getOffsetPinX",
                   NewSlot(this, &DesignerUtils::GetOffsetPinX));
    RegisterMethod("getOffsetPinY",
                   NewSlot(this, &DesignerUtils::GetOffsetPinY));
    RegisterMethod("initGadgetFileManager",
                   NewSlot(this, &DesignerUtils::InitGadgetFileManager));
    RegisterMethod("getGlobalFileManager",
                   NewSlot(this, &DesignerUtils::GetGlobalFileManager));
    RegisterMethod("showXMLOptionsDialog",
                   NewSlot(this, &DesignerUtils::ShowXMLOptionsDialog));
    RegisterMethod("setDesignerMode",
                   NewSlot(this, &DesignerUtils::SetDesignerMode));
    RegisterMethod("systemOpenFileWith",
                   NewSlot(this, &DesignerUtils::SystemOpenFileWith));
    RegisterMethod("runGadget", NewSlot(this, &DesignerUtils::RunGadget));
    RegisterMethod("removeGadget", NewSlot(this, &DesignerUtils::RemoveGadget));
    RegisterMethod("generateUUID", NewSlot(GenerateUUID));
    RegisterMethod("getUserName", NewSlot(GetUserLoginName));
    RegisterMethod("parseColor", NewSlot(ParseColor));
    RegisterMethod("toColorString", NewSlot(ToColorString));
  }

  JSONString ElementCoordToAncestor(const BasicElement *element,
                                    const BasicElement *ancestor,
                                    double x, double y) {
    while (element && element != ancestor) {
      element->SelfCoordToParentCoord(x, y, &x, &y);
      element = element->GetParentElement();
    }
    return JSONString(StringPrintf("{\"x\":%f,\"y\":%f}", x, y));
  }

  JSONString AncestorCoordToElement(const BasicElement *ancestor,
                                    const BasicElement *element,
                                    double x, double y) {
    std::vector<const BasicElement *> path;
    while (element && element != ancestor) {
      path.push_back(element);
      element = element->GetParentElement();
    }
    for (size_t i = path.size(); i > 0; i--) {
      path[i - 1]->ParentCoordToSelfCoord(x, y, &x, &y);
    }
    return JSONString(StringPrintf("{\"x\":%f,\"y\":%f}", x, y));
  }

  bool IsPointIn(const BasicElement *element, double x, double y) {
    return element->IsPointIn(x, y);
  }

  double GetOffsetPinX(const BasicElement *element) {
    return element->GetPixelPinX();
  }

  double GetOffsetPinY(const BasicElement *element) {
    return element->GetPixelPinY();
  }

  ScriptableFileManager *InitGadgetFileManager(const char *gadget_path) {
    if (!gadget_path || !*gadget_path)
      return NULL;

    if (g_gadget_file_manager) {
      g_designer_file_manager->UnregisterFileManager(kGadgetFileManagerPrefix,
                                                     g_gadget_file_manager);
      delete g_gadget_file_manager;
    }
    std::string path, filename;
    SplitFilePath(gadget_path, &path, &filename);

    // Uses the parent path of base_path if it refers to a manifest file.
    if (filename != kGadgetGManifest)
      path = gadget_path;

    g_gadget_file_manager = CreateFileManager(path.c_str());
    if (g_gadget_file_manager) {
      g_designer_file_manager->RegisterFileManager(kGadgetFileManagerPrefix,
                                                   g_gadget_file_manager);
      return new ScriptableFileManager(g_gadget_file_manager);
    }
    return NULL;
  }

  ScriptableFileManager *GetGlobalFileManager() {
    return new ScriptableFileManager(::ggadget::GetGlobalFileManager());
  }

  void ShowXMLOptionsDialog(const char *xml_file, ScriptableInterface *param) {
    g_designer_gadget->ShowXMLOptionsDialog(
        ViewInterface::OPTIONS_VIEW_FLAG_OK |
        ViewInterface::OPTIONS_VIEW_FLAG_CANCEL,
        xml_file, param);
  }

  void SetDesignerMode(BasicElement *element) {
    element->SetDesignerMode(true);
  }

  void SystemOpenFileWith(const char *command, const char *file) {
    pid_t pid;
    // fork and run the command.
    if ((pid = fork()) == 0) {
      if (fork() != 0)
        _exit(0);
      execlp(command, command, file, NULL);
      DLOG("Failed to exec command: %s", command);
      _exit(-1);
    }
  }

  class RunGadgetWatchCallback : public WatchCallbackInterface {
   public:
    RunGadgetWatchCallback(const std::string &gadget_path,
                           const std::string &options_name)
        : gadget_path_(gadget_path), options_name_(options_name) {
    }

    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      // The gadget manager always use positive instance ids, so this designee_id
      // won't conflict with them.
      int designee_id = -g_designer_gadget->GetInstanceID() - 1;
      if (designee_id >= 0) {
        LOG("This designer can't run gadgets if the designer is running in "
            "another designer");
        return false;
      }

      StringMap manifest;
      Permissions permissions;
      if (Gadget::GetGadgetManifest(gadget_path_.c_str(), &manifest)) {
        Gadget::GetGadgetRequiredPermissions(&manifest, &permissions);
        permissions.GrantAllRequired();
        Gadget::SaveGadgetInitialPermissions(options_name_.c_str(),
                                             permissions);
      } else {
        LOG("Failed to load gadget's required permissions information.");
      }

      g_designee_gadget = g_designer_gadget->GetHost()->LoadGadget(
          gadget_path_.c_str(), options_name_.c_str(), designee_id, true);

      if (g_designee_gadget && g_designee_gadget->IsValid()) {
        g_designee_gadget->ShowMainView();
        g_designee_close_connection = g_designee_gadget->GetMainView()->
            ConnectOnCloseEvent(NewSlot(ResetDesigneeGadget));
      }
      return false;
    }

    virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      delete this;
    }

    static void ResetDesigneeGadget() {
      g_designee_gadget = NULL;
    }

   private:
    std::string gadget_path_;
    std::string options_name_;
  };

  void RunGadget(const char *gadget_path) {
    if (!gadget_path || !*gadget_path)
      return;

    RemoveGadget();
    // Schedule actual gadget run, because Gadget::RemoveMe() scheduled the
    // actual gadget removal in the next main loop.
    GetGlobalMainLoop()->AddTimeoutWatch(
        0, new RunGadgetWatchCallback(gadget_path, designee_options_name_));
  }

  void RemoveGadget() {
    if (g_designee_gadget) {
      g_designee_close_connection->Disconnect();
      g_designee_gadget->RemoveMe(false);
      g_designee_gadget = NULL;
    }

    OptionsInterface *options = CreateOptions(designee_options_name_.c_str());
    if (options) {
      options->DeleteStorage();
      delete options;
    }
  }

  static std::string GenerateUUID() {
    UUID uuid;
    uuid.Generate();
    return uuid.GetString();
  }

  static JSONString ParseColor(const char *color_str) {
    Color color;
    double opacity = 0;
    if (!Color::FromString(color_str, &color, &opacity))
      return JSONString("");
    return JSONString(StringPrintf(
        "{\"red\":%d,\"green\":%d,\"blue\":%d,\"opacity\":%d}",
        color.RedInt(), color.GreenInt(), color.BlueInt(),
        static_cast<int>(round(opacity * 255))));
  }

  static std::string ToColorString(int r, int g, int b, int opacity) {
    r = std::min(255, std::max(0, r));
    g = std::min(255, std::max(0, g));
    b = std::min(255, std::max(0, b));
    opacity = std::min(255, std::max(0, opacity));
    return opacity == 255 ? StringPrintf("#%02X%02X%02X", r, g, b) :
                            StringPrintf("#%02X%02X%02X%02X", opacity, r, g, b);
  }

 private:
  std::string designee_options_name_;
};

static DesignerUtils g_designer_utils;

} // namespace designer
} // namespace ggadget

#define Initialize designer_script_utils_LTX_Initialize
#define Finalize designer_script_utils_LTX_Finalize
#define RegisterScriptExtension \
    designer_script_utils_LTX_RegisterScriptExtension
#define RegisterFileManagerExtension \
    designer_script_utils_LTX_RegisterFileManagerExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize designer_script_utils extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize designer_script_utils extension.");
  }

  bool RegisterScriptExtension(ggadget::ScriptContextInterface *context,
                               ggadget::GadgetInterface *gadget) {
    ASSERT(context);
    ASSERT(gadget);
    ASSERT(gadget->IsInstanceOf(ggadget::Gadget::TYPE_ID));
    if (context) {
      if (!context->AssignFromNative(
          NULL, NULL, "designerUtils",
          ggadget::Variant(&ggadget::designer::g_designer_utils))) {
        LOG("Failed to register designerUtils.");
        return false;
      }
      ggadget::designer::g_designer_gadget =
          ggadget::down_cast<ggadget::Gadget*>(gadget);
      return true;
    }
    return false;
  }

  bool RegisterFileManagerExtension(ggadget::FileManagerWrapper *fm) {
    LOGI("Register designer_script_utils file manager extension.");
    ASSERT(fm);
    ggadget::designer::g_designer_file_manager = fm;
    return true;
  }
}
