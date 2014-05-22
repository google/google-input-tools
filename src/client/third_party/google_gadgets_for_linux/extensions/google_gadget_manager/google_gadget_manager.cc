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

#include "google_gadget_manager.h"

#include <ctime>
#include <cstdlib>
#include <ggadget/digest_utils.h>
#include <ggadget/file_manager_factory.h>
#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/host_interface.h>
#include <ggadget/locales.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/menu_interface.h>
#include <ggadget/options_interface.h>
#include <ggadget/permissions.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_map.h>
#include <ggadget/system_utils.h>
#include <ggadget/usage_collector_interface.h>
#include <ggadget/view.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_parser_interface.h>

namespace ggadget {
namespace google {

// Daily pings are sent every 25~28 hours.
static const int kDailyPingIntervalBase = 25 * 3600 * 1000;
static const int kWeeklyPingIntervalBase = kDailyPingIntervalBase * 7;

static const char kGadgetFeedbackURLIGoogle[] =
    "http://www.google.com/ig/directory?root=/ig&url=%s&hl=%s";

static const char kGadgetFeedbackURLDesktop[] =
    "http://desktop.google.com/plugins/i/%s.html&hl=%s";

// Note: The backoff and randomization features in this implementation is
// very important for proper server-side operation. Please do *NOT* disable
// or remove them.

// Convert a string to a valid and safe file name. Need not be inversable.
static std::string MakeGoodFileName(const char *uuid_or_url) {
  std::string result(uuid_or_url);
  for (size_t i = 0; i < result.size(); i++) {
    char c = result[i];
    if (!isalnum(c) && c != '-' && c != '_' && c != '.' && c != '+')
      result[i] = '_';
  }
  return result;
}

// Return a value which is the input value +/- random(20%).
static int Randomize(int input) {
  int variant = input * 20 / 100;
  return input + (rand() % (variant * 2)) - variant;
}

// TODO: Add blacklisting of gadgets and whitelisting of gadget features.
GoogleGadgetManager::GoogleGadgetManager()
    : main_loop_(GetGlobalMainLoop()),
      global_options_(GetGlobalOptions()),
      file_manager_(GetGlobalFileManager()),
      last_update_time_(0), last_try_time_(0), retry_timeout_(0),
      update_timer_(0), free_metadata_timer_(0), daily_ping_timer_(0),
      full_download_(false),
      updating_metadata_(false),
      browser_gadget_(NULL),
      first_run_(false),
      collector_(NULL) {
  ASSERT(main_loop_);
  ASSERT(global_options_);
  ASSERT(file_manager_);
}

GoogleGadgetManager::~GoogleGadgetManager() {
  if (update_timer_) {
    main_loop_->RemoveWatch(update_timer_);
    update_timer_ = 0;
  }

  if (free_metadata_timer_) {
    main_loop_->RemoveWatch(free_metadata_timer_);
    free_metadata_timer_ = 0;
  }

  if (daily_ping_timer_) {
    main_loop_->RemoveWatch(daily_ping_timer_);
    daily_ping_timer_ = 0;
  }

  if (browser_gadget_) {
    delete browser_gadget_;
    browser_gadget_ = NULL;
  }
}

void GoogleGadgetManager::Init() {
  free_metadata_timer_ = main_loop_->AddTimeoutWatch(kFreeMetadataInterval,
      new WatchCallbackSlot(
          NewSlot(this, &GoogleGadgetManager::OnFreeMetadataTimer)));

  int current_max_id = -1;
  global_options_->GetValue(kMaxInstanceIdOption).ConvertToInt(&current_max_id);
  if (current_max_id >= kMaxNumGadgetInstances)
    current_max_id = kMaxNumGadgetInstances - 1;

  instance_statuses_.resize(current_max_id + 1);
  for (int i = 0; i <= current_max_id; i++) {
    std::string key(kInstanceStatusOptionPrefix);
    key += StringPrintf("%d", i);
    int status = kInstanceStatusNone;
    global_options_->GetValue(key.c_str()).ConvertToInt(&status);
    std::string gadget_id;
    if (status != kInstanceStatusNone)
      gadget_id = GetInstanceGadgetId(i);
    instance_statuses_[i] = status;
    if (status == kInstanceStatusActive)
      active_gadgets_.insert(gadget_id);
  }
  TrimInstanceStatuses();

  int run_count = 0;
  global_options_->GetValue(kRunCountOption).ConvertToInt(&run_count);
  // Also test if instances_statuses_ is empty to make old versions happy.
  first_run_ = (run_count == 0 && instance_statuses_.empty());
  global_options_->PutValue(kRunCountOption, Variant(run_count + 1));

  UsageCollectorFactoryInterface *uc_factory = GetUsageCollectorFactory();
  if (uc_factory)
    collector_ = uc_factory->GetPlatformUsageCollector();
  if (collector_) {
    int64_t last_daily_ping_time = 0;
    global_options_->GetValue(kLastDailyPingTimeOption)
        .ConvertToInt64(&last_daily_ping_time);
    int64_t current_time = static_cast<int64_t>(main_loop_->GetCurrentTime());
    int64_t interval = current_time - last_daily_ping_time;
    if (interval <= 0) {
      // In case of options data error or timer error.
      ScheduleDailyPing();
    } else {
      int timeout = interval > kDailyPingIntervalBase ? 1000 + rand() % 100000 :
                    kDailyPingIntervalBase - static_cast<int>(interval);
      daily_ping_timer_ = main_loop_->AddTimeoutWatch(
          timeout,
          new WatchCallbackSlot(
              NewSlot(this, &GoogleGadgetManager::OnFirstDailyPing)));
    }
  }

  if (first_run_) {
    // Add some default built-in gadgets.
    NewGadgetInstance("analog-clock");
    NewGadgetInstance("rss");
    // Schedule an immediate update if it is first run.
    ScheduleUpdate(0);
    if (collector_)
      collector_->ReportFirstUse();
  } else {
    ScheduleNextUpdate();
  }
}

bool GoogleGadgetManager::OnFreeMetadataTimer(int timer) {
  GGL_UNUSED(timer);
  if (!browser_gadget_) {
    // Only free metadata when the gadget browser is not active.
    metadata_.FreeMemory();
  }
  return true;
}

void GoogleGadgetManager::ScheduleNextUpdate() {
  if (last_try_time_ == 0) {
    global_options_->GetValue(kLastTryTimeOption).
        ConvertToInt64(&last_try_time_);
  }
  if (last_try_time_ > 0) {
    // Schedule a retry update because the last update failed.
    if (retry_timeout_ == 0) {
      global_options_->GetValue(kRetryTimeoutOption).
          ConvertToInt(&retry_timeout_);
    }
    if (retry_timeout_ <= 0 ||
        retry_timeout_ > kGadgetsMetadataRetryMaxInterval) {
      retry_timeout_ = Randomize(kGadgetsMetadataRetryInterval);
    }
    ScheduleUpdate(last_try_time_ + retry_timeout_);
  } else {
    // Schedule a normal update.
    if (last_update_time_ == 0) {
      global_options_->GetValue(kLastUpdateTimeOption).
          ConvertToInt64(&last_update_time_);
    }
    ScheduleUpdate(last_update_time_ + kGadgetsMetadataUpdateInterval);
  }
}

void GoogleGadgetManager::ScheduleUpdate(int64_t time) {
  if (update_timer_) {
    main_loop_->RemoveWatch(update_timer_);
    update_timer_ = 0;
  }

  int64_t current_time = static_cast<int64_t>(main_loop_->GetCurrentTime());
  int time_diff = static_cast<int>(std::max(INT64_C(0), time - current_time));
  update_timer_ = main_loop_->AddTimeoutWatch(time_diff,
      new WatchCallbackSlot(
          NewSlot(this, &GoogleGadgetManager::OnUpdateTimer)));
}

bool GoogleGadgetManager::OnUpdateTimer(int timer) {
  GGL_UNUSED(timer);
  UpdateGadgetsMetadata(false);
  return false;
}

bool GoogleGadgetManager::UpdateGadgetsMetadata(bool full_download) {
  if (updating_metadata_)
    return false;
  updating_metadata_ = true;
  full_download_ = full_download;
  last_try_time_ = static_cast<int64_t>(main_loop_->GetCurrentTime());
  global_options_->PutValue(kLastTryTimeOption, Variant(last_try_time_));
  metadata_.UpdateFromServer(
      full_download,
      GetXMLHttpRequestFactory()->CreateXMLHttpRequest(0, GetXMLParser()),
      NewSlot(this, &GoogleGadgetManager::OnUpdateDone));
  return true;
}

void GoogleGadgetManager::OnUpdateDone(bool request_success,
                                       bool parsing_success) {
  updating_metadata_ = false;
  if (request_success) {
    if (parsing_success) {
      LOGI("Successfully updated gadget metadata");
      last_update_time_ = static_cast<int64_t>(main_loop_->GetCurrentTime());
      last_try_time_ = -1;
      retry_timeout_ = 0;
      global_options_->PutValue(kLastTryTimeOption, Variant(last_try_time_));
      global_options_->PutValue(kRetryTimeoutOption,
                                Variant(retry_timeout_));
      global_options_->PutValue(kLastUpdateTimeOption,
                                Variant(last_update_time_));
      ScheduleNextUpdate();
      return;
    }

    LOG("Succeeded to request gadget metadata update, "
        "but failed to parse the result");
    if (!full_download_) {
      // The failed partial update may be because of corrupted cached file,
      // so immediately do a full download now.
      UpdateGadgetsMetadata(true);
      return;
    }
  }

  if (retry_timeout_ == 0) {
    retry_timeout_ = kGadgetsMetadataRetryInterval;
  } else {
    retry_timeout_ = Randomize(std::min(retry_timeout_ * 2,
                                        kGadgetsMetadataRetryMaxInterval));
  }
  global_options_->PutValue(kRetryTimeoutOption, Variant(retry_timeout_));
  LOG("Failed to update gadget metadata. Will retry after %dms",
      retry_timeout_);
  ScheduleNextUpdate();
}

std::string GoogleGadgetManager::GetInstanceGadgetId(int instance_id) {
  std::string key(kInstanceGadgetIdOptionPrefix);
  key += StringPrintf("%d", instance_id);
  std::string result;
  global_options_->GetValue(key.c_str()).ConvertToString(&result);
  return result;
}

void GoogleGadgetManager::SaveInstanceGadgetId(int instance_id,
                                               const char *gadget_id) {
  std::string key(kInstanceGadgetIdOptionPrefix);
  key += StringPrintf("%d", instance_id);
  if (gadget_id && *gadget_id)
    global_options_->PutValue(key.c_str(), Variant(gadget_id));
  else
    global_options_->Remove(key.c_str());
}

void GoogleGadgetManager::SetInstanceStatus(int instance_id, int status) {
  instance_statuses_[instance_id] = status;
  std::string key(kInstanceStatusOptionPrefix);
  key += StringPrintf("%d", instance_id);
  if (status == kInstanceStatusNone)
    global_options_->Remove(key.c_str());
  else
    global_options_->PutValue(key.c_str(), Variant(status));
}

// Trims the instance statuses array by removing trailing empty slots.
void GoogleGadgetManager::TrimInstanceStatuses() {
  int size = static_cast<int>(instance_statuses_.size());
  for (int i = size - 1; i >= 0; i--) {
    if (instance_statuses_[i] != kInstanceStatusNone) {
      if (i < size - 1) {
        instance_statuses_.resize(i + 1);
        global_options_->PutValue(kMaxInstanceIdOption, Variant(i + 1));
      }
      break;
    }
  }
}

void GoogleGadgetManager::ActuallyRemoveInstance(int instance_id,
                                                 bool remove_downloaded_file) {
  SetInstanceStatus(instance_id, kInstanceStatusNone);
  // Remove the options file for this instance.
  OptionsInterface *instance_options =
      CreateOptions(GetGadgetInstanceOptionsName(instance_id).c_str());
  instance_options->DeleteStorage();
  delete instance_options;

  if (remove_downloaded_file) {
    // Don't use GadgetInfo::source to judge if this gadget is a downloaded
    // sidebar gadget, because the gadget may be outdated so no longer exists
    // in plugins.xml.
    std::string gadget_id = GetInstanceGadgetId(instance_id);
    if (!gadget_id.empty() &&
        !GadgetIdIsFileLocation(gadget_id.c_str()) &&
        !GadgetIdIsSystemName(gadget_id.c_str())) {
      // Try best to remove the gadget file, ignoring any errors.
      std::string downloaded_file =
          GetDownloadedGadgetLocation(gadget_id.c_str());
      file_manager_->RemoveFile(downloaded_file.c_str());
    }
  }
  SaveInstanceGadgetId(instance_id, NULL);
}

void GoogleGadgetManager::IncreseAndCheckExpirationScores() {
  int size = static_cast<int>(instance_statuses_.size());
  for (int i = 0; i < size; i++) {
    if (instance_statuses_[i] >= kInstanceStatusInactiveStart) {
      // This is an inactive instance.
      if (instance_statuses_[i] >= kExpirationThreshold - 1) {
        // The expriation score reaches the threshold, actually remove
        // the instance.
        ActuallyRemoveInstance(i, true);
        std::string gadget_id = GetInstanceGadgetId(i);
        global_options_->Remove((kGadgetAddedTimeOptionPrefix +
            gadget_id).c_str());
        global_options_->Remove((kGadgetModuleIdOptionPrefix +
            gadget_id).c_str());
      } else {
        SetInstanceStatus(i, instance_statuses_[i] + 1);
      }
    }
  }
}

// Gets a lowest available instance id for a new instance.
int GoogleGadgetManager::GetNewInstanceId() {
  int size = static_cast<int>(instance_statuses_.size());
  for (int i = 0; i < size; i++) {
    if (instance_statuses_[i] == kInstanceStatusNone)
      return i;
  }

  if (size < kMaxNumGadgetInstances) {
    instance_statuses_.resize(size + 1);
    global_options_->PutValue(kMaxInstanceIdOption, Variant(size));
    return size;
  }

  LOG("Too many gadget instances");
  return -1;
}

bool GoogleGadgetManager::GadgetIdIsFileLocation(const char *gadget_id) {
  return file_manager_->FileExists(gadget_id, NULL);
}

bool GoogleGadgetManager::GadgetIdIsSystemName(const char *gadget_id) {
  return !GetSystemGadgetPath(gadget_id).empty();
}

bool GoogleGadgetManager::InitInstanceOptions(const char *gadget_id,
                                              int instance_id) {
  std::string options_name = GetGadgetInstanceOptionsName(instance_id);
  OptionsInterface *instance_options = CreateOptions(options_name.c_str());

  Variant org_gadget_id =
      instance_options->GetInternalValue(kInstanceGadgetIdOption);

  // Options won't be reused for blank igoogle gadgets and rss gadgets.
  if (strcmp(gadget_id, kIGoogleGadgetName) != 0 &&
      strcmp(gadget_id, kRSSGadgetName) != 0) {
    if (org_gadget_id == Variant(gadget_id)) {
      // The existing options can be reused.
      delete instance_options;
      return true;
    }
  }

  if (org_gadget_id.type() != Variant::TYPE_VOID) {
    // This options file is not belonged to this gadget, delete it.
    instance_options->DeleteStorage();
    delete instance_options;
    // Recreate the options instance.
    instance_options = CreateOptions(options_name.c_str());
  }

  instance_options->PutInternalValue(kInstanceGadgetIdOption,
                                     Variant(gadget_id));
  const GadgetInfo *info = GetGadgetInfo(gadget_id);
  if (info && info->source == SOURCE_PLUGINS_XML) {
    StringMap::const_iterator module_id =
        info->attributes.find(kModuleIDAttrib);
    if (module_id != info->attributes.end()) {
      // Use putValue instead of putDefaultValue in the code below since the
      // gadget may set its own default. Gadget can check if it has been
      // initialized already by checking exists().
      if (module_id->second == kIGoogleModuleID &&
          GetSystemGadgetPath(kIGoogleGadgetName).length()) {
        // Seed URL
        std::string json_url = "\"";
        json_url += gadget_id;
        json_url += "\"";
        Variant url = Variant(JSONString(json_url)); // raw objects
        instance_options->PutValue(kIGoogleURLOption, url);
      } else if (module_id->second == kRSSModuleID &&
                 GetSystemGadgetPath(kRSSGadgetName).length()) {
        // Seed options with URL.
        std::string json_url = "\"";
        json_url += gadget_id;
        json_url += "\"";
        Variant url = Variant(JSONString(json_url)); // raw objects
        instance_options->PutValue(kRSSURLOption, url);
      } else {
        instance_options->DeleteStorage();
        delete instance_options;
        return false;
      }
    }
  }
  instance_options->Flush();
  delete instance_options;
  return true;
}

int GoogleGadgetManager::NewGadgetInstance(const char *gadget_id) {
  DLOG("Adding gadget %s", gadget_id);
  if (!gadget_id || !*gadget_id)
    return -1;

  global_options_->PutValue(
      (std::string(kGadgetAddedTimeOptionPrefix) + gadget_id).c_str(),
      Variant(main_loop_->GetCurrentTime()));

  // First try to find the inactive instance of of this gadget.
  int size = static_cast<int>(instance_statuses_.size());
  for (int i = 0; i < size; i++) {
    if (instance_statuses_[i] >= kInstanceStatusInactiveStart &&
        GetInstanceGadgetId(i) == gadget_id) {
      SetInstanceStatus(i, kInstanceStatusActive);
      active_gadgets_.insert(gadget_id);
      if (!InitInstanceOptions(gadget_id, i))
        return -1;
      if (!new_instance_signal_.HasActiveConnections() ||
          new_instance_signal_(i)) {
        SendGadgetUsagePing(1, gadget_id);
        return i;
      } else {
        RemoveGadgetInstanceInternal(i, false);
        return -1;
      }
    }
  }

  // Add a pure new instance.
  int instance_id = GetNewInstanceId();
  if (instance_id < 0) {
    // TODO: Show error message.
    return instance_id;
  }

  if (!InitInstanceOptions(gadget_id, instance_id))
    return -1;

  SetInstanceStatus(instance_id, kInstanceStatusActive);
  SaveInstanceGadgetId(instance_id, gadget_id);
  active_gadgets_.insert(gadget_id);
  if (!new_instance_signal_.HasActiveConnections() ||
      new_instance_signal_(instance_id)) {
    SendGadgetUsagePing(1, gadget_id);
    return instance_id;
  }

  RemoveGadgetInstanceInternal(instance_id, false);
  TrimInstanceStatuses();
  return -1;
}

int GoogleGadgetManager::NewGadgetInstanceFromFile(const char *file) {
  return NewGadgetInstance(file);
}

bool GoogleGadgetManager::RemoveGadgetInstanceInternal(int instance_id,
                                                       bool send_ping) {
  if (instance_id == kGoogleGadgetBrowserInstanceId && browser_gadget_) {
    delete browser_gadget_;
    browser_gadget_ = NULL;
    return true;
  }

  int size = static_cast<int>(instance_statuses_.size());
  if (instance_id < 0 || instance_id >= size ||
      instance_statuses_[instance_id] != kInstanceStatusActive)
    return false;

  // Check if this instance is the last instance of this gadget.
  bool is_last_instance = true;
  std::string gadget_id = GetInstanceGadgetId(instance_id);
  for (int i = 0; i < size; i++) {
    if (i != instance_id &&
        instance_statuses_[i] == kInstanceStatusActive &&
        GetInstanceGadgetId(i) == gadget_id) {
      is_last_instance = false;
      break;
    }
  }

  IncreseAndCheckExpirationScores();
  if (is_last_instance) {
    // Only change status to inactive for the last instance of a gadget.
    SetInstanceStatus(instance_id, kInstanceStatusInactiveStart);
    active_gadgets_.erase(gadget_id);
  } else {
    // Actually remove the instance.
    ActuallyRemoveInstance(instance_id, false);
  }
  TrimInstanceStatuses();

  if (send_ping)
    SendGadgetUsagePing(2, gadget_id.c_str());

  remove_instance_signal_(instance_id);
  return true;
}

bool GoogleGadgetManager::RemoveGadgetInstance(int instance_id) {
  return RemoveGadgetInstanceInternal(instance_id, true);
}

void GoogleGadgetManager::UpdateGadgetInstances(const char *gadget_id) {
  if (!gadget_id || !*gadget_id)
    return;

  int size = static_cast<int>(instance_statuses_.size());
  for (int i = 0; i < size; i++) {
    if (instance_statuses_[i] == kInstanceStatusActive &&
        GetInstanceGadgetId(i) == gadget_id) {
      if (update_instance_signal_.HasActiveConnections()) {
        update_instance_signal_(i);
      } else {
        remove_instance_signal_(i);
        new_instance_signal_(i);
      }
    }
  }
}

std::string GoogleGadgetManager::GetGadgetInstanceOptionsName(int instance_id) {
  return StringPrintf("gadget-%d", instance_id);
}

bool GoogleGadgetManager::EnumerateGadgetInstances(Slot1<bool, int> *callback) {
  ASSERT(callback);
  int size = static_cast<int>(instance_statuses_.size());
  for (int i = 0; i < size; i++) {
    if (instance_statuses_[i] == kInstanceStatusActive &&
        !(*callback)(i)) {
      delete callback;
      return false;
    }
  }
  delete callback;
  return true;
}

class AddedTimeUpdater {
 public:
  AddedTimeUpdater(GadgetInfoMap *map) : map_(map) { }
  bool Callback(const char *name, const Variant &value, bool encrypted) {
    GGL_UNUSED(encrypted);
    if (strncmp(name, kGadgetAddedTimeOptionPrefix,
                arraysize(kGadgetAddedTimeOptionPrefix) - 1) == 0) {
      std::string gadget_id(name);
      gadget_id.erase(0, arraysize(kGadgetAddedTimeOptionPrefix) - 1);
      GadgetInfoMap::iterator it = map_->find(gadget_id);
      if (it != map_->end()) {
        int64_t time = 0;
        value.ConvertToInt64(&time);
        it->second.accessed_date = static_cast<uint64_t>(time);
      } else {
        // The gadget doesn't exist, so remove the options item.
        options_to_remove_.push_back(name);
      }
    }
    return true;
  }
  GadgetInfoMap *map_;
  std::vector<std::string> options_to_remove_;
};

const GadgetInfoMap &GoogleGadgetManager::GetAllGadgetInfo() {
  GadgetInfoMap *map = metadata_.GetAllGadgetInfo();
  AddedTimeUpdater updater(map);
  global_options_->EnumerateItems(
      NewSlot(&updater, &AddedTimeUpdater::Callback));

  // Remove the options items for gadgets which no longer exist.
  for (std::vector<std::string>::const_iterator it =
           updater.options_to_remove_.begin();
       it != updater.options_to_remove_.end(); ++it) {
    global_options_->Remove(it->c_str());
  }
  return *map;
}

const GadgetInfo *GoogleGadgetManager::GetGadgetInfo(const char *gadget_id) {
  if (!gadget_id || !*gadget_id)
    return NULL;

  GadgetInfoMap *map = metadata_.GetAllGadgetInfo();
  GadgetInfoMap::const_iterator it = map->find(gadget_id);
  if (it != map->end())
    return &it->second;

  if (GadgetIdIsFileLocation(gadget_id)) {
    // Ensure metadata of the local gadget file is loaded.
    return metadata_.AddLocalGadgetInfo(
        file_manager_->GetFullPath(gadget_id).c_str());
  }
  return NULL;
}

const GadgetInfo *GoogleGadgetManager::GetGadgetInfoOfInstance(
    int instance_id) {
  std::string gadget_id = GetInstanceGadgetId(instance_id);
  return gadget_id.empty() ? NULL : GetGadgetInfo(gadget_id.c_str());
}

bool GoogleGadgetManager::GadgetHasInstance(const char *gadget_id) {
  if (!gadget_id || !*gadget_id)
    return false;

  return active_gadgets_.find(gadget_id) != active_gadgets_.end();
}

bool GoogleGadgetManager::NeedDownloadOrUpdateGadget(const char *gadget_id,
                                                     bool failure_result) {
  if (!gadget_id || !*gadget_id)
    return false;

  const GadgetInfo *gadget_info = GetGadgetInfo(gadget_id);
  if (!gadget_info) // This should not happen.
    return failure_result;
  if (gadget_info->source != SOURCE_PLUGINS_XML)
    return false;

  StringMap::const_iterator attr_it = gadget_info->attributes.find("type");
  if (attr_it != gadget_info->attributes.end() &&
      attr_it->second != "sidebar") {
    // We only download desktop gadgets.
    return false;
  }

  std::string path(GetDownloadedGadgetLocation(gadget_id));
  if (file_manager_->GetLastModifiedTime(path.c_str()) <
      gadget_info->updated_date)
    return true;

  std::string full_path = file_manager_->GetFullPath(path.c_str());
  if (full_path.empty()) // This should not happen.
    return failure_result;
  StringMap manifest;
  if (!Gadget::GetGadgetManifest(full_path.c_str(), &manifest))
    return failure_result;

  std::string local_version = manifest[kManifestVersion];
  attr_it = gadget_info->attributes.find("version");
  if (attr_it != gadget_info->attributes.end()) {
    std::string remote_version = attr_it->second;
    int compare_result;
    if (CompareVersion(local_version.c_str(), remote_version.c_str(),
                       &compare_result) &&
        compare_result < 0) {
      return true;
    }
  }
  return false;
}

std::string GoogleGadgetManager::GetDownloadedGadgetLocation(
    const char *gadget_id) {
  ASSERT(!GadgetIdIsFileLocation(gadget_id) &&
         !GadgetIdIsSystemName(gadget_id));
  std::string path(kDownloadedGadgetsDir);
  path += MakeGoodFileName(gadget_id);
  path += kGadgetFileSuffix;
  return path;
}

bool GoogleGadgetManager::GetGadgetDefaultPermissions(
    int instance_id, Permissions *permissions) {
  ASSERT(permissions);

  std::string path = GetGadgetInstancePath(instance_id);
  const GadgetInfo *info = GetGadgetInfoOfInstance(instance_id);
  StringMap manifest;
  if (path.length() && info &&
      Gadget::GetGadgetManifest(path.c_str(), &manifest)) {
    *permissions = Permissions();
    Gadget::GetGadgetRequiredPermissions(&manifest, permissions);

    if (info->source == SOURCE_BUILTIN) {
      // built-in gadgets are absolutely trusted.
      permissions->SetGranted(Permissions::ALL_ACCESS, true);
    } else if (info->source == SOURCE_PLUGINS_XML) {
      // FIXME:
      // Trusts gadgets created by Google for now, until they are
      // all upgraded to make use of <permissions/> API.
      StringMap::const_iterator it = info->attributes.find("category");
      if (it != info->attributes.end()) {
        std::string category = ',' + it->second + ',';
        if (category.find(",google,") != category.npos)
          permissions->GrantAllRequired();
      }
    }
    return true;
  }
  return false;
}

bool GoogleGadgetManager::GetGadgetInstanceInfo(
    int instance_id, const char *locale,
    std::string *author, std::string *download_url,
    std::string *title, std::string *description) {
  const GadgetInfo *info = GetGadgetInfoOfInstance(instance_id);
  if (info == NULL)
    return false;

  std::string locale_str = locale ? locale : GetSystemLocaleName();
  locale_str = ToLower(locale_str);
  StringMap::const_iterator it;
  if (author) {
    it = info->attributes.find("author");
    *author = it == info->attributes.end() ? std::string() : it->second;
  }
  if (download_url) {
    it = info->attributes.find("download_url");
    *download_url = it == info->attributes.end() ? std::string() : it->second;
  }

  if (title) {
    it = info->titles.find(locale_str);
    if (it == info->titles.end())
      it = info->titles.find("en");
    if (it == info->titles.end()) {
      it = info->attributes.find("name");
      *title = it == info->attributes.end() ? std::string() : it->second;
    } else {
      *title = it->second;
    }
  }
  if (description) {
    it = info->descriptions.find(ToLower(locale_str));
    if (it == info->descriptions.end())
      it = info->descriptions.find("en");
    if (it == info->descriptions.end()) {
      it = info->attributes.find("product_summary");
      *description = it == info->attributes.end() ? std::string() : it->second;
    } else {
      *description = it->second;
    }
  }
  return true;
}

std::string GoogleGadgetManager::GetGadgetInstanceFeedbackURL(int instance_id) {
  const GadgetInfo *info = GetGadgetInfoOfInstance(instance_id);

  if (!info)
    return std::string();

  // If the gadget is not from plugins.xml, try to find it in plugins.xml by
  // matching guid.
  if (info->source != SOURCE_PLUGINS_XML) {
    StringMap::const_iterator guid = info->attributes.find("guid");
    if (guid != info->attributes.end()) {
      const GadgetInfo *new_info = GetGadgetInfo(guid->second.c_str());
      if (new_info && new_info->source == SOURCE_PLUGINS_XML &&
          new_info->id == guid->second) {
        info = new_info;
      }
    }
  }

  // Only gadgets from plugins.xml have feedback url.
  if (info->source != SOURCE_PLUGINS_XML)
    return std::string();

  bool is_desktop = true;
  StringMap::const_iterator it;
  it = info->attributes.find(kModuleIDAttrib);
  if (it != info->attributes.end()) {
    // Only igoogle gadgets have feedback url. RSS feeds don't have.
    if (it->second == kIGoogleModuleID)
      is_desktop = false;
    else
      return std::string();
  }

  it = info->attributes.find(is_desktop ? "id" : "download_url");
  if (it == info->attributes.end())
    return std::string();

  return StringPrintf(
      is_desktop ? kGadgetFeedbackURLDesktop : kGadgetFeedbackURLIGoogle,
      it->second.c_str(), GetSystemLocaleName().c_str());
}

Connection *GoogleGadgetManager::ConnectOnNewGadgetInstance(
    Slot1<bool, int> *callback) {
  return new_instance_signal_.Connect(callback);
}

Connection *GoogleGadgetManager::ConnectOnRemoveGadgetInstance(
    Slot1<void, int> *callback) {
  return remove_instance_signal_.Connect(callback);
}

Connection *GoogleGadgetManager::ConnectOnUpdateGadgetInstance(
    Slot1<void, int> *callback) {
  return update_instance_signal_.Connect(callback);
}

void GoogleGadgetManager::SaveThumbnailToCache(const char *thumbnail_url,
                                               const std::string &data) {
  if (!thumbnail_url || !*thumbnail_url || data.empty())
    return;

  std::string path(kThumbnailCacheDir);
  path += MakeGoodFileName(thumbnail_url);
  file_manager_->WriteFile(path.c_str(), data, true);
}

uint64_t GoogleGadgetManager::GetThumbnailCachedTime(
    const char *thumbnail_url) {
  if (!thumbnail_url || !*thumbnail_url)
    return 0;

  std::string path(kThumbnailCacheDir);
  path += MakeGoodFileName(thumbnail_url);
  return file_manager_->GetLastModifiedTime(path.c_str());
}

std::string GoogleGadgetManager::LoadThumbnailFromCache(
    const char *thumbnail_url) {
  if (!thumbnail_url || !*thumbnail_url)
    return std::string();

  std::string path(kThumbnailCacheDir);
  path += MakeGoodFileName(thumbnail_url);
  std::string data;
  if (file_manager_->ReadFile(path.c_str(), &data))
    return data;
  return std::string();
}

// Define TEST_UPDATING to test gadget updating. All installed gadgets are
// treated updatable and the gadget will always be downloaded.
// #define TEST_UPDATING

bool GoogleGadgetManager::NeedDownloadGadget(const char *gadget_id) {
#ifdef TEST_UPDATING
  return true;
#else
  return NeedDownloadOrUpdateGadget(gadget_id, true);
#endif
}

bool GoogleGadgetManager::NeedUpdateGadget(const char *gadget_id) {
  return GadgetHasInstance(gadget_id)
#ifndef TEST_UPDATING
         && NeedDownloadOrUpdateGadget(gadget_id, false)
#endif
         ;
}

bool GoogleGadgetManager::SaveGadget(const char *gadget_id,
                                     const std::string &data) {
  const GadgetInfo *gadget_info = GetGadgetInfo(gadget_id);
  if (!gadget_info) // This should not happen.
    return false;

  StringMap::const_iterator attr_it = gadget_info->attributes.find("checksum");
  if (attr_it != gadget_info->attributes.end()) {
    std::string required_checksum;
    std::string actual_checksum;
    if (!WebSafeDecodeBase64(attr_it->second.c_str(), &required_checksum) ||
        !GenerateSHA1(data, &actual_checksum) ||
        actual_checksum != required_checksum) {
      LOG("Checksum mismatch for %s", gadget_id);
      // This checksum mismatch may be caused by an old version of plugins.xml,
      // So immediately update the metadata to ensure it is the latest.
      UpdateGadgetsMetadata(true);
      return false;
    }
    DLOG("Checksum OK %s", gadget_id);
  }

  std::string location(GetDownloadedGadgetLocation(gadget_id));
  if (!file_manager_->WriteFile(location.c_str(), data, true))
    return false;

  UpdateGadgetInstances(gadget_id);
  return true;
}

std::string GoogleGadgetManager::GetGadgetPath(const char *gadget_id) {
  std::string result = GetSystemGadgetPath(gadget_id);
  if (!result.empty()) {
    // This is a built-in gadget.
    return result;
  }

  if (GadgetIdIsFileLocation(gadget_id)) {
    // This is a gadget added from local gadget file/directory.
    return file_manager_->GetFullPath(gadget_id);
  }

  if (!HasValidURLPrefix(gadget_id)) {
    // This is a normal gadget published in plugins.xml and was downloaded.
    return file_manager_->GetFullPath(
        GetDownloadedGadgetLocation(gadget_id).c_str());
  }

  std::string module_id;
  std::string key(kGadgetModuleIdOptionPrefix);
  key += gadget_id;
  global_options_->GetValue(key.c_str()).ConvertToString(&module_id);
  if (module_id.empty()) {
      const GadgetInfo *info = GetGadgetInfo(gadget_id);
      if (!info) {
        // This gadget has no metadata, maybe because it was removed from
        // plugins.xml, or current plugins.xml is not complete. Still allow it
        // to be opened.
        return file_manager_->GetFullPath(
            GetDownloadedGadgetLocation(gadget_id).c_str());
      }

      result.clear();
      StringMap::const_iterator module_id_it = info->attributes.find(kModuleIDAttrib);
      if (module_id_it != info->attributes.end())
        module_id = module_id_it->second;
  }

  if (module_id == kRSSModuleID) {
    global_options_->PutValue(key.c_str(), Variant(module_id));
    return GetSystemGadgetPath(kRSSGadgetName);
  }
  if (module_id == kIGoogleModuleID) {
    global_options_->PutValue(key.c_str(), Variant(module_id));
    return GetSystemGadgetPath(kIGoogleGadgetName);
  }

  return file_manager_->GetFullPath(
      GetDownloadedGadgetLocation(gadget_id).c_str());
}

std::string GoogleGadgetManager::GetGadgetInstancePath(int instance_id) {
  std::string gadget_id = GetInstanceGadgetId(instance_id);
  return gadget_id.empty() ? std::string() : GetGadgetPath(gadget_id.c_str());
}

class ScriptableGadgetInfo : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x61fde0b5d5b94ab4, ScriptableInterface);

  ScriptableGadgetInfo(const GadgetInfo &info)
      // Must make a copy here because the info may be unavailable when
      // background update runs.
      : info_(info) {
    RegisterConstant("id", info_.id);
    RegisterConstant("source", info_.source);
    RegisterConstant("attributes", NewScriptableMap(info_.attributes));
    RegisterConstant("titles", NewScriptableMap(info_.titles));
    RegisterConstant("descriptions", NewScriptableMap(info_.descriptions));
    RegisterConstant("updated_date", Date(info_.updated_date));
    RegisterConstant("accessed_date", Date(info_.accessed_date));
  }

  // Allow the script to add new script properties to this object.
  virtual bool IsStrict() const { return false; }

  GadgetInfo info_;
};

// Provides utility function for the gadget browser gadget.
class GoogleGadgetManager::GadgetBrowserScriptUtils
    : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x0659826090ca44b0, ScriptableInterface);

  GadgetBrowserScriptUtils(GoogleGadgetManager *gadget_manager)
      : gadget_manager_(gadget_manager) {
    ASSERT(gadget_manager_);
    RegisterProperty("gadgetMetadata",
        NewSlot(this, &GadgetBrowserScriptUtils::GetGadgetMetadata), NULL);
    RegisterMethod("loadThumbnailFromCache",
        NewSlot(this, &GadgetBrowserScriptUtils::LoadThumbnailFromCache));
    RegisterMethod("getThumbnailCachedDate",
        NewSlot(this, &GadgetBrowserScriptUtils::GetThumbnailCachedDate));
    RegisterMethod("saveThumbnailToCache",
        NewSlot(this, &GadgetBrowserScriptUtils::SaveThumbnailToCache));
    RegisterMethod("needDownloadGadget",
        NewSlot(gadget_manager_, &GoogleGadgetManager::NeedDownloadGadget));
    RegisterMethod("needUpdateGadget",
        NewSlot(gadget_manager_, &GoogleGadgetManager::NeedUpdateGadget));
    RegisterMethod("saveGadget",
        NewSlot(this, &GadgetBrowserScriptUtils::SaveGadget));
    RegisterMethod("addGadget",
        NewSlot(gadget_manager_, &GoogleGadgetManager::NewGadgetInstance));
  }

  ~GadgetBrowserScriptUtils() {
  }

  ScriptableArray *GetGadgetMetadata() {
    ScriptableArray *array = new ScriptableArray();
    // Iterate all instances to refresh metadata for local gadgets.
    int size = static_cast<int>(gadget_manager_->instance_statuses_.size());
    for (int i = 0; i < size; ++i)
      gadget_manager_->GetGadgetInfoOfInstance(i);

    const GadgetInfoMap &map = gadget_manager_->GetAllGadgetInfo();
    for (GadgetInfoMap::const_iterator it = map.begin();
         it != map.end(); ++it) {
      const GadgetInfo &info = it->second;
      if (info.source != SOURCE_PLUGINS_XML) {
        // Check if the local or builtin gadget is shadowed by a gadget from
        // plugins.xml with the same uuid. If the gadget is shadowed, it won't
        // be shown in the gadget browser.
        StringMap::const_iterator uuid_it = info.attributes.find("uuid");
        if (uuid_it != info.attributes.end() &&
            map.find(uuid_it->second) != map.end()) {
          DLOG("Local or builtin gadget %s is shadowed by a gadget from"
               "plugins.xml with uuid %s",
               info.id.c_str(), uuid_it->second.c_str());
          continue;
        }
      }
      array->Append(Variant(new ScriptableGadgetInfo(it->second)));
    }
    return array;
  }

  void SaveThumbnailToCache(const char *thumbnail_url,
                            ScriptableBinaryData *image_data) {
    if (thumbnail_url && image_data)
      gadget_manager_->SaveThumbnailToCache(thumbnail_url, image_data->data());
  }

  ScriptableBinaryData *LoadThumbnailFromCache(const char *thumbnail_url) {
    std::string data = gadget_manager_->LoadThumbnailFromCache(thumbnail_url);
    return data.empty() ? NULL : new ScriptableBinaryData(data);
  }

  Date GetThumbnailCachedDate(const char *thumbnail_url) {
    return Date(gadget_manager_->GetThumbnailCachedTime(thumbnail_url));
  }

  bool SaveGadget(const char *gadget_id, ScriptableBinaryData *data) {
    if (gadget_id && data)
      return gadget_manager_->SaveGadget(gadget_id, data->data());
    return false;
  }

  static bool Register(GoogleGadgetManager *manager,
                       ScriptContextInterface *script_context) {
    ASSERT(script_context);
    if (script_context) {
      GadgetBrowserScriptUtils *utils = new GadgetBrowserScriptUtils(manager);
      if (!script_context->AssignFromNative(NULL, NULL, "gadgetBrowserUtils",
                                            Variant(utils))) {
        LOG("Failed to register gadgetBrowserUtils.");
        return false;
      }
      return true;
    }
    return false;
  }
  GoogleGadgetManager *gadget_manager_;
};

void GoogleGadgetManager::ShowGadgetBrowserDialog(HostInterface *host) {
  if (browser_gadget_) {
    if (host != browser_gadget_->GetHost()) {
      delete browser_gadget_;
      browser_gadget_ = NULL;
    }
  }

  if (!browser_gadget_) {
    Permissions permissions;
    permissions.SetGranted(Permissions::ALL_ACCESS, true);
    Gadget::SaveGadgetInitialPermissions(kGoogleGadgetBrowserOptionsName,
                                         permissions);
    browser_gadget_ =
        new Gadget(host,
                   GetSystemGadgetPath(kGoogleGadgetBrowserName).c_str(),
                   kGoogleGadgetBrowserOptionsName,
                   kGoogleGadgetBrowserInstanceId,
                   permissions, Gadget::DEBUG_CONSOLE_DISABLED);

    if (browser_gadget_ && browser_gadget_->IsValid()) {
      View *main_view = browser_gadget_->GetMainView();
      main_view->ConnectOnCloseEvent(
          NewSlot(&metadata_, &GadgetsMetadata::FreeMemory));
      GadgetBrowserScriptUtils::Register(this, main_view->GetScriptContext());
    }
  }

  if (browser_gadget_ && browser_gadget_->IsValid()) {
    browser_gadget_->ShowMainView();
  } else {
    delete browser_gadget_;
    browser_gadget_ = NULL;
    LOG("Failed to load Google Gadget Browser.");
  }
}

void GoogleGadgetManager::ScheduleDailyPing() {
  daily_ping_timer_ = main_loop_->AddTimeoutWatch(
      kDailyPingIntervalBase + rand() % (kDailyPingIntervalBase / 10),
      new WatchCallbackSlot(NewSlot(this, &GoogleGadgetManager::OnDailyPing)));
}

bool GoogleGadgetManager::OnFirstDailyPing(int timer) {
  if (OnDailyPing(timer))
    ScheduleDailyPing();
  return false;
}

bool GoogleGadgetManager::OnDailyPing(int timer) {
  GGL_UNUSED(timer);
  ASSERT(collector_);
  global_options_->PutValue(kLastDailyPingTimeOption,
                            Variant(main_loop_->GetCurrentTime()));
  collector_->ReportUsage();

  int64_t last_weekly_time = 0;
  global_options_->GetValue(kLastWeeklyPingTimeOption).ConvertToInt64(
      &last_weekly_time);
  int64_t current_time = static_cast<int64_t>(main_loop_->GetCurrentTime());
  if (current_time > last_weekly_time + kWeeklyPingIntervalBase) {
    // Report gadget usage weekly.
    int size = static_cast<int>(instance_statuses_.size());
    for (int i = 0; i < size; i++) {
      if (instance_statuses_[i] == kInstanceStatusActive)
        SendGadgetUsagePing(0, GetInstanceGadgetId(i).c_str());
    }
    global_options_->PutValue(kLastWeeklyPingTimeOption, Variant(current_time));
  } else if (current_time < last_weekly_time) {
    // In case of options data error or timer error.
    global_options_->PutValue(kLastWeeklyPingTimeOption, Variant(current_time));
  }
  return true;
}

void GoogleGadgetManager::SendGadgetUsagePing(int type, const char *gadget_id) {
  if (!collector_)
    return;

  const GadgetInfo *info = GetGadgetInfo(gadget_id);
  if (info &&
      (info->source == SOURCE_PLUGINS_XML || info->source == SOURCE_BUILTIN)) {
    // Don't send usage pings for local gadgets.
    std::string version;
    StringMap::const_iterator it = info->attributes.find("version");
    if (it != info->attributes.end())
      version = it->second;

    // Generate a human-readable gadget label.
    std::string gadget_label;
    it = info->attributes.find("id");
    if (it != info->attributes.end()) {
      gadget_label = it->second;
      gadget_label += '-';
    }
    gadget_label += gadget_id;
    gadget_label = MakeGoodFileName(gadget_label.c_str());

    switch (type) {
      case 0:
        collector_->ReportGadgetUsage(gadget_label.c_str(), version.c_str());
        break;
      case 1:
        collector_->ReportGadgetInstall(gadget_label.c_str(), version.c_str());
        break;
      case 2:
        collector_->ReportGadgetUninstall(gadget_label.c_str(),
                                          version.c_str());
        break;
    }
  }
}

} // namespace google
} // namespace ggadget
