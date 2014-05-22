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

#ifndef GGADGET_GADGET_CONSTS_H__
#define GGADGET_GADGET_CONSTS_H__

#include <map>
#include <string>
#include <ggadget/common.h>

namespace ggadget {

/** Character to separate directories */
#if defined(OS_WIN)
const char kDirSeparator = '\\';
const char kDirSeparatorStr[] = "\\";
#elif defined(OS_POSIX)
const char kDirSeparator = '/';
const char kDirSeparatorStr[] = "/";
#endif

/** Character to separate search paths, eg. used in PATH environment var */
const char kSearchPathSeparator = ':';
const char kSearchPathSeparatorStr[] = ":";

/** Standard suffix of gadget file. */
const char kGadgetFileSuffix[] = ".gg";

/** Filenames of required resources in each .gg package. */
const char kMainXML[]         = "main.xml";
const char kOptionsXML[]      = "options.xml";
const char kStringsXML[]      = "strings.xml";
const char kGadgetGManifest[] = "gadget.gmanifest";
const char kXMLExt[]          = ".xml";

/** Information XPath identifiers in gadget.gmanifest file. */
const char kManifestMinVersion[]  = "@minimumGoogleDesktopVersion";
const char kManifestId[]          = "about/id";
const char kManifestName[]        = "about/name";
const char kManifestDescription[] = "about/description";
const char kManifestVersion[]     = "about/version";
const char kManifestAuthor[]      = "about/author";
const char kManifestAuthorEmail[] = "about/authorEmail";
const char kManifestAuthorWebsite[]
                                  = "about/authorWebsite";
const char kManifestCopyright[]   = "about/copyright";
const char kManifestAboutText[]   = "about/aboutText";
const char kManifestSmallIcon[]   = "about/smallIcon";
const char kManifestIcon[]        = "about/icon";
const char kManifestDisplayAPI[]  = "about/displayAPI";
const char kManifestDisplayAPIUseNotifier[]
                                  = "about/displayAPI@useNotifier";
const char kManifestEventAPI[]    = "about/eventAPI";
const char kManifestQueryAPI[]    = "about/queryAPI";
const char kManifestQueryAPIAllowModifyIndex[]
                                  = "about/queryAPI@allowModifyIndex";

// TODO: make the "linux" part a configurable parameter.
const char kManifestPlatformSupported[] =
    "platform/" GGL_PLATFORM "@supported";
const char kManifestPlatformMinVersion[] =
    "platform/" GGL_PLATFORM "@minimumGadgetHostVersion";

/**
 * To enumerate all fonts to be installed, you must enumerate the manifest
 * map and use SimpleMatchXPath(key, kManifestInstallFontSrc) to test if the
 * value is a font src attribute.
 */
const char kManifestInstallFontSrc[] = "install/font@src";

/**
 * To enumerate all objects to be installed, you must enumerate the manifest
 * map and use SimpleMatchXPath(key, kManifestInstallObject) and then
 * for each of the config items found, access their attributes with keys by
 * postpending "@name", "@clsid", "@src" to the keys.
 */
const char kManifestInstallObject[] = "install/object";
/**
 * Simple way to enumerate all objects to be installed, ignoring "name" and
 * "clsid" attributes.
 */
const char kManifestInstallObjectSrc[] = "install/object@src";

/** Permissions node key in manifest file. */
const char kManifestPermissions[] = "permissions";

/** The tag name of the root element in string table files. */
const char kStringsTag[] = "strings";
/** The tag name of the root element in view file. */
const char kViewTag[] = "view";
/** The tag name of the root element in gadget.gmanifest files. */
const char kGadgetTag[] = "gadget";
/** The tag name of script elements */
const char kScriptTag[] = "script";
/** The name of the 'src' attribute of script elements. */
const char kSrcAttr[] = "src";
/** The name of the 'name' attribute of elements. */
const char kNameAttr[] = "name";
/** The property name for elements to contain its text contents. */
const char kInnerTextProperty[] = "innerText";
/** The tag name of the contentarea element. */
const char kContentAreaTag[] = "contentarea";
/** The tag name for <param> subelement of <object>. */
const char kParamTag[] = "param";
/** The attribute name for param value. */
const char kValueAttr[] = "value";
/** The attribute name for object class id. */
const char kClassIdAttr[] = "classId";

/** Prefix for user profile files. */
const char kProfilePrefix[] = "profile://";

/** Prefix for global file resources. */
const char kGlobalResourcePrefix[] = "resource://";

/** Default directory to store profiles. */
const char kDefaultProfileDirectory[] = ".google/gadgets";

const char kDefaultFocusOverlay[] = "resource://focus_overlay.png";

const char kScrollDefaultBackgroundH[] = "resource://scroll_background_h.png";
const char kScrollDefaultGrippyH[] = "resource://scrollbar_grippy_h.png";
const char kScrollDefaultThumbH[] = "resource://scrollbar_u_h.png";
const char kScrollDefaultThumbDownH[] = "resource://scrollbar_d_h.png";
const char kScrollDefaultThumbOverH[] = "resource://scrollbar_o_h.png";
const char kScrollDefaultLeft[] = "resource://scrollleft_u.png";
const char kScrollDefaultLeftDown[] = "resource://scrollleft_d.png";
const char kScrollDefaultLeftOver[] = "resource://scrollleft_o.png";
const char kScrollDefaultRight[] = "resource://scrollright_u.png";
const char kScrollDefaultRightDown[] = "resource://scrollright_d.png";
const char kScrollDefaultRightOver[] = "resource://scrollright_o.png";

const char kScrollDefaultBackgroundV[] = "resource://scroll_background.png";
const char kScrollDefaultGrippyV[] = "resource://scrollbar_grippy.png";
const char kScrollDefaultThumbV[] = "resource://scrollbar_u.png";
const char kScrollDefaultThumbDownV[] = "resource://scrollbar_d.png";
const char kScrollDefaultThumbOverV[] = "resource://scrollbar_o.png";
const char kScrollDefaultUp[] = "resource://scrollup_u.png";
const char kScrollDefaultUpDown[] = "resource://scrollup_d.png";
const char kScrollDefaultUpOver[] = "resource://scrollup_o.png";
const char kScrollDefaultDown[] = "resource://scrolldown_u.png";
const char kScrollDefaultDownDown[] = "resource://scrolldown_d.png";
const char kScrollDefaultDownOver[] = "resource://scrolldown_o.png";

const char kComboArrow[] = "resource://combo_arrow_up.png";
const char kComboArrowDown[] = "resource://combo_arrow_down.png";
const char kComboArrowOver[] = "resource://combo_arrow_over.png";

const char kContentItemUnpinned[] = "resource://unpinned.png";
const char kContentItemUnpinnedOver[] = "resource://unpinned_over.png";
const char kContentItemPinned[] = "resource://pinned.png";

const char kButtonImage[] = "resource://button_up.png";
const char kButtonDownImage[] = "resource://button_down.png";
const char kButtonOverImage[] = "resource://button_over.png";
const char kCheckBoxImage[] = "resource://checkbox_up.png";
const char kCheckBoxDownImage[] = "resource://checkbox_down.png";
const char kCheckBoxOverImage[] = "resource://checkbox_over.png";
const char kCheckBoxCheckedImage[] = "resource://checkbox_checked_up.png";
const char kCheckBoxCheckedDownImage[] = "resource://checkbox_checked_down.png";
const char kCheckBoxCheckedOverImage[] = "resource://checkbox_checked_over.png";
const char kRadioImage[] = "resource://radio_up.png";
const char kRadioDownImage[] = "resource://radio_down.png";
const char kRadioOverImage[] = "resource://radio_over.png";
const char kRadioCheckedImage[] = "resource://radio_checked_up.png";
const char kRadioCheckedDownImage[] = "resource://radio_checked_down.png";
const char kRadioCheckedOverImage[] = "resource://radio_checked_over.png";

const char kProgressBarFullImage[] = "resource://progressbar_full.png";
const char kProgressBarEmptyImage[] = "resource://progressbar_empty.png";

// The following are used by the view decorator.
const char kVDButtonBackground[] = "resource://vd_button_background.png";
const char kVDButtonCloseNormal[] = "resource://vd_close_normal.png";
const char kVDButtonCloseOver[] = "resource://vd_close_over.png";
const char kVDButtonCloseDown[] = "resource://vd_close_down.png";
const char kVDButtonMenuNormal[] = "resource://vd_menu_normal.png";
const char kVDButtonMenuOver[] = "resource://vd_menu_over.png";
const char kVDButtonMenuDown[] = "resource://vd_menu_down.png";
const char kVDButtonBackNormal[] = "resource://vd_back_normal.png";
const char kVDButtonBackOver[] = "resource://vd_back_over.png";
const char kVDButtonBackDown[] = "resource://vd_back_down.png";
const char kVDButtonForwardNormal[] = "resource://vd_forward_normal.png";
const char kVDButtonForwardOver[] = "resource://vd_forward_over.png";
const char kVDButtonForwardDown[] = "resource://vd_forward_down.png";
const char kVDButtonPopOutNormal[] = "resource://vd_popout_normal.png";
const char kVDButtonPopOutOver[] = "resource://vd_popout_over.png";
const char kVDButtonPopOutDown[] = "resource://vd_popout_down.png";
const char kVDButtonPopInNormal[] = "resource://vd_popin_normal.png";
const char kVDButtonPopInOver[] = "resource://vd_popin_over.png";
const char kVDButtonPopInDown[] = "resource://vd_popin_down.png";

const char kVDMainBackground[] = "resource://vd_main_background.png";
const char kVDMainBackgroundMinimized[] =
    "resource://vd_main_background_minimized.png";
const char kVDMainBackgroundTransparent[] =
    "resource://vd_main_background_transparent.png";
const char kVDMainDockedBorderH[] = "resource://vd_main_docked_border_h.png";
const char kVDMainDockedBorderV[] = "resource://vd_main_docked_border_v.png";

const char kVDDetailsButtonBkgndClick[] =
    "resource://vd_details_button_bkgnd_click.png";
const char kVDDetailsButtonBkgndNormal[] =
    "resource://vd_details_button_bkgnd_normal.png";
const char kVDDetailsButtonBkgndOver[] =
    "resource://vd_details_button_bkgnd_over.png";
const char kVDDetailsButtonNegfbNormal[] =
    "resource://vd_details_button_negfb_normal.png";
const char kVDDetailsButtonNegfbOver[] =
    "resource://vd_details_button_negfb_over.png";

const char kVDFramedBackground[] = "resource://vd_framed_background.png";
const char kVDFramedBottom[] = "resource://vd_framed_bottom.png";
const char kVDFramedMiddle[] = "resource://vd_framed_middle.png";
const char kVDFramedTop[] = "resource://vd_framed_top.png";
const char kVDFramedCloseDown[] = "resource://vd_framed_close_down.png";
const char kVDFramedCloseNormal[] = "resource://vd_framed_close_normal.png";
const char kVDFramedCloseOver[] = "resource://vd_framed_close_over.png";

const char kVDBottomRightCorner[] = "resource://vd_bottom_right_corner.png";

const char kSideBarGoogleIcon[] = "resource://sidebar_google.png";
const char kSBButtonAddDown[] = "resource://sidebar_add_down.png";
const char kSBButtonAddUp[] = "resource://sidebar_add_up.png";
const char kSBButtonAddOver[] = "resource://sidebar_add_over.png";
const char kSBButtonMenuDown[] = "resource://sidebar_menu_down.png";
const char kSBButtonMenuUp[] = "resource://sidebar_menu_up.png";
const char kSBButtonMenuOver[] = "resource://sidebar_menu_over.png";
const char kSBButtonMinimizeDown[] = "resource://sidebar_minimize_down.png";
const char kSBButtonMinimizeUp[] = "resource://sidebar_minimize_up.png";
const char kSBButtonMinimizeOver[] = "resource://sidebar_minimize_over.png";

const char kCommonJS[] = "resource://common.js";
const char kTextDetailsView[] = "resource://text_details_view.xml";
const char kHTMLDetailsView[] = "resource://html_details_view.xml";
const char kGGLAboutView[] = "resource://ggl_about.xml";
const char kGadgetAboutView[] = "resource://gadget_about.xml";

const char kGadgetsIcon[] = "resource://google-gadgets.png";

const char kMenuCheckedMarkIcon[] = "resource://menu_checked_mark.png";

const char kFtpUrlPrefix[] = "ftp://";
const char kHttpUrlPrefix[] = "http://";
const char kHttpsUrlPrefix[] = "https://";
const char kFeedUrlPrefix[] = "feed://";
const char kFileUrlPrefix[] = "file://";
const char kMailtoUrlPrefix[] = "mailto:";

/** The fallback encoding used to parse text files. */
const char kEncodingFallback[] = "ISO8859-1";

/**
 * The option key in the global options file, indicates the configuration
 * about debug console. It's value is an integer:
 *   - 0: no debug console;
 *   - 1: allow debug console (a "Show debug console" menu item will be added
 *        to each gadget)
 *   - 2: open debug console when a gadget is added, useful to debug gadget
 *        startup code.
 */
const char kDebugConsoleOption[] = "debug_console";

/**
 * The option key to store permissions of a gadget instance in its options file.
 */
const char kPermissionsOption[] = "permissions";

const char kDefaultFontName[] = "sans-serif";
const int kDefaultFontSize = 8;

const char kGoogleGadgetsMimeType[] = "application/x-google-gadget";

/**
 * The upper limit of size of single file if the file is read as a whole.
 */
const size_t kMaxFileSize = 20 * 1024 * 1024;

} // namespace ggadget

#endif // GGADGET_GADGET_CONSTS_H__
