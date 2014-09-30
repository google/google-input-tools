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

#ifndef GGADGET_GOOGLE_GADGET_MANAGER_H__
#define GGADGET_GOOGLE_GADGET_MANAGER_H__

#include <limits.h>
#include <stdint.h>
#include <set>
#include <vector>
#include <ggadget/common.h>
#include <ggadget/signals.h>
#include <ggadget/gadget_manager_interface.h>
#include "gadgets_metadata.h"

namespace ggadget {

class Connection;
class FileManagerInterface;
class MainLoopInterface;
class OptionsInterface;
class HostInterface;
class Gadget;
class PlatformUsageCollectorInterface;

namespace google {

/** Time interval of gadget metadata updates: 7 days. */
const int kGadgetsMetadataUpdateInterval = 7 * 86400 * 1000;

/**
 * First retry interval for unsuccessful gadget metadata updates.
 * If first retry fails, the second retry will be scheduled after
 * 2 * kGadgetMetadataRetryInterval, and so on, until the interval becomes
 * greater than @c kGadgetMetadataRetryMaxInterval.
 */
const int kGadgetsMetadataRetryInterval = 2 * 3600 * 1000;
const int kGadgetsMetadataRetryMaxInterval = 86400 * 1000;

/** Free metadata memory for every 3 minutes. */
const int kFreeMetadataInterval = 180 * 1000;

/**
 * Options key to record the last time of successful metadata update.
 */
const char kLastUpdateTimeOption[] = "metadata_last_update_time";
/**
 * Options key to record the last time of trying metadata update. This value
 * will be cleared when the update successfully finishes.
 */
const char kLastTryTimeOption[] = "metadata_last_try_time";
/**
 * Options key to record the current retry timeout value.
 */
const char kRetryTimeoutOption[] = "metadata_retry_timeout";

/**
 * Options key to record the current maximum instance id of gadget instances
 * including active and inavtive ones.
 */
const char kMaxInstanceIdOption[] = "max_inst_id";

/**
 * The prefix of options keys each of which records the status of gadget
 * instances, including both active and inactive ones. The whole options key
 * is like "inst_status.instance_id".
 * The meaning of the options value:
 *  - 0: An empty slot.
 *  - 1: An active instance.
 *  - >=2: An inactive instance. The value is the expiration score, which is
 *    initially 2 when the instance is just become inactive, and will be
 *    incremented on some events. If the number reaches kExpirationThreshold,
 *    the instance will be actually removed (expire).
 */
const char kInstanceStatusOptionPrefix[] = "inst_status.";

const int kInstanceStatusNone = 0;
const int kInstanceStatusActive = 1;
const int kInstanceStatusInactiveStart = 2;

/**
 * The prefix of options keys to record the corresponding gadget id of
 * each instance. The whole options key is like "inst_gadget_id.instance_id".
 */
const char kInstanceGadgetIdOptionPrefix[] = "inst_gadget_id.";
/**
 * The options key stored in an instance's options file to record the
 * corresponding gadget id. This information is redundant but useful to check
 * if the options file is actually belongs to this gadget.
 */
const char kInstanceGadgetIdOption[] = "gadget_id";

/**
 * The prefix of options keys to record the last added time of the gadget.
 * The whole options key is like "added_time.gadget_id".
 */
const char kGadgetAddedTimeOptionPrefix[] = "added_time.";

/**
 * The prefix of options keys to record the module_id of a gadget.
 * Only iGoogle gadgets and RSS gadgets are recorded:
 *   kIGoogleModuleID
 *   kRSSModuleID
 * Addded 2009/05/15. This is mainly for optimization of GetGadgetPath() by
 * eliminating dependency to gadget metadata. This can speed up program
 * initialization.
 * The code should keep backward compatibility if these items doesn't exist.
 */
const char kGadgetModuleIdOptionPrefix[] = "module_id.";

/**
 * This option item is empty until the first run, and after each run, it's
 * value will be incremented.
 */
const char kRunCountOption[] = "run_count";

/** The time when the last daily ping was sent. */
const char kLastDailyPingTimeOption[] = "last_daily_ping";
const char kLastWeeklyPingTimeOption[] = "last_weekly_ping";

/** A hard limit of maximum number of active and inactive gadget instances. */
const int kMaxNumGadgetInstances = 128;
/** The maximum expiration score before an inactive instance expires. */
const int kExpirationThreshold = 64;

const char kDownloadedGadgetsDir[] = "profile://downloaded_gadgets/";
const char kThumbnailCacheDir[] = "profile://thumbnails/";

const char kModuleIDAttrib[] = "module_id";

/**
 * The following are constants used for feed and iGoogle gadgets.
 * These declarations must match those in the corresponding gadget .js files.
 */
const char kRSSURLOption[] = "rss_url";
const char kRSSModuleID[] = "25";
const char kIGoogleModuleID[] = "32";
const char kIGoogleURLOption[] = "download_url";
/** The following two are optional. */
const char kIGoogleModuleURLOption[] = "module_url_prefix";
const char kIGoogleBGColorOption[] = "bg_color";

/** Fake instance id for google gadget browser. */
const int kGoogleGadgetBrowserInstanceId = INT_MAX;
const char kGoogleGadgetBrowserName[] = "google-gadget-browser";
const char kGoogleGadgetBrowserOptionsName[] = "google-gadget-browser";

/**
 * Manages gadgets and instances based on Google gadget metadata.
 * Notes about gadget id and gadget instance id:
 *   - gadget id: the string id of a gadget (now the plugin uuid is used for
 *     desktop gadgets, and download_url is used for igoogle gadgets).
 *     It's stored in @c GadgetMetadata::GadgetInfo::id at runtime.
 *     Some methods also accept @c gadget_id containing location of a gadget
 *     file. This location can be a full path or any location that can be
 *     recognized by the global file manager.
 *   - gadget instance id: an integer serial number of a gadget instance. One
 *     gadget can have multiple instances.
 *
 * An instance may be active or inactive. Active instances will run in the
 * container. When the last instance of a gadget is to be removed, the instance
 * won't be actually removed, but becomes inactive. When the user'd add a new
 * instance of the gadget, the inactive instance will be reused, so that the
 * last options data can continue to be used.
 *
 * NOTE: The backoff and randomization features in this implementation is
 * very important for proper server-side operation. Please do *NOT* disable
 * or remove them.
 */
class GoogleGadgetManager : public GadgetManagerInterface {
 public:
  GoogleGadgetManager();
  virtual ~GoogleGadgetManager();

 public: // interface methods.
  virtual void Init();
  virtual int NewGadgetInstanceFromFile(const char *file);
  virtual bool RemoveGadgetInstance(int instance_id);
  virtual std::string GetGadgetInstanceOptionsName(int instance_id);
  virtual bool EnumerateGadgetInstances(Slot1<bool, int> *callback);
  virtual std::string GetGadgetInstancePath(int instance_id);
  virtual void ShowGadgetBrowserDialog(HostInterface *host);
  virtual bool GetGadgetDefaultPermissions(int instance_id,
                                           Permissions *permissions);
  virtual bool GetGadgetInstanceInfo(int instance_id, const char *locale,
                                     std::string *author,
                                     std::string *download_url,
                                     std::string *title,
                                     std::string *description);
  virtual std::string GetGadgetInstanceFeedbackURL(int instance_id);

  virtual Connection *ConnectOnNewGadgetInstance(Slot1<bool, int> *callback);
  virtual Connection *ConnectOnRemoveGadgetInstance(Slot1<void, int> *callback);
  virtual Connection *ConnectOnUpdateGadgetInstance(Slot1<void, int> *callback);

 public: // methods for unittest.
  /**
   * Forces an update of gadget metadata.
   * @param full_download whether the update should be a full download
   *    instead of an incremental download.
   * @return @c false if there is running updating session.
   */
  bool UpdateGadgetsMetadata(bool full_download);

  /**
   * Creates an instance of a gadget.
   * @param gadget_id gadget id or location of a gadget file.
   * @return the gadget instance id (>=0) of the new instance, or -1 on error.
   */
  int NewGadgetInstance(const char *gadget_id);

  /**
   * Updates a running gadget instances by reloading the gadget file.
   * Normally this is called after a gadget file is just upgraded.
   */
  void UpdateGadgetInstances(const char *gadget_id);

  /**
   * Returns the current gadgets metadata.
   */
  const GadgetInfoMap &GetAllGadgetInfo();

  /**
   * Returns the current metadata for a gadget.
   * @param gadget_id gadget id, file location not allowed.
   * @return the corresponding gadget metadata for the gadget, or @c NULL if
   *     not found.
   */
  const GadgetInfo *GetGadgetInfo(const char *gadget_id);

  /**
   * Get the corresponding gadget info for a instance.
   * @param instance_id the id of a gadget instance, either active or inactive.
   * @return the gadget info, or NULL if the instance is not found.
   */
  const GadgetInfo *GetGadgetInfoOfInstance(int instance_id);

  /**
   * Checks if the gadget has at least one active instance.
   * @param gadget_id gadget id, or location of a gadget file.
   * @return @c true if the gadget has at least one active instance.
   */
  bool GadgetHasInstance(const char *gadget_id);

  /**
   * Get the corresponding gadget id for a gadget instance (active or
   * inactive). Returns empty string if the instance_id is invalid.
   * @param instance_id id of a gadget instance, either active or inactive.
   * @return the gadget id, or the location of the gadget file if the gadget
   *     was added with file location.
   */
  std::string GetInstanceGadgetId(int instance_id);

  /**
   * Gadget thumbnail caching management.
   */
  void SaveThumbnailToCache(const char *thumbnail_url, const std::string &data);
  uint64_t GetThumbnailCachedTime(const char *thumbnail_url);
  std::string LoadThumbnailFromCache(const char *thumbnail_url);

  /**
   * Checks if the gadget needs to be downloaded.
   * @param gadget_id gadget id, file location not allowed.
   * @return @c true if the gadget need to be downloaded.
   */
  bool NeedDownloadGadget(const char *gadget_id);

  /**
   * Checks if the gadget needs to be updated.
   * @param gadget_id gadget id, file location not allowed.
   * @return @c true if the gadget need to be updated.
   */
  bool NeedUpdateGadget(const char *gadget_id);

  /**
   * Saves gadget file content to the file system.
   * @param gadget_id gadget id, file location not allowed.
   * @param data the binary gadget file content.
   * @return @c true if succeeds.
   */
  bool SaveGadget(const char *gadget_id, const std::string &data);

  /**
   * Get the full path of the file for a gadget.
   * @param gadget_id gadget id, or location of a gadget file.
   * @return the full path of the file for a gadget.
   */
  std::string GetGadgetPath(const char *gadget_id);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(GoogleGadgetManager);

  void ScheduleNextUpdate();
  void ScheduleUpdate(int64_t time);
  bool OnFreeMetadataTimer(int timer);
  bool OnUpdateTimer(int timer);
  void OnUpdateDone(bool request_success, bool parsing_success);
  void SaveInstanceGadgetId(int instance_id, const char *gadget_id);
  void SetInstanceStatus(int instance_id, int status);
  void TrimInstanceStatuses();
  void ActuallyRemoveInstance(int instance_id, bool remove_downloaded_file);
  void IncreseAndCheckExpirationScores();
  int GetNewInstanceId();
  bool GadgetIdIsFileLocation(const char *gadget_id);
  bool GadgetIdIsSystemName(const char *gadget_id);
  bool NeedDownloadOrUpdateGadget(const char *gadget_id, bool failure_result);
  std::string GetDownloadedGadgetLocation(const char *gadget_id);
  bool InitInstanceOptions(const char *gadget_id, int instance_id);
  void ScheduleDailyPing();
  bool OnFirstDailyPing(int timer);
  bool OnDailyPing(int timer);
  void SendGadgetUsagePing(int type, const char *gadget_id);
  bool RemoveGadgetInstanceInternal(int instance_id, bool send_ping);

  class GadgetBrowserScriptUtils;

  MainLoopInterface *main_loop_;
  OptionsInterface *global_options_;
  FileManagerInterface *file_manager_;
  int64_t last_update_time_, last_try_time_;
  int retry_timeout_;
  int update_timer_, free_metadata_timer_, daily_ping_timer_;
  bool full_download_;  // Records the last UpdateGadgetsMetadata mode.
  bool updating_metadata_;

  // Contains the statuses of all active and inactive gadget instances.
  typedef std::vector<int> InstanceStatuses;
  InstanceStatuses instance_statuses_;

  // Set of gadgets each of which has at least one active instance.
  LightSet<std::string> active_gadgets_;

  Signal1<bool, int> new_instance_signal_;
  Signal1<void, int> remove_instance_signal_;
  Signal1<void, int> update_instance_signal_;

  GadgetsMetadata metadata_;
  Gadget *browser_gadget_;
  bool first_run_;
  PlatformUsageCollectorInterface *collector_;
};

} // namespace google
} // namespace ggadget

#endif // GGADGET_GOOGLE_GADGET_MANAGER_H__
