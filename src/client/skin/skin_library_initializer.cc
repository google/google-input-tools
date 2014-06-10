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

#include "skin/skin_library_initializer.h"
#ifndef GDIPVER
#define GDIPVER 0x0110
#endif
#include <gdiplus.h>
#include "base/logging.h"
#include "base/string_utils_win.h"
#include "common/app_utils.h"
#include "skin/skin_consts.h"
#pragma push_macro("DLOG")
#pragma push_macro("LOG")
#include "third_party/google_gadgets_for_linux/ggadget/file_manager_factory.h"
#include "third_party/google_gadgets_for_linux/ggadget/file_manager_wrapper.h"
#include "third_party/google_gadgets_for_linux/ggadget/gadget_consts.h"
#include "third_party/google_gadgets_for_linux/ggadget/localized_file_manager.h"
#include "third_party/google_gadgets_for_linux/ggadget/memory_options.h"
#include "third_party/google_gadgets_for_linux/ggadget/options_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/system_utils.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/gdiplus_font.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/main_loop.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/thread_local_singleton_holder.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/xml_parser.h"
#pragma pop_macro("LOG")
#pragma pop_macro("DLOG")

using ggadget::FileManagerWrapper;
using ggadget::win32::MainLoop;
using ggadget::win32::ThreadLocalSingletonHolder;
using ggadget::win32::XMLParser;

namespace ime_goopy {
namespace skin {

namespace {

ggadget::OptionsInterface* CreateMemoryOption(const char *name) {
  return new ggadget::MemoryOptions;
}

struct InitializationData {
  InitializationData()
    : log_level(ggadget::LOG_TRACE),
      long_log(true),
      log_listener_connection(NULL),
      reference_count(0),
      gdiplus_token(NULL) {
  }
  int reference_count;
  ULONG_PTR gdiplus_token;
  int log_level;
  bool long_log;
  ggadget::Connection* log_listener_connection;
};

// For debug mode, there are 2 environment variables controlling skin
// logging behaviors:
// GOOPY_SKIN_LOG_LEVEL can be 0, 1, 2 or 3. Please refer to LogLevel in
// logger.h. Default value is 0.
// GOOPY_SKIN_LONG_LOG can be 0 or 1. If it's 1, skin will add an UTC time
// with format "<hour>:<minute>:<second>.<microsecond>" at the beginning of
// each log. Default value is 1.
static const wchar_t kSkinLogLevelEnvVariable[] = L"GOOPY_SKIN_LOG_LEVEL";
static const wchar_t kSkinLongLogEnvVariable[] = L"GOOPY_SKIN_LONG_LOG";

static std::string DefaultLogListener(ggadget::LogLevel level,
                                      const char* filename,
                                      int line,
                                      const std::string& message) {
#ifdef _DEBUG
  InitializationData* initialization_data =
      ThreadLocalSingletonHolder<InitializationData>::GetValue();
  DCHECK(initialization_data);
  if (level >= initialization_data->log_level) {
    std::string output_message;
    if (initialization_data->long_log) {
      struct timeval tv;
      ggadget::gettimeofday(&tv, NULL);
      ggadget::StringAppendPrintf(&output_message,
                                  "%02d:%02d:%02d.%03d: ",
                                  static_cast<int>(tv.tv_sec / 3600 % 24),
                                  static_cast<int>(tv.tv_sec / 60 % 60),
                                  static_cast<int>(tv.tv_sec % 60),
                                  static_cast<int>(tv.tv_usec / 1000));
      if (filename) {
        // Print only the last part of the file name.
        const char *name = strrchr(filename, ggadget::kDirSeparator);
        if (name)
          filename = name + 1;
        ggadget::StringAppendPrintf(&output_message, "%s:%d: ", filename, line);
      }
    }
    output_message.append(message);
    output_message.append("\n");
    ::OutputDebugStringW(ime_goopy::Utf8ToWide(output_message).c_str());
  }
  return message;
#else
  return "";
#endif
}

static bool SetupSkinLogger() {
  InitializationData* initialization_data =
      ThreadLocalSingletonHolder<InitializationData>::GetValue();
  DCHECK(initialization_data);
  if (!initialization_data) return false;
#ifdef _DEBUG
  wchar_t log_level[MAX_PATH] = {0}, long_log[MAX_PATH] = {0};
  if (::GetEnvironmentVariable(kSkinLogLevelEnvVariable, log_level, MAX_PATH)) {
    initialization_data->log_level = _wtoi(log_level);
  }
  if (::GetEnvironmentVariable(kSkinLongLogEnvVariable, long_log, MAX_PATH)) {
    initialization_data->long_log = (_wtoi(long_log) > 0);
  }
#endif
  if (initialization_data->log_listener_connection == NULL) {
    initialization_data->log_listener_connection =
        ConnectGlobalLogListener(NewSlot(DefaultLogListener));
  }
  return true;
}

static bool RegisterFileManager(ggadget::FileManagerWrapper* wrapper,
                                ggadget::FileManagerInterface* file_manager) {
  ggadget::FileManagerInterface* localized_file_manager =
      new ggadget::LocalizedFileManager(file_manager);
  if (!wrapper->RegisterFileManager(ggadget::kGlobalResourcePrefix,
                                    localized_file_manager)) {
    delete localized_file_manager;
    return false;
  }
  return true;
}

static bool SetupSkinGlobalFileManager() {
  using ggadget::FileManagerInterface;
  ggadget::FileManagerWrapper* file_manager_wrapper = new FileManagerWrapper();
  bool is_registered = false;
#if defined(_DEBUG) && defined(PACKAGE_DIR)
  // Add a file manager based on compiler specified directory.
  std::string debug_resource_path =
      ggadget::BuildFilePath(PACKAGE_DIR, kSkinResourcesFileName, NULL);
  FileManagerInterface* debug_resource_file_manager =
      ggadget::CreateFileManager(debug_resource_path.c_str());
  if (debug_resource_file_manager != NULL) {
    is_registered = RegisterFileManager(file_manager_wrapper,
                                        debug_resource_file_manager);
  }
#endif
  if (!is_registered) {
    // Add a file manager based on program data
    std::wstring resources_name = ime_goopy::Utf8ToWide(kSkinResourcesFileName);
    std::string system_resources_path =
        ime_goopy::WideToUtf8(AppUtils::GetSystemDataFilePath(resources_name));
    FileManagerInterface* system_resource_file_manager =
        ggadget::CreateFileManager(system_resources_path.c_str());
    if (system_resource_file_manager != NULL) {
      is_registered = RegisterFileManager(file_manager_wrapper,
                                          system_resource_file_manager);
    }
  }

  if (!ggadget::SetGlobalFileManager(file_manager_wrapper)) {
    delete file_manager_wrapper;
    return false;
  }
  return is_registered;
}
}  // namespace

bool SkinLibraryInitializer::Initialize() {
  InitializationData* data =
      ThreadLocalSingletonHolder<InitializationData>::GetValue();
  if (data != NULL && data->reference_count != 0) {
    ++data->reference_count;
    return true;
  } else if (data == NULL) {
    data = new InitializationData;
    if (!ThreadLocalSingletonHolder<InitializationData>::SetValue(data)) {
      delete data;
      return false;
    }
  }
  data->reference_count = 1;

  MainLoop* main_loop = new MainLoop;
  ggadget::SetGlobalMainLoop(main_loop);
  if (!SetupSkinLogger()) return false;
  // SetupSkinGlobalFileManager will write some log, so SetupSkinLogger and
  // SetGlobalMainLoop should be called before.
  if (!SetupSkinGlobalFileManager()) return false;

  XMLParser* xml_parser = new XMLParser;
  if (!xml_parser) return false;
  ggadget::SetXMLParser(xml_parser);

  // Test if options factory is set by creating an option.
  ggadget::OptionsInterface* test_option = ggadget::CreateOptions("");
  if (!test_option)
    ggadget::SetOptionsFactory(CreateMemoryOption);
  else
    delete test_option;
  Gdiplus::GdiplusStartupInput input;
  return Gdiplus::GdiplusStartup(&(data->gdiplus_token), &input, NULL) ==
         Gdiplus::Ok;
}

void SkinLibraryInitializer::Finalize() {
  InitializationData* data =
      ThreadLocalSingletonHolder<InitializationData>::GetValue();
  DCHECK(data);
  if (!data) return;
  --data->reference_count;
  if (data->reference_count != 0)
    return;
  Gdiplus::GdiplusShutdown(data->gdiplus_token);
  // Clear the static fonts defined in gdiplusheaders.h as they are no longer
  // available after GdiplusShutdown.
  ggadget::win32::GdiplusFont::ClearStaticFonts();
  if (ggadget::GetGlobalFileManager())
    delete ::down_cast<FileManagerWrapper*>(ggadget::GetGlobalFileManager());
  if (ggadget::GetGlobalMainLoop())
    delete ::down_cast<MainLoop*>(ggadget::GetGlobalMainLoop());
  if (ggadget::GetXMLParser())
    delete ::down_cast<XMLParser*>(ggadget::GetXMLParser());
  ggadget::SetGlobalFileManager(NULL);
  ggadget::SetGlobalMainLoop(NULL);
  ggadget::SetXMLParser(NULL);
  // finalize logger
  DCHECK(data->log_listener_connection);
  if (data->log_listener_connection) {
    data->log_listener_connection->Disconnect();
    data->log_listener_connection = NULL;
  }
  ggadget::FinalizeLogger();
}
}  // namespace skin
}  // namespace ggadget
