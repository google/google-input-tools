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

#ifndef GGADGET_TESTS_MOCKED_FILE_MANAGER_H__
#define GGADGET_TESTS_MOCKED_FILE_MANAGER_H__

#include <ggadget/file_manager_interface.h>
#include <ggadget/common.h>

class MockedFileManager : public ggadget::FileManagerInterface {
 public:
  MockedFileManager() : should_fail_(false) { }
  explicit MockedFileManager(const std::string &path)
    : should_fail_(false), path_(path) { }
  virtual bool IsValid() { return true; }
  virtual bool Init(const char *base_path, bool create) { return true; }
  virtual bool ReadFile(const char *file, std::string *data) {
    requested_file_ = file;
    if (should_fail_) return false;
    *data = data_[file];
    return true;
  }
  virtual bool WriteFile(const char *file, const std::string &data,
                         bool overwrite) {
    requested_file_ = file;
    if (should_fail_) return false;
    data_[file] = data;
    return true;
  }
  virtual bool RemoveFile(const char *file) {
    requested_file_ = file;
    data_.erase(file);
    return true;
  }
  virtual bool ExtractFile(const char *, std::string *) { return false; }
  virtual bool FileExists(const char *file, std::string *path) {
    if (path)
      *path = GetFullPath(file);
    return data_.find(file) != data_.end();
  }
  virtual bool IsDirectlyAccessible(const char *file, std::string *path) {
    return true;
  }
  virtual std::string GetFullPath(const char *file) {
    return path_.empty() ? file : path_ + file;
  }
  virtual uint64_t GetLastModifiedTime(const char *file) { return 0; }
  virtual bool EnumerateFiles(const char *dir,
                              ggadget::Slot1<bool, const char *> *callback) {
    return false;
  }
  bool should_fail_;
  std::string path_;
  std::map<std::string, std::string> data_;
  std::string requested_file_;
};

#endif // GGADGET_TESTS_MOCKED_FILE_MANAGER_H__
