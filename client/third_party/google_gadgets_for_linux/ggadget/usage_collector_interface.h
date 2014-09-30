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

#ifndef GGADGET_USAGE_COLLECTOR_INTERFACE_H__
#define GGADGET_USAGE_COLLECTOR_INTERFACE_H__

namespace ggadget {

class OptionsInterface;

/**
 * @ingroup Interfaces
 * The interface used to collect usage data.
 */
class UsageCollectorInterface {
 public:
  virtual ~UsageCollectorInterface() { }

 public:
  /**
   * Report a usage event.
   * The format of usage should be like an absolute file path.
   */
  virtual void Report(const char *usage) = 0;
};

class PlatformUsageCollectorInterface {
 public:
  virtual ~PlatformUsageCollectorInterface() { }

 public:
  virtual void ReportFirstUse() = 0;
  virtual void ReportUsage() = 0;
  /** Reports the events of install, uninstall and usage status of a gadget. */
  virtual void ReportGadgetInstall(const char *gadget_id,
                                   const char *version) = 0;
  virtual void ReportGadgetUninstall(const char *gadget_id,
                                     const char *version) = 0;
  virtual void ReportGadgetUsage(const char *gadget_id,
                                 const char *version) = 0;
};

class UsageCollectorFactoryInterface {
 public:
  virtual ~UsageCollectorFactoryInterface() { }

 public:
  /**
   * Creates a @c UsageCollectorInterface instance.
   * @param account the account name for the usage collector.
   * @param allow_params whether allow the collector to send extra parameters
   *     set by @c SetParameter().
   * @param options the options in which the usage collector records status
   *     information.
   * @return the created instance. Should never return @c NULL.
   */
  virtual UsageCollectorInterface *CreateUsageCollector(
      const char *account, bool allow_params, OptionsInterface *options) = 0;

  /**
   * Gets the @c UsageCollectorInterface instance used to collect usage data
   * of the gadget platform.
   */
  virtual PlatformUsageCollectorInterface *GetPlatformUsageCollector() = 0;

  /**
   * Enables the platform usage collector, and sets the name and version
   * of the current application.
   */
  virtual void SetApplicationInfo(const char *application_name,
                                  const char *version) = 0;

  enum Parameter {
    /** The size of current screen, like "1024x768" */
    PARAM_SCREEN_SIZE,
    PARAM_MAX,
  };
  virtual void SetParameter(Parameter param, const char *value) = 0;
};

/**
 * Sets a @c UsageCollectorFactory as the global UsageCollector factory.
 * An UsageCollector extension module can call this function in its
 * @c Initialize() function.
 */
bool SetUsageCollectorFactory(UsageCollectorFactoryInterface *factory);

/**
 * Gets the global UsageCollector factory.
 */
UsageCollectorFactoryInterface *GetUsageCollectorFactory();

} // namespace ggadget

#endif // GGADGET_USAGE_COLLECTOR_INTERFACE_H__
