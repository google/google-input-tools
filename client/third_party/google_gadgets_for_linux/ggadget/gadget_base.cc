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

#include "file_manager_factory.h"
#include "file_manager_interface.h"
#include "gadget_base.h"
#include "gadget_consts.h"
#include "host_interface.h"
#include "localized_file_manager.h"
#include "logger.h"
#include "main_loop_interface.h"
#include "permissions.h"
#include "script_context_interface.h"
#include "small_object.h"
#include "system_utils.h"
#include "xml_http_request_interface.h"
#include "xml_parser_interface.h"

namespace {

// Maximum allowed idle time (in milliseconds) after user interaction.
// IsInUserInteraction() returns true within this idle time.
static const uint64_t kMaxAllowedUserInteractionIdleTime = 10000;

}  // namespace

namespace ggadget {

class GadgetBase::Impl : public SmallObject<> {
 public:
  Impl(HostInterface *host, int instance_id)
      : host_(host),
        instance_id_(instance_id),
        xml_http_request_session_(0),
        last_user_interaction_time_(0),
        in_user_interaction_(false) {
  }

  ~Impl() {
    if (xml_http_request_session_) {
      XMLHttpRequestFactoryInterface *factory = GetXMLHttpRequestFactory();
      ASSERT(factory);
      factory->DestroySession(xml_http_request_session_);
      xml_http_request_session_ = 0;
    }
  }

  int GetXMLHttpRequestSession() {
    if (!xml_http_request_session_) {
      XMLHttpRequestFactoryInterface *factory = GetXMLHttpRequestFactory();
      if (factory)
        xml_http_request_session_ = factory->CreateSession();
    }
    return xml_http_request_session_;
  }

  HostInterface *host_;
  int instance_id_;
  int xml_http_request_session_;
  Signal2<void, LogLevel, const std::string &> log_signal_;
  uint64_t last_user_interaction_time_;
  bool in_user_interaction_;
};

GadgetBase::GadgetBase(HostInterface *host, int instance_id)
    : impl_(new Impl(host, instance_id)) {
  // Let the gadget become a log context, so that all logs related to this
  // gadget can be outputted to correct debug console.
  ConnectContextLogListener(
      this, NewSlot(this, &GadgetBase::OnContextLog,
                    static_cast<ScriptContextInterface *>(NULL)));
}

GadgetBase::~GadgetBase() {
  RemoveLogContext(this);
}

HostInterface *GadgetBase::GetHost() const {
  return impl_->host_;
}

int GadgetBase::GetInstanceID() const {
  return impl_->instance_id_;
}

XMLHttpRequestInterface *GadgetBase::CreateXMLHttpRequest() {
  XMLHttpRequestFactoryInterface *factory = GetXMLHttpRequestFactory();
  if (!factory) {
    LOG("No XMLHttpRequestFactory.");
    return NULL;
  }

  const Permissions *permissions = GetPermissions();
  if (permissions && permissions->IsRequiredAndGranted(Permissions::NETWORK)) {
    return factory->CreateXMLHttpRequest(impl_->GetXMLHttpRequestSession(),
                                         GetXMLParser());
  }

  LOG("No permission to access network.");
  return NULL;
}

bool GadgetBase::SetInUserInteraction(bool in_user_interaction) {
  const bool old_value = impl_->in_user_interaction_;
  impl_->in_user_interaction_ = in_user_interaction;
  // Record time when ending user interaction.
  if (old_value && !in_user_interaction)
    impl_->last_user_interaction_time_ = GetGlobalMainLoop()->GetCurrentTime();
  return old_value;
}

bool GadgetBase::IsInUserInteraction() const {
  const uint64_t idle = GetGlobalMainLoop()->GetCurrentTime() -
      impl_->last_user_interaction_time_;
  return impl_->in_user_interaction_ ||
      idle <= kMaxAllowedUserInteractionIdleTime;
}

bool GadgetBase::OpenURL(const char *url) const {
  if (!impl_->host_)
    return false;

  const Permissions *permissions = GetPermissions();
  if (!permissions ||
      (!permissions->IsRequiredAndGranted(Permissions::NETWORK) &&
       !permissions->IsRequiredAndGranted(Permissions::ALL_ACCESS))) {
    LOG("No permission to open url.");
    return false;
  }

  if (IsInUserInteraction())
    return impl_->host_->OpenURL(this, url);

  LOG("OpenURL() can only be called during user interaction.");
  return false;
}

int GadgetBase::GetDefaultFontSize() const {
  return impl_->host_ ? impl_->host_->GetDefaultFontSize() : 0;
}

Connection *GadgetBase::ConnectLogListener(
      Slot2<void, LogLevel, const std::string &> *listener) {
  return impl_->log_signal_.Connect(listener);
}

std::string GadgetBase::OnContextLog(LogLevel level,
                                     const char *filename,
                                     int line,
                                     const std::string &message,
                                     ScriptContextInterface *context) {
  GGL_UNUSED(filename);
  GGL_UNUSED(line);
  std::string real_message;
  std::string script_filename;
  int script_line = 0;
  if (context)
    context->GetCurrentFileAndLine(&script_filename, &script_line);
  if (script_filename.empty() ||
      strncmp(script_filename.c_str(), message.c_str(),
              script_filename.size()) == 0) {
    real_message = message;
  } else {
    StringAppendPrintf(&real_message, "%s:%d: %s",
                       script_filename.c_str(), script_line, message.c_str());
  }
  impl_->log_signal_(level, real_message);
  return real_message;
}

// static
FileManagerInterface *GadgetBase::CreateFileManager(
    const char *manifest_filename, const char *base_path, const char *locale) {
  ASSERT(manifest_filename);
  ASSERT(base_path);

  std::string path, filename;
  SplitFilePath(base_path, &path, &filename);

  // Uses the parent path of base_path if it refers to a manifest file.
  if (filename != manifest_filename)
    path = base_path;

  FileManagerInterface *fm = ::ggadget::CreateFileManager(path.c_str());
  return fm ? new LocalizedFileManager(fm, locale) : NULL;
}

// static
bool GadgetBase::ExtractFileFromFileManager(FileManagerInterface *fm,
                                            const char *file,
                                            std::string *path) {
  path->clear();
  return fm->ExtractFile(file, path);
}

// static
bool GadgetBase::ReadStringsAndManifest(FileManagerInterface *file_manager,
                                        const char *manifest_filename,
                                        const char *manifest_tag,
                                        StringMap *strings_map,
                                        StringMap *manifest_info_map) {
  ASSERT(file_manager);
  ASSERT(manifest_filename);
  ASSERT(manifest_tag);
  ASSERT(strings_map);
  ASSERT(manifest_info_map);

  // Load string table.
  std::string strings_data;
  if (file_manager->ReadFile(kStringsXML, &strings_data)) {
    std::string full_path = file_manager->GetFullPath(kStringsXML);
    if (!GetXMLParser()->ParseXMLIntoXPathMap(strings_data, NULL,
                                              full_path.c_str(),
                                              kStringsTag,
                                              NULL, kEncodingFallback,
                                              strings_map)) {
      return false;
    }
  }

  std::string manifest_contents;
  if (!file_manager->ReadFile(manifest_filename, &manifest_contents))
    return false;

  std::string manifest_path = file_manager->GetFullPath(manifest_filename);
  if (!GetXMLParser()->ParseXMLIntoXPathMap(manifest_contents,
                                            strings_map,
                                            manifest_path.c_str(),
                                            manifest_tag,
                                            NULL, kEncodingFallback,
                                            manifest_info_map)) {
    return false;
  }

  for (StringMap::iterator it = strings_map->begin();
       it != strings_map->end(); ++it) {
    // Trimming is required for compatibility.
    it->second = TrimString(it->second);
  }
  return true;
}

// static
bool GadgetBase::GetManifestForLocale(const char *manifest_filename,
                                      const char *manifest_tag,
                                      const char *base_path,
                                      const char *locale,
                                      StringMap *data) {
  ASSERT(manifest_filename);
  ASSERT(manifest_tag);
  ASSERT(base_path);
  ASSERT(data);

  scoped_ptr<FileManagerInterface> file_manager(
      CreateFileManager(manifest_filename, base_path, locale));
  if (!file_manager.get())
    return false;

  StringMap strings_map;
  return ReadStringsAndManifest(
      file_manager.get(), manifest_filename, manifest_tag, &strings_map, data);
}

}  // namespace ggadget
