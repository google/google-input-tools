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

#ifndef GGADGET_LINUX_FILE_SYSTEM_H__
#define GGADGET_LINUX_FILE_SYSTEM_H__

#include <ggadget/file_system_interface.h>

namespace ggadget {
namespace framework {
namespace linux_system {

class FileSystem : public FileSystemInterface {
 public:
  FileSystem();
  virtual ~FileSystem();

 public:
  virtual DrivesInterface *GetDrives();
  virtual std::string BuildPath(const char *path, const char *name);
  virtual std::string GetDriveName(const char *path);
  virtual std::string GetParentFolderName(const char *path);
  virtual std::string GetFileName(const char *path);
  virtual std::string GetBaseName(const char *path);
  virtual std::string GetExtensionName(const char *path);
  virtual std::string GetAbsolutePathName(const char *path);
  virtual std::string GetTempName();
  virtual bool DriveExists(const char *drive_spec);
  virtual bool FileExists(const char *file_spec);
  virtual bool FolderExists(const char *folder_spec);
  virtual DriveInterface *GetDrive(const char *drive_spec);
  virtual FileInterface *GetFile(const char *file_path);
  virtual FolderInterface *GetFolder(const char *folder_path);
  virtual FolderInterface *GetSpecialFolder(SpecialFolder special_folder);
  virtual bool DeleteFile(const char *file_spec, bool force);
  virtual bool DeleteFolder(const char *folder_spec, bool force);
  virtual bool MoveFile(const char *source, const char *dest);
  virtual bool MoveFolder(const char *source, const char *dest);
  virtual bool CopyFile(const char *source, const char *dest,
                        bool overwrite);
  virtual bool CopyFolder(const char *source, const char *dest,
                          bool overwrite);
  virtual FolderInterface *CreateFolder(const char *path);
  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite,
                                              bool unicode);
  virtual TextStreamInterface *OpenTextFile(const char *filename,
                                            IOMode mode,
                                            bool create,
                                            Tristate format);
  virtual BinaryStreamInterface *CreateBinaryFile(const char *filename,
                                                  bool overwrite);
  virtual BinaryStreamInterface *OpenBinaryFile(const char *filename,
                                                IOMode mode,
                                                bool create);
  virtual TextStreamInterface *GetStandardStream(StandardStreamType type,
                                                 bool unicode);
  virtual std::string GetFileVersion(const char *filename);
};

} // namespace linux_system
} // namespace framework
} // namespace ggadget

#endif // GGADGET_LINUX_FILE_SYSTEM_H__
