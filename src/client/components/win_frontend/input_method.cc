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

#define _ATL_NO_HOSTING
#include <atlbase.h>

#include "appsensorapi/appsensor.h"
#include "appsensorapi/appsensor_helper.h"
#include "appsensorapi/handlers/cross_fire_handler.h"
#include "appsensorapi/handlers/mspub_handler.h"
#include "appsensorapi/handlers/wow.h"
#include "base/logging.h"
#include "base/singleton.h"
#include "common/app_const.h"
#include "common/app_utils.h"
#include "common/framework_interface.h"
#include "components/win_frontend/frontend_factory.h"
#include "components/win_frontend/ipc_ui_manager.h"
#include "components/win_frontend/resource.h"
#include "components/win_frontend/text_styles.h"
#include "imm/immdev.h"

namespace ime_goopy {
// TODO(haicsun): localizes display name for each dll entry, and modifies ui
// class name.
const wchar_t InputMethod::kUIClassName[] = L"GOOGINPUTTOOLS";
const wchar_t InputMethod::kDisplayName[] =
  L"Google Input Tools"
  T13N_SHORT_VERSION;

const DWORD InputMethod::kConversionModeMask = IME_CMODE_FULLSHAPE |
                                               IME_CMODE_NATIVE |
                                               IME_CMODE_SYMBOL;
const DWORD InputMethod::kSentenceModeMask = IME_SMODE_NONE;
const DWORD InputMethod::kImmProperty =
    IME_PROP_CANDLIST_START_FROM_1 |
    IME_PROP_UNICODE |
    IME_PROP_END_UNLOAD |
    IME_PROP_KBD_CHAR_FIRST |
    IME_PROP_COMPLETE_ON_UNSELECT |
    // This property is disabled to allow application to show its own inline
    // composition window.
    // Goopy2 enables this property to fix a bug in QQ2009/TM2008 to prevent
    // application to show its own inline composition window, because they
    // give us wrong caret coordinate in inline mode.
    // IME_PROP_SPECIAL_UI |
    IME_PROP_AT_CARET |
    // Add IME_PROP_NEED_ALTKEY because virtual keyboard requires ctrl + atl +
    // key.
    IME_PROP_NEED_ALTKEY;
bool InputMethod::ShowConfigureWindow(HWND parent) {
  if (AppUtils::GetInstalledVersion(kFrameworkPackName).empty()) return false;
  AppUtils::LaunchOptions();
  return true;
}
// {B35B9698-3701-4C40-9444-4B2AE1B4E591}
const CLSID InputMethod::kTextServiceClsid =
    {0xb35b9698, 0x3701, 0x4c40,
     {0x94, 0x44, 0x4b, 0x2a, 0xe1, 0xb4, 0xe5, 0x91}};

// {E0FD6A19-23A5-4A3E-BA20-54CD24670664}
const GUID InputMethod::kInputAttributeGUID =
    {0xe0fd6a19, 0x23a5, 0x4a3e,
     {0xba, 0x20, 0x54, 0xcd, 0x24, 0x67, 0x6, 0x64}};

const int InputMethod::kRegistrarScriptId = IDR_TSF;
EngineInterface* InputMethod::CreateEngine(ContextInterface* context) {
  DCHECK(context);
  // Register appsensor handler for client game "World of Warcraft"
  if (ime_goopy::AppSensorHelper::Instance()->Init()) {
    ime_goopy::AppSensorHelper::Instance()->RegisterHandler(
        new ime_goopy::handlers::WowHandler);
    ime_goopy::AppSensorHelper::Instance()->RegisterHandler(
        new ime_goopy::handlers::CrossFireHandler);
    ime_goopy::AppSensorHelper::Instance()->RegisterHandler(
        new ime_goopy::handlers::MSPubHandler);
  }

  return FrontendFactory::UnshelveOrCreateFrontend(context->GetId());
}

void InputMethod::DestroyEngineOfContext(ContextInterface* context) {
  EngineInterface* engine = context->GetEngine();
  if (engine) {
    context->DetachEngine();
    engine->SetContext(NULL);
    // Instead of destroying the engine, we shelve the engine to preserve the
    // states in case user is switching between two of our input methods.
    FrontendFactory::ShelveFrontend(context->GetId(), engine);
  }
}

UIManagerInterface* InputMethod::CreateUIManager(HWND parent) {
  return new components::IPCUIManager();
}

int InputMethod::GetTextStyleCount() {
  return components::STOCKSTYLE_COUNT;
}

void InputMethod::GetTextStyle(int index, TextStyle *style) {
    if (index < 0 || index >= GetTextStyleCount()) {
    DCHECK(false);
    return;
  }
    if (!style) {
    DCHECK(false);
    return;
  }
  *style = components::kStockTextStyles[index];
}

}  // namespace ime_goopy
