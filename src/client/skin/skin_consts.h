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

#ifndef GOOPY_SKIN_SKIN_CONSTS_H_
#define GOOPY_SKIN_SKIN_CONSTS_H_

#include <string>

namespace ime_goopy {
namespace skin {

// Skin resources file name.
extern const char kSkinResourcesFileName[];

// We use ".png" image files by default.
extern const char kDefaultImageSuffix[];

// Version of Ime Skin's API (Skin package format). Do not forget to bump this
// version when adding new features.
extern const char kImeSkinAPIVersion[];

// Filename of Skin's manifest file.
extern const char kImeSkinManifest[];

// Filename of Skin's composing view xml file. It's for both vertical and
// horizontal orientations. If vertical and horizontal layouts are differents
// then kHorizontalComposingViewXML and kVerticalComposingViewXML should be
// used.
extern const char kComposingViewXML[];

// Filename of Skin's horizontal composing view xml file.
extern const char kHorizontalComposingViewXML[];

// Filename of Skin's vertical composing view xml file.
extern const char kVerticalComposingViewXML[];

// Filename of Skin's toolbar view xml file.
extern const char kToolbarViewXML[];

// Filename of Skin's right to left composing view file.
extern const char kRTLComposingViewXML[];

// Filename of Skin's right to left horizontal composing view file.
extern const char kRTLHorizontalComposingViewXML[];

// Filename of Skin's right to left vertical composing view file.
extern const char kRTLVerticalComposingViewXML[];

// Filename of Skin's right to left toolbar view file.
extern const char kRTLToolbarViewXML[];

// Filename of Skin's virtual keyboard view xml file.
extern const char kVirtualKeyboardViewXML[];

// Filename of Skin's virtual keyboard view xml file.
extern const char kVirtualKeyboard102ViewXML[];

// The tag name of the root element in Skin's manifest file.
extern const char kImeSkinTag[];

// XPath identifiers in ime_skin.manifest file.
// The minimum required Ime Skin API version to run the skin package.
extern const char kImeSkinManifestMinVersion[];

// Sample text for text size measurement.
extern const char kSampleTextForMeasurement[];

// Predefined element names for composing view.
// An linear element that contains the composition element and help tips.
extern const char kCompositionContainerElement[];
extern const char kCompositionElement[];
extern const char kHelpMessageLabel[];
extern const char kGoogleSearchButton[];
extern const char kCandidateListElement[];
extern const char kCandidateListPageUpButton[];
extern const char kCandidateListPageDownButton[];

// Predefined element names for toolbar view.
extern const char kToolbarElement[];
extern const char kImeLogoElement[];
extern const char kImeSelectionButton[];
extern const char kChineseEnglishModeButton[];
extern const char kFullHalfWidthCharacterModeButton[];
extern const char kFullHalfWidthPunctuationModeButton[];
extern const char kSoftKeyboardButton[];
extern const char kVoiceInputButton[];
extern const char kImeMenuButton[];
extern const char kToolbarCollapseExpandButton[];
extern const char kImeSkinAboutButton[];

// Predefined image/icon names.
extern const char kCandidateMenuIcon[];
extern const char kCandidateMenuDownIcon[];
extern const char kCandidateMenuOverIcon[];
extern const char kCandidateMenuDisabledIcon[];

extern const char kSelectedCandidateMenuIcon[];
extern const char kSelectedCandidateMenuDownIcon[];
extern const char kSelectedCandidateMenuOverIcon[];
extern const char kSelectedCandidateMenuDisabledIcon[];

extern const char kCandidateSelectionImage[];

extern const char kChineseModeIcon[];
extern const char kChineseModeDownIcon[];
extern const char kChineseModeOverIcon[];
extern const char kChineseModeDisabledIcon[];

extern const char kEnglishModeIcon[];
extern const char kEnglishModeDownIcon[];
extern const char kEnglishModeOverIcon[];
extern const char kEnglishModeDisabledIcon[];

extern const char kCapsLockModeIcon[];
extern const char kCapsLockModeDownIcon[];
extern const char kCapsLockModeOverIcon[];
extern const char kCapsLockModeDisabledIcon[];

extern const char kFullWidthCharacterModeIcon[];
extern const char kFullWidthCharacterModeDownIcon[];
extern const char kFullWidthCharacterModeOverIcon[];
extern const char kFullWidthCharacterModeDisabledIcon[];

extern const char kHalfWidthCharacterModeIcon[];
extern const char kHalfWidthCharacterModeDownIcon[];
extern const char kHalfWidthCharacterModeOverIcon[];
extern const char kHalfWidthCharacterModeDisabledIcon[];

extern const char kFullWidthPunctuationModeIcon[];
extern const char kFullWidthPunctuationModeDownIcon[];
extern const char kFullWidthPunctuationModeOverIcon[];
extern const char kFullWidthPunctuationModeDisabledIcon[];

extern const char kHalfWidthPunctuationModeIcon[];
extern const char kHalfWidthPunctuationModeDownIcon[];
extern const char kHalfWidthPunctuationModeOverIcon[];
extern const char kHalfWidthPunctuationModeDisabledIcon[];

extern const char kOpenSoftKeyboardIcon[];
extern const char kOpenSoftKeyboardDownIcon[];
extern const char kOpenSoftKeyboardOverIcon[];
extern const char kOpenSoftKeyboardDisabledIcon[];

extern const char kCloseSoftKeyboardIcon[];
extern const char kCloseSoftKeyboardDownIcon[];
extern const char kCloseSoftKeyboardOverIcon[];
extern const char kCloseSoftKeyboardDisabledIcon[];

extern const char kOpenVoiceInputIcon[];
extern const char kOpenVoiceInputDownIcon[];
extern const char kOpenVoiceInputOverIcon[];
extern const char kOpenVoiceInputDisabledIcon[];

extern const char kCloseVoiceInputIcon[];
extern const char kCloseVoiceInputDownIcon[];
extern const char kCloseVoiceInputOverIcon[];
extern const char kCloseVoiceInputDisabledIcon[];

extern const char kImeMenuIcon[];
extern const char kImeMenuDownIcon[];
extern const char kImeMenuOverIcon[];
extern const char kImeMenuDisabledIcon[];

extern const char kImeSkinAboutIcon[];
extern const char kImeSkinAboutDownIcon[];
extern const char kImeSkinAboutOverIcon[];
extern const char kImeSkinAboutDisabledIcon[];

extern const char kToolbarCollapseIcon[];
extern const char kToolbarCollapseDownIcon[];
extern const char kToolbarCollapseOverIcon[];
extern const char kToolbarCollapseDisabledIcon[];

extern const char kToolbarExpandIcon[];
extern const char kToolbarExpandDownIcon[];
extern const char kToolbarExpandOverIcon[];
extern const char kToolbarExpandDisabledIcon[];

extern const char kManifestCategory[];

// Returns the path to a file in the global resources package.
std::string GetFilePathInGlobalResources(const char* basename,
                                         const char* suffix);

// Returns the path to a default image file in the global resources package.
std::string GetDefaultImageFilePath(const char* basename);

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_SKIN_CONSTS_H_
