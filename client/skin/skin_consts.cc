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

#include "skin/skin_consts.h"

#include "third_party/google_gadgets_for_linux/ggadget/gadget_consts.h"

namespace ime_goopy {
namespace skin {

#ifdef SKIN_RESOURCES_FILE
const char kSkinResourcesFileName[] = SKIN_RESOURCES_FILE;
#else
const char kSkinResourcesFileName[] = "skin_resources.dat";
#endif

const char kDefaultImageSuffix[] = ".png";

const char kImeSkinAPIVersion[] = "1.0.0.0";

const char kImeSkinManifest[] = "ime_skin.manifest";

const char kComposingViewXML[] = "composing.xml";
const char kVerticalComposingViewXML[] = "v_composing.xml";
const char kHorizontalComposingViewXML[] = "h_composing.xml";
const char kVirtualKeyboardViewXML[] = "virtual_keyboard.xml";
const char kVirtualKeyboard102ViewXML[] = "virtual_keyboard_102.xml";

const char kToolbarViewXML[] = "toolbar.xml";

const char kRTLComposingViewXML[] = "composing_rtl.xml";
const char kRTLVerticalComposingViewXML[] = "v_composing_rtl.xml";
const char kRTLHorizontalComposingViewXML[] = "h_composing_rtl.xml";
const char kRTLToolbarViewXML[] = "toolbar_rtl.xml";

const char kImeSkinTag[] = "ime_skin";

const char kImeSkinManifestMinVersion[] = "@minimumGoogleImeSkinVersion";

const char kSampleTextForMeasurement[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "\xe8\xb0\xb7\xe6\xad\x8c\xe6\x8b\xbc\xe9\x9f\xb3"
    "\xe8\xbe\x93\xe5\x85\xa5\xe6\xb3\x95";

const char kCompositionContainerElement[] = "composition_container";
const char kCompositionElement[] = "composition";
const char kHelpMessageLabel[] = "help_message";
const char kGoogleSearchButton[] = "google_search";
const char kCandidateListElement[] = "candidate_list";
const char kCandidateListPageUpButton[] = "candidate_list_page_up";
const char kCandidateListPageDownButton[] = "candidate_list_page_down";

const char kToolbarElement[] = "toolbar";
const char kImeLogoElement[] = "ime_logo";
const char kImeSelectionButton[] = "ime_selection";
const char kChineseEnglishModeButton[] = "chinese_english_mode";
const char kFullHalfWidthCharacterModeButton[] =
    "full_half_width_character_mode";
const char kFullHalfWidthPunctuationModeButton[] =
    "full_half_width_punctuation_mode";
const char kSoftKeyboardButton[] = "soft_keyboard";
const char kVoiceInputButton[] = "voice_input";
const char kImeMenuButton[] = "ime_menu";
const char kToolbarCollapseExpandButton[] = "toolbar_collapse_expand";
const char kImeSkinAboutButton[] = "ime_skin_about";

const char kCandidateMenuIcon[] = "candidate_menu";
const char kCandidateMenuDownIcon[] = "candidate_menu_down";
const char kCandidateMenuOverIcon[] = "candidate_menu_over";
const char kCandidateMenuDisabledIcon[] = "candidate_menu_disabled";

const char kSelectedCandidateMenuIcon[] = "selected_candidate_menu";
const char kSelectedCandidateMenuDownIcon[] = "selected_candidate_menu_down";
const char kSelectedCandidateMenuOverIcon[] = "selected_candidate_menu_over";
const char kSelectedCandidateMenuDisabledIcon[] =
    "selected_candidate_menu_disabled";

const char kCandidateSelectionImage[] =
    "candidate_selection";

const char kChineseModeIcon[] = "chinese_mode";
const char kChineseModeDownIcon[] = "chinese_mode_down";
const char kChineseModeOverIcon[] = "chinese_mode_over";
const char kChineseModeDisabledIcon[] = "chinese_mode_disabled";

const char kEnglishModeIcon[] = "english_mode";
const char kEnglishModeDownIcon[] = "english_mode_down";
const char kEnglishModeOverIcon[] = "english_mode_over";
const char kEnglishModeDisabledIcon[] = "english_mode_disabled";

extern const char kCapsLockModeIcon[] = "caps_lock_mode";
extern const char kCapsLockModeDownIcon[] = "caps_lock_mode_down";
extern const char kCapsLockModeOverIcon[] = "caps_lock_mode_over";
extern const char kCapsLockModeDisabledIcon[] = "caps_lock_mode_disabled";

const char kFullWidthCharacterModeIcon[] = "full_width_character_mode";
const char kFullWidthCharacterModeDownIcon[] = "full_width_character_mode_down";
const char kFullWidthCharacterModeOverIcon[] = "full_width_character_mode_over";
const char kFullWidthCharacterModeDisabledIcon[] =
    "full_width_character_mode_disabled";

const char kHalfWidthCharacterModeIcon[] = "half_width_character_mode";
const char kHalfWidthCharacterModeDownIcon[] = "half_width_character_mode_down";
const char kHalfWidthCharacterModeOverIcon[] = "half_width_character_mode_over";
const char kHalfWidthCharacterModeDisabledIcon[] =
    "half_width_character_mode_disabled";

const char kFullWidthPunctuationModeIcon[] = "full_width_punctuation_mode";
const char kFullWidthPunctuationModeDownIcon[] =
    "full_width_punctuation_mode_down";
const char kFullWidthPunctuationModeOverIcon[] =
    "full_width_punctuation_mode_over";
const char kFullWidthPunctuationModeDisabledIcon[] =
    "full_width_punctuation_mode_disabled";

const char kHalfWidthPunctuationModeIcon[] = "half_width_punctuation_mode";
const char kHalfWidthPunctuationModeDownIcon[] =
    "half_width_punctuation_mode_down";
const char kHalfWidthPunctuationModeOverIcon[] =
    "half_width_punctuation_mode_over";
const char kHalfWidthPunctuationModeDisabledIcon[] =
    "half_width_punctuation_mode_disabled";

const char kOpenSoftKeyboardIcon[] = "open_soft_keyboard";
const char kOpenSoftKeyboardDownIcon[] = "open_soft_keyboard_down";
const char kOpenSoftKeyboardOverIcon[] = "open_soft_keyboard_over";
const char kOpenSoftKeyboardDisabledIcon[] = "open_soft_keyboard_disabled";

const char kCloseSoftKeyboardIcon[] = "close_soft_keyboard";
const char kCloseSoftKeyboardDownIcon[] = "close_soft_keyboard_down";
const char kCloseSoftKeyboardOverIcon[] = "close_soft_keyboard_over";
const char kCloseSoftKeyboardDisabledIcon[] = "close_soft_keyboard_disabled";

const char kOpenVoiceInputIcon[] = "open_voice_input";
const char kOpenVoiceInputDownIcon[] = "open_voice_input_down";
const char kOpenVoiceInputOverIcon[] = "open_voice_input_over";
const char kOpenVoiceInputDisabledIcon[] = "open_voice_input_disabled";

const char kCloseVoiceInputIcon[] = "close_voice_input";
const char kCloseVoiceInputDownIcon[] = "close_voice_input_down";
const char kCloseVoiceInputOverIcon[] = "close_voice_input_over";
const char kCloseVoiceInputDisabledIcon[] = "close_voice_input_disabled";

const char kImeMenuIcon[] = "ime_menu";
const char kImeMenuDownIcon[] = "ime_menu_down";
const char kImeMenuOverIcon[] = "ime_menu_over";
const char kImeMenuDisabledIcon[] = "ime_menu_disabled";

const char kImeSkinAboutIcon[] = "ime_skin_about";
const char kImeSkinAboutDownIcon[] = "ime_skin_about_down";
const char kImeSkinAboutOverIcon[] = "ime_skin_about_over";
const char kImeSkinAboutDisabledIcon[] = "ime_skin_about_disabled";

const char kToolbarCollapseIcon[] = "toolbar_collapse";
const char kToolbarCollapseDownIcon[] = "toolbar_collapse_down";
const char kToolbarCollapseOverIcon[] = "toolbar_collapse_over";
const char kToolbarCollapseDisabledIcon[] = "toolbar_collapse_disabled";

const char kToolbarExpandIcon[] = "toolbar_expand";
const char kToolbarExpandDownIcon[] = "toolbar_expand_down";
const char kToolbarExpandOverIcon[] = "toolbar_expand_over";
const char kToolbarExpandDisabledIcon[] = "toolbar_expand_disabled";

const char kManifestCategory[] = "about/category";

std::string GetFilePathInGlobalResources(const char* basename,
                                         const char* suffix) {
  std::string path(ggadget::kGlobalResourcePrefix);
  if (basename && *basename) {
    path.append(basename);
    if (suffix && *suffix)
      path.append(suffix);
  }
  return path;
}

std::string GetDefaultImageFilePath(const char* basename) {
  return GetFilePathInGlobalResources(basename, kDefaultImageSuffix);
}

}  // namespace skin
}  // namespace ime_goopy
