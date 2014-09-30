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

#include <stdlib.h>
#include <cstring>
#include <string>

#include <ggadget/logger.h>
#include <ggadget/options_interface.h>
#include <ggadget/string_utils.h>
#include <ggadget/usage_collector_interface.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_parser_interface.h>

namespace ggadget {

static const char kAnalyticsURLPrefix[] =
    "http://www.google-analytics.com/__utm.gif?utmwv=4.3";
static const char kUserIdOptionPrefix[] = "collector-user-id";
static const char kFirstUseTimeOptionPrefix[] = "collector-first-use";
static const char kLastUseTimeOptionPrefix[] = "collector-last-use";

// TODO: Config?
static const char kPlatformUsageAccount[] = "UA-6103715-1";
static const char kPlatformFirstUsePing[] = "/firstuse/";
static const char kPlatformUsagePing[] = "/usage/";

static const char kGadgetsUsageAccount[] = "UA-6103720-1";
static const char kGadgetInstallPingPrefix[] = "/gadget-install/";
static const char kGadgetUninstallPingPrefix[] = "/gadget-uninstall/";
static const char kGadgetUsagePingPrefix[] = "/gadget-usage/";

static const char *kParamNames[
    static_cast<size_t>(UsageCollectorFactoryInterface::PARAM_MAX)] = {
    "utmsr"
};

class UsageCollector : public UsageCollectorInterface {
 public:
  UsageCollector(const char *account,
                 const std::string *params,
                 OptionsInterface *options)
      : account_(account),
        params_(params),
        options_(options),
        user_id_(0), first_use_time_(0), last_use_time_(0) {
    ASSERT(options);

    options_->GetInternalValue((kUserIdOptionPrefix + account_).c_str())
        .ConvertToInt(&user_id_);
    int64_t first_use_time = 0, last_use_time = 0;
    options_->GetInternalValue((kFirstUseTimeOptionPrefix + account_).c_str())
        .ConvertToInt64(&first_use_time);
    options_->GetInternalValue((kLastUseTimeOptionPrefix + account_).c_str())
        .ConvertToInt64(&last_use_time);
    if (user_id_ <= 0 || first_use_time <= 0 ||
        last_use_time <= 0 || last_use_time < first_use_time) {
      first_use_time_ = time(NULL);
      user_id_ = abs(static_cast<int>(rand() * first_use_time_));
      last_use_time_ = first_use_time_;
      options_->PutInternalValue((kUserIdOptionPrefix + account_).c_str(),
                                 Variant(user_id_));
      options_->PutInternalValue((kFirstUseTimeOptionPrefix + account_).c_str(),
                                 Variant(first_use_time_));
      options_->PutInternalValue((kLastUseTimeOptionPrefix + account_).c_str(),
                                 Variant(last_use_time_));
    } else {
      first_use_time_ = static_cast<time_t>(first_use_time);
      last_use_time_ = static_cast<time_t>(last_use_time);
    }
  }

  virtual void Report(const char *usage) {
    ASSERT(usage);
    XMLHttpRequestInterface *request =
        GetXMLHttpRequestFactory()->CreateXMLHttpRequest(0, GetXMLParser());
    request->Ref();

    time_t this_use_time = time(NULL);
    std::string url = StringPrintf("%s&utmn=%d&utmhn=no.domain.com&utmcs=UTF-8",
                                   kAnalyticsURLPrefix, rand());
    if (params_) {
      for (size_t i = 0; i < arraysize(kParamNames); i++) {
        if (!params_[i].empty()) {
          url += '&';
          url += kParamNames[i];
          url += '=';
          url += EncodeURLComponent(params_[i]);
        }
      }
    }

    StringAppendPrintf(&url,
        "&utmdt=-&utmhid=%d&utmr=-&utmp=%s&utmac=%s&"
        "utmcc=__utma%%3D%d.%jd.%u.%u.%u.1%%3B%%2B__utmv%%3D%d.%s%%3B",
        rand(), EncodeURLComponent(usage).c_str(), account_.c_str(),
        user_id_, static_cast<int64_t>(rand()) * rand(),
        static_cast<unsigned int>(first_use_time_),
        static_cast<unsigned int>(last_use_time_),
        static_cast<unsigned int>(this_use_time),
        user_id_,
#ifdef GGL_OEM_BRAND
        EncodeURLComponent(GGL_OEM_BRAND).c_str());
#else
        "-");
#endif

    DLOG("Report to Analytics: %s", url.c_str());
    request->Open("GET", url.c_str(), true, NULL, NULL);
    request->Send(NULL);
    request->Unref();

    last_use_time_ = this_use_time;
    options_->PutInternalValue((kLastUseTimeOptionPrefix + account_).c_str(),
                               Variant(last_use_time_));
  }

  std::string account_;
  const std::string *params_;
  OptionsInterface *options_;
  int user_id_;
  time_t first_use_time_, last_use_time_;
};

class PlatformUsageCollector : public PlatformUsageCollectorInterface {
 public:
  PlatformUsageCollector(const std::string &application_name,
                         const std::string &version,
                         const std::string *params)
      : application_name_(application_name),
        version_(version),
        platform_collector_(kPlatformUsageAccount, params, GetGlobalOptions()),
        gadgets_collector_(kGadgetsUsageAccount, params, GetGlobalOptions()) {
  }

  void ReportPlatform(const char *prefix) {
    platform_collector_.Report(
        (prefix + EncodeURLComponent(application_name_.c_str()) + "/" +
         EncodeURLComponent(version_.c_str())
#ifdef GGL_DIST_INFO
         + "/" + EncodeURLComponent(GGL_DIST_INFO)
#endif
        ).c_str());
  }

  virtual void ReportFirstUse() {
    ReportPlatform(kPlatformFirstUsePing);
  }

  virtual void ReportUsage() {
    ReportPlatform(kPlatformUsagePing);
  }

  void ReportGadget(const char *prefix, const char *item, const char *version) {
    ASSERT(item && version);
    gadgets_collector_.Report((prefix + EncodeURLComponent(item) + "/" +
                               EncodeURLComponent(version)).c_str());
  }

  virtual void ReportGadgetInstall(const char *gadget_id,
                                   const char *version) {
    ReportGadget(kGadgetInstallPingPrefix, gadget_id, version);
  }

  virtual void ReportGadgetUninstall(const char *gadget_id,
                                     const char *version) {
    ReportGadget(kGadgetUninstallPingPrefix, gadget_id, version);
  }

  virtual void ReportGadgetUsage(const char *gadget_id, const char *version) {
    ReportGadget(kGadgetUsagePingPrefix, gadget_id, version);
  }

  std::string application_name_, version_;
  UsageCollector platform_collector_, gadgets_collector_;
};

class UsageCollectorFactory : public UsageCollectorFactoryInterface {
 public:
  UsageCollectorFactory()
      : platform_collector_(NULL) {
  }

  virtual ~UsageCollectorFactory() {
    delete platform_collector_;
  }

  virtual UsageCollectorInterface *CreateUsageCollector(
      const char *account, bool allow_params, OptionsInterface *options) {

    return new UsageCollector(account,
                              allow_params ? params_ : NULL,
                              options);
  }

  virtual PlatformUsageCollectorInterface *GetPlatformUsageCollector() {
    if (application_name_.empty()) {
      // Platform usage collector has not been enabled.
      return NULL;
    }
    if (!platform_collector_) {
      platform_collector_ = new PlatformUsageCollector(application_name_,
                                                       version_,
                                                       params_);
    }
    return platform_collector_;
  }

  virtual void SetApplicationInfo(const char *application_name,
                                  const char *version) {
    ASSERT(application_name && version && *application_name && *version);
    application_name_ = application_name;
    version_ = version;
  }

  virtual void SetParameter(Parameter param, const char *value) {
    ASSERT(value);
    if (param >= 0 && param < PARAM_MAX)
      params_[param] = value;
  }

  std::string params_[PARAM_MAX];
  PlatformUsageCollector *platform_collector_;
  std::string application_name_, version_;
};

} // namespace ggadget

#define Initialize analytics_usage_collector_LTX_Initialize
#define Finalize analytics_usage_collector_LTX_Finalize

static ggadget::UsageCollectorFactory g_factory;

extern "C" {
  bool Initialize() {
    LOGI("Initialize analytics_usage_collector extension.");
    return ggadget::SetUsageCollectorFactory(&g_factory);
  }

  void Finalize() {
    LOGI("Finalize analytics_usage_collector extension.");
  }
}
