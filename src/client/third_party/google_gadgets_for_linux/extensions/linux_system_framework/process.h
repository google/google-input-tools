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

#ifndef EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_PROCESS_H__
#define EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_PROCESS_H__

#include <string>
#include <vector>
#include <ggadget/framework_interface.h>

namespace ggadget {
namespace framework {
namespace linux_system {

class ProcessInfo : public ProcessInfoInterface {
 public:
  ProcessInfo(int pid, const std::string &path);

 public:
  virtual void Destroy();

 public:
  virtual int GetProcessId() const;
  virtual std::string GetExecutablePath() const;

 private:
  int pid_;
  std::string path_;
};

class Processes : public ProcessesInterface {
 public:
  Processes();
  virtual void Destroy();

  virtual int GetCount() const;
  virtual ProcessInfoInterface *GetItem(int index);

 private:
  void InitProcesses();

 private:
  typedef std::pair<int, std::string> IntStringPair;
   std::vector<IntStringPair> procs_;
};

class Process : public ProcessInterface {
 public:
  virtual ProcessesInterface *EnumerateProcesses();
  virtual ProcessInfoInterface *GetForeground();
  virtual ProcessInfoInterface *GetInfo(int pid);
};

} // namespace linux_system
} // namespace framework
} // namespace ggadget

#endif // EXTENSIONS_LINUX_SYSTEM_FRAMEWORK_PROCESS_H__
