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

#include <locale.h>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>

#include <ggadget/dir_file_manager.h>
#include <ggadget/extension_manager.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/file_manager_wrapper.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/localized_file_manager.h>
#include <ggadget/logger.h>
#include <ggadget/messages.h>
#include <ggadget/string_utils.h>
#include <ggadget/xml_parser_interface.h>
#include <ggadget/slot.h>

static const char *kGlobalExtensions[] = {
  "libxml2-xml-parser",
  NULL
};

static const char *kGlobalResourcePaths[] = {
  "resources.gg",
  "../resources/resources.gg",
  "../../resources/resources.gg",
  NULL
};

class IntlDesktopEntryFunctor {
 public:
  IntlDesktopEntryFunctor(const std::string &entry,
                          std::vector<std::string> *result_content)
    : result_content_(result_content) {
    ggadget::SplitString(entry, "=", &key_, &msg_id_);
    if (key_[0] == '_')
      key_.erase(0, 1);
  }

  bool operator()(const char *lang) const {
    std::string localized_msg =
        ggadget::Messages::get()->GetMessageForLocale(msg_id_.c_str(), lang);
    if (key_.length() && localized_msg.length()) {
      // Use English message as the default one.
      if (strcmp(lang, "en") == 0) {
        result_content_->push_back(
            ggadget::StringPrintf("%s=%s", key_.c_str(),
                                  localized_msg.c_str()));
      }
      std::string lang_str(lang);
      // Use '_' as conjunction character to conform freedesktop specificaton.
      for (std::string::iterator it = lang_str.begin();
           it != lang_str.end(); ++it)
        if (*it == '-') *it = '_';
      result_content_->push_back(
          ggadget::StringPrintf("%s[%s]=%s", key_.c_str(), lang_str.c_str(),
                                localized_msg.c_str()));
    }
    return true;
  }

  bool operator==(const IntlDesktopEntryFunctor &) const {
    return false;
  }

 private:
  std::string key_;
  std::string msg_id_;
  std::vector<std::string> *result_content_;
};

int IntlDesktopFile(const char *input, const char *output) {
  FILE *input_fp = fopen(input, "r");
  if (!input_fp) {
    printf("Can't open input file.\n");
    return 1;
  }

  FILE *output_fp = fopen(output, "w");
  if (!output_fp) {
    printf("Can't open output file.\n");
    fclose(input_fp);
    return 1;
  }

  std::vector<std::string> original_content;

  // Read original content
  char buf[1024];
  while(!feof(input_fp) && !ferror(input_fp)) {
    if (fgets(buf, 1024, input_fp)) {
      original_content.push_back(ggadget::TrimString(buf));
    }
  }
  fclose(input_fp);

  // Internationlize original content
  std::vector<std::string> result_content;
  std::vector<std::string>::iterator it = original_content.begin();
  for (; it != original_content.end(); ++it) {
    if ((*it)[0] == '_') {
      IntlDesktopEntryFunctor functor(*it, &result_content);
      ggadget::Messages::get()->EnumerateSupportedLocales(
          ggadget::NewFunctorSlot<bool, const char *>(functor));
    } else {
      result_content.push_back(*it);
    }
  }

  // Output
  it = result_content.begin();
  for (; it != result_content.end(); ++it) {
    fprintf(output_fp, "%s\n", it->c_str());
  }

  fclose(output_fp);
  return 0;
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");

  ggadget::FileManagerWrapper *fm_wrapper = new ggadget::FileManagerWrapper();
  ggadget::FileManagerInterface *fm;

  for (size_t i = 0; kGlobalResourcePaths[i]; ++i) {
    fm = ggadget::CreateFileManager(kGlobalResourcePaths[i]);
    if (fm) {
      fm_wrapper->RegisterFileManager(ggadget::kGlobalResourcePrefix, fm);
      break;
    }
  }
  ggadget::SetGlobalFileManager(fm_wrapper);

  // Load global extensions.
  ggadget::ExtensionManager *ext_manager =
      ggadget::ExtensionManager::CreateExtensionManager();
  ggadget::ExtensionManager::SetGlobalExtensionManager(ext_manager);

  // Ignore errors when loading extensions.
  for (size_t i = 0; kGlobalExtensions[i]; ++i)
    ext_manager->LoadExtension(kGlobalExtensions[i], false);
  ext_manager->SetReadonly();

  if (argc != 3) {
    printf("Wrong arguments. Usage:\n"
           "intl-desktop-file input_file output_file\n");
    return 1;
  }

  return IntlDesktopFile(argv[1], argv[2]);
}
