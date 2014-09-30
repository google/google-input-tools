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

#include "zip_file_manager.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <vector>
#include <string>
#include <cstdio>
#include <cerrno>
#include <ctime>

#include <third_party/unzip/zip.h>
#include <third_party/unzip/unzip.h>
#include "common.h"
#include "logger.h"
#include "gadget_consts.h"
#include "scoped_ptr.h"
#include "slot.h"
#include "small_object.h"
#include "string_utils.h"
#include "system_file_functions.h"
#include "system_utils.h"

namespace ggadget {

#ifdef GADGET_CASE_SENSITIVE
static const int kZipCaseSensitivity = 1;
#else
static const int kZipCaseSensitivity = 2;
#endif

static const uLong kMaxFieldSize = 200000;
static const char kZipGlobalComment[] = "Created by Google Gadgets for Linux.";
static const char kZipReadMeFile[] = ".readme";
static const char kTempZipFile[] = "%%Temp%%.zip";

namespace {
#if defined(OS_WIN)
// Zip uses '/' for internal path inside the package. But windows convention
// uses a '\\'. To make unzip compatible with windows convention, here it
// wrappers all unzip interfaces involving internal file paths. It replaces
// '\\' with '/' when passing an internal file path to unzip, and repalces '/'
// back to '\\' when receiving an internal file path from unzip.

// replace old_char with new_char in path.
void StringReplace(char* path, char old_char, char new_char) {
  if (path == NULL)
    return;
  while (*path != '\0') {
    if (*path == old_char)
      *path = new_char;
    ++path;
  }
  return;
}
#endif

int UnzLocateFile(unzFile file, const char* szFileName, int iCaseSensitivity) {
#if defined(OS_WIN)
  int name_buffer_length = strlen(szFileName) + 1;
  scoped_array<char> file_name(new char[name_buffer_length]);
  strcpy_s(file_name.get(), name_buffer_length, szFileName);
  StringReplace(file_name.get(), '\\', '/');
  return ::unzLocateFile(file, file_name.get(), iCaseSensitivity);
#elif defined(OS_POSIX)
  return ::unzLocateFile(file, szFileName, iCaseSensitivity);
#endif
}

int UnzGetCurrentFileInfo(unzFile file, unz_file_info* pfile_info,
                          char* szFileName, uLong fileNameBufferSize,
                          void* extraField, uLong extraFieldBufferSize,
                          char *szComment, uLong commentBufferSize) {
#if defined(OS_WIN)
  int result_code = ::unzGetCurrentFileInfo(file, pfile_info, szFileName,
                                            fileNameBufferSize, extraField,
                                            extraFieldBufferSize, szComment,
                                            commentBufferSize);
  StringReplace(szFileName, '/', '\\');
  return result_code;
#elif defined(OS_POSIX)
  return ::unzGetCurrentFileInfo(file, pfile_info, szFileName,
                                 fileNameBufferSize, extraField,
                                 extraFieldBufferSize, szComment,
                                 commentBufferSize);
#endif
}

int ZipOpenNewFileInZip(zipFile file,
                        const char* filename,
                        const zip_fileinfo* zipfi,
                        const void* extrafield_local,
                        uInt size_extrafield_local,
                        const void* extrafield_global,
                        uInt size_extrafield_global,
                        const char* comment,
                        int method,
                        int level) {
#if defined(OS_WIN)
  int name_buffer_length = strlen(filename) + 1;
  scoped_array<char> file_name(new char[name_buffer_length]);
  strcpy_s(file_name.get(), name_buffer_length, filename);
  StringReplace(file_name.get(), '\\', '/');
  return ::zipOpenNewFileInZip(file, file_name.get(), zipfi, extrafield_local,
                               size_extrafield_local, extrafield_global,
                               size_extrafield_global, comment, method, level);
#elif defined(OS_POSIX)
  return ::zipOpenNewFileInZip(file, filename, zipfi, extrafield_local,
                               size_extrafield_local, extrafield_global,
                               size_extrafield_global, comment, method, level);
#endif
}
} // namespace

class ZipFileManager::Impl : public SmallObject<> {
 public:
  Impl() : unzip_handle_(NULL), zip_handle_(NULL) {
  }

  ~Impl() {
    Finalize();
  }

  void Finalize() {
    if (temp_dir_.length())
      RemoveDirectory(temp_dir_.c_str(), true);

    temp_dir_.clear();
    base_path_.clear();

    if (unzip_handle_)
      unzClose(unzip_handle_);
    if (zip_handle_)
      zipClose(zip_handle_, kZipGlobalComment);

    unzip_handle_ = NULL;
    zip_handle_ = NULL;
  }

  bool IsValid() {
    return !base_path_.empty() && (zip_handle_ || unzip_handle_);
  }

  bool Init(const char *base_path, bool create) {
    if (!base_path || !base_path[0]) {
      LOG("Base path is empty.");
      return false;
    }

    std::string path(base_path);

    // Use absolute path.
    if (!ggadget::IsAbsolutePath(base_path))
      path = BuildFilePath(GetCurrentDirectory().c_str(), base_path, NULL);

    path = NormalizeFilePath(path.c_str());

    unzFile unzip_handle = NULL;
    zipFile zip_handle = NULL;
    ggadget::StatStruct stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    if (ggadget::stat(path.c_str(), &stat_value) == 0) {
      if (!S_ISREG(stat_value.st_mode)) {
        DLOG("Not a regular file: %s", path.c_str());
        return false;
      }

      if (ggadget::access(path.c_str(), R_OK) != 0) {
        LOG("No permission to access the file %s", path.c_str());
        return false;
      }

      unzip_handle = unzOpen(path.c_str());
      if (!unzip_handle) {
        LOG("Failed to open zip file %s for reading", path.c_str());
        return false;
      }
    } else if (errno == ENOENT && create) {
      zip_handle = zipOpen(path.c_str(), APPEND_STATUS_CREATE);
      if (!zip_handle) {
        LOG("Failed to open zip file %s for writing", path.c_str());
        return false;
      }
      AddReadMeFileInZip(zip_handle, path.c_str());
    } else {
      LOG("Failed to open zip file %s: %s", path.c_str(), strerror(errno));
      return false;
    }

    DLOG("ZipFileManager was initialized successfully for path %s",
         path.c_str());

    Finalize();

    unzip_handle_ = unzip_handle;
    zip_handle_ = zip_handle;
    base_path_ = path;
    return true;
  }

  bool ReadFile(const char *file, std::string *data) {
    ASSERT(data);
    if (data) data->clear();

    std::string relative_path;
    if (!CheckFilePath(file, &relative_path, NULL))
      return false;

    if (!SwitchToRead())
      return false;

    if (ggadget::UnzLocateFile(unzip_handle_, relative_path.c_str(),
                               kZipCaseSensitivity) != UNZ_OK)
      return false;

    if (unzOpenCurrentFile(unzip_handle_) != UNZ_OK) {
      LOG("Can't open file %s for reading in zip archive %s.",
          relative_path.c_str(), base_path_.c_str());
      return false;
    }

    bool result = true;
    const int kChunkSize = 2048;
    char buffer[kChunkSize];
    while (true) {
      int read_size = unzReadCurrentFile(unzip_handle_, buffer, kChunkSize);
      if (read_size > 0) {
        data->append(buffer, read_size);
        if (data->length() > kMaxFileSize) {
          LOG("File %s is too big", relative_path.c_str());
          data->clear();
          result = false;
          break;
        }
      } else if (read_size < 0) {
        LOG("Error reading file: %s in zip archive %s",
            relative_path.c_str(), base_path_.c_str());
        data->clear();
        result = false;
        break;
      } else {
        break;
      }
    }

    if (unzCloseCurrentFile(unzip_handle_) != UNZ_OK) {
      LOG("CRC error in file: %s in zip file: %s",
          relative_path.c_str(), base_path_.c_str());
      data->clear();
      result = false;
    }
    return result;
  }

  bool WriteFile(const char *file, const std::string &data, bool overwrite) {
    std::string relative_path;
    if (!CheckFilePath(file, &relative_path, NULL))
      return false;

    if (FileExists(file, NULL)) {
      if (!overwrite) {
        LOG("Can't overwrite an existing file %s in zip archive %s.",
            relative_path.c_str(), base_path_.c_str());
        return false;
      }
      if (!RemoveFile(file))
        return false;
    }

    if (!SwitchToWrite())
      return false;

    return AddFileInZip(zip_handle_, base_path_.c_str(), relative_path.c_str(),
                        data.c_str(), data.length());
  }

  class CopyZipFile {
   public:
    CopyZipFile(Impl *impl, zipFile dest, const char *excluded_file)
        : impl_(impl), dest_(dest), excluded_file_(excluded_file) {
    }
    bool Copy(const char *filename) {
      if (GadgetStrCmp(filename, excluded_file_) == 0) {
        // Don't copy this excluded file;
        return true;
      }

      unz_file_info unz_info;
      if (ggadget::UnzGetCurrentFileInfo(impl_->unzip_handle_, &unz_info,
                                         NULL, 0, NULL, 0, NULL, 0) != UNZ_OK ||
          unz_info.size_file_extra > kMaxFieldSize ||
          unz_info.size_file_comment > kMaxFieldSize)
        return false;
      char *extra = new char[unz_info.size_file_extra];
      char *comment = new char[unz_info.size_file_comment + 1];
      if (ggadget::UnzGetCurrentFileInfo(impl_->unzip_handle_,
                                         &unz_info, NULL, 0, extra,
                                         unz_info.size_file_extra, comment,
                                         unz_info.size_file_comment + 1)
          != UNZ_OK) {
        delete [] extra;
        delete [] comment;
        return false;
      }

      zip_fileinfo zip_info;
      memset(&zip_info, 0, sizeof(zip_info));
      zip_info.dosDate = unz_info.dosDate;
      zip_info.internal_fa = unz_info.internal_fa;
      zip_info.external_fa = unz_info.external_fa;
      std::string content;
      bool result = ggadget::ZipOpenNewFileInZip(
                        dest_, filename, &zip_info, extra,
                        static_cast<uInt>(unz_info.size_file_extra),
                        NULL, 0, comment,
                        static_cast<int>(unz_info.compression_method),
                        Z_DEFAULT_COMPRESSION) == ZIP_OK &&
                    impl_->ReadFile(filename, &content) &&
                    zipWriteInFileInZip(
                        dest_, content.c_str(),
                        static_cast<unsigned>(content.size())) == UNZ_OK;
      if (!result)
        LOG("Failed to copy file %s from zip to temp zip", filename);
      delete [] extra;
      delete [] comment;
      zipCloseFileInZip(dest_);
      return result;
    }
   private:
    Impl *impl_;
    zipFile dest_;
    const char *excluded_file_;
  };

  bool RemoveFile(const char *file) {
    if (!FileExists(file, NULL) || !SwitchToRead() || !EnsureTempDirectory())
      return false;

    unz_global_info global_info;
    char *global_comment = NULL;
    if (unzGetGlobalInfo(unzip_handle_, &global_info) == UNZ_OK &&
        global_info.size_comment <= kMaxFieldSize) {
      global_comment = new char[global_info.size_comment + 1];
      if (unzGetGlobalComment(unzip_handle_, global_comment,
                              global_info.size_comment + 1) < 0) {
        delete [] global_comment;
        global_comment = NULL;
      }
    }

    std::string temp_file = BuildFilePath(temp_dir_.c_str(),
                                          kTempZipFile, NULL);
    ggadget::unlink(temp_file.c_str());
    zipFile temp_zip = zipOpen(temp_file.c_str(), APPEND_STATUS_CREATE);
    if (!temp_zip) {
      LOG("Can't create temp zip file: %s", temp_file.c_str());
      return false;
    }
    AddReadMeFileInZip(temp_zip, temp_file.c_str());

    CopyZipFile copy_zip_file(this, temp_zip, file);
    bool res =
        EnumerateFiles("", NewSlot(&copy_zip_file, &CopyZipFile::Copy)) == 0;
    zipClose(temp_zip, global_comment);
    delete [] global_comment;

    if (res) {
      // Copy the temp zip file over the original zip.
      unzClose(unzip_handle_);
      unzip_handle_ = NULL;
      res = ggadget::unlink(base_path_.c_str()) == 0 &&
            CopyFile(temp_file.c_str(), base_path_.c_str());
      if (!res) {
        LOG("Failed to copy temp zip file %s to original zip file %s: %s",
            temp_file.c_str(), base_path_.c_str(), strerror(errno));
      }
    }
    ggadget::unlink(temp_file.c_str());
    return res;
  }

  bool ExtractFile(const char *file, std::string *into_file) {
    ASSERT(into_file);

    std::string relative_path;
    if (!CheckFilePath(file, &relative_path, NULL))
      return false;

    if (!SwitchToRead())
      return false;

    if (ggadget::UnzLocateFile(unzip_handle_, relative_path.c_str(),
                               kZipCaseSensitivity) != UNZ_OK)
      return false;

    if (into_file->empty()) {
      if (!EnsureTempDirectory())
        return false;

      // Creates the relative sub directories under temp direcotry.
      std::string dir, file_name;
      SplitFilePath(relative_path.c_str(), &dir, &file_name);

      dir = BuildFilePath(temp_dir_.c_str(), dir.c_str(), NULL);
      if (!EnsureDirectories(dir.c_str()))
        return false;

      *into_file = BuildFilePath(dir.c_str(), file_name.c_str(), NULL);
    }

    ggadget::unlink(into_file->c_str());
    FILE *out_fp = ggadget::fopen(into_file->c_str(), "wb");
    if (!out_fp) {
      LOG("Can't open file %s for writing.", into_file->c_str());
      return false;
    }

    if (unzOpenCurrentFile(unzip_handle_) != UNZ_OK) {
      LOG("Can't open file %s for reading in zip archive %s.",
          relative_path.c_str(), base_path_.c_str());
      fclose(out_fp);
      return false;
    }

    bool result = true;
    const int kChunkSize = 8192;
    char buffer[kChunkSize];
    while(true) {
      int read_size = unzReadCurrentFile(unzip_handle_, buffer, kChunkSize);
      if (read_size > 0) {
        if (fwrite(buffer, read_size, 1, out_fp) != 1) {
          result = false;
          LOG("Error when writing to file %s", into_file->c_str());
          break;
        }
      } else if (read_size < 0) {
        LOG("Error reading file: %s in zip archive %s",
            relative_path.c_str(), base_path_.c_str());
        result = false;
        break;
      } else {
        break;
      }
    }

    if (unzCloseCurrentFile(unzip_handle_) != UNZ_OK) {
      LOG("CRC error in file: %s in zip file: %s",
          relative_path.c_str(), base_path_.c_str());
      result = false;
    }
    // fclose() is placed first to ensure it's always called.
    result = (fclose(out_fp) == 0 && result);

    if (!result)
      ggadget::unlink(into_file->c_str());

    return result;
  }

  bool FileExists(const char *file, std::string *path) {
    std::string full_path, relative_path;
    bool result = CheckFilePath(file, &relative_path, &full_path);
    if (path) *path = full_path;

    return result && SwitchToRead() &&
           ggadget::UnzLocateFile(unzip_handle_, relative_path.c_str(),
                                  kZipCaseSensitivity) == UNZ_OK;
  }

  bool IsDirectlyAccessible(const char *file, std::string *path) {
    CheckFilePath(file, NULL, path);
    return false;
  }

  std::string GetFullPath(const char *file) {
    std::string path;
    if (!file || !*file)
      return base_path_;
    else if (CheckFilePath(file, NULL, &path))
      return path;
    return std::string("");
  }

  uint64_t GetLastModifiedTime(const char *file) {
    std::string full_path, relative_path;
    bool result = CheckFilePath(file, &relative_path, &full_path);

    unz_file_info file_info;
    if (result && SwitchToRead() &&
        ggadget::UnzLocateFile(unzip_handle_, relative_path.c_str(),
                               kZipCaseSensitivity) == UNZ_OK &&
        ggadget::UnzGetCurrentFileInfo(unzip_handle_, &file_info,
                                       NULL, 0, NULL, 0, NULL, 0) == UNZ_OK) {
      struct tm tm;
      memset(&tm, 0, sizeof(tm));
      tm.tm_year = file_info.tmu_date.tm_year - 1900;
      tm.tm_mon = file_info.tmu_date.tm_mon;
      tm.tm_mday = file_info.tmu_date.tm_mday;
      tm.tm_hour = file_info.tmu_date.tm_hour;
      tm.tm_min = file_info.tmu_date.tm_min;
      tm.tm_sec = file_info.tmu_date.tm_sec;
      // Sets tm.tm_isdst with -1 to have system decide whether daylight savings
      // time is in effect or not.
      tm.tm_isdst = -1;
      return mktime(&tm) * UINT64_C(1000);
    }
    return 0;
  }

  // Returns -1 on error, 0 on success, 1 on canceled.
  int EnumerateFiles(const char *dir, Slot1<bool, const char *> *callback) {
    ASSERT(dir);
    std::string dir_name(dir);
    // Make sure dir_name is ended with '/' if it is not empty to make the
    // prefix matching works for files under the directory.
    if (!dir_name.empty() && dir_name[dir_name.size() - 1] != kDirSeparator)
      dir_name += kDirSeparator;

    if (!SwitchToRead())
      return -1;

    int res = unzGoToFirstFile(unzip_handle_);
    while (res == UNZ_OK) {
      unz_file_info file_info;
      char filename[256];
      res = ggadget::UnzGetCurrentFileInfo(unzip_handle_, &file_info,
                                           filename, sizeof(filename),
                                           NULL, 0, NULL, 0);
      if (res != UNZ_OK)
        break;
      char *filename_ptr = filename;
      size_t filename_size = static_cast<size_t>(file_info.size_filename + 1);
      // In most cases filename buffer is big enough to contain the file name.
      if (filename_size > sizeof(filename)) {
        filename_ptr = new char[filename_size];
        res = ggadget::UnzGetCurrentFileInfo(unzip_handle_, &file_info,
                                             filename_ptr, filename_size,
                                             NULL, 0, NULL, 0);
        if (res != UNZ_OK)
          break;
      }
      if (filename_ptr[filename_size - 1] != kDirSeparator &&
          strcmp(filename_ptr, kZipReadMeFile) != 0 &&
          GadgetStrNCmp(dir_name.c_str(), filename_ptr, dir_name.size()) == 0 &&
          !(*callback)(filename_ptr + dir_name.size())) {
        if (filename_ptr != filename) delete [] filename_ptr;
        delete callback;
        return 1;
      }
      if (filename_ptr != filename) delete [] filename_ptr;
      res = unzGoToNextFile(unzip_handle_);
    }
    delete callback;
    return res == UNZ_OK || res == UNZ_END_OF_LIST_OF_FILE ? 0 : -1;
  }

  // Check if the given file path is valid and return the full path and
  // relative path.
  bool CheckFilePath(const char *file, std::string *relative_path,
                     std::string *full_path) {
    if (relative_path) relative_path->clear();
    if (full_path) full_path->clear();

    if (base_path_.empty()) {
      LOG("ZipFileManager hasn't been initialized.");
      return false;
    }

    // Can't read a file from an absolute path.
    // The file must be a relative path under base_path.
    if (!file || !*file || IsAbsolutePath(file)) {
      LOG("Invalid file path: %s", (file ? file : "(NULL)"));
      return false;
    }

    std::string path;
    path = BuildFilePath(base_path_.c_str(), file, NULL);
    path = NormalizeFilePath(path.c_str());

    if (full_path) *full_path = path;

    // Check if the normalized path is starting from base_path.
    if (path.length() <= base_path_.length() ||
        strncmp(base_path_.c_str(), path.c_str(), base_path_.length()) != 0 ||
        path[base_path_.length()] != kDirSeparator) {
      LOG("Invalid file path: %s", file);
      return false;
    }

    if (relative_path)
      relative_path->assign(path.begin() + base_path_.length()+1, path.end());

    return true;
  }

  bool EnsureTempDirectory() {
    if (temp_dir_.length())
      return EnsureDirectories(temp_dir_.c_str());

    if (base_path_.length()) {
      std::string path, name;
      SplitFilePath(base_path_.c_str(), &path, &name);

      if (CreateTempDirectory(name.c_str(), &path)) {
        temp_dir_ = path;
        DLOG("A temporary directory has been created: %s", path.c_str());
        return true;
      }
    }

    return false;
  }

  bool SwitchToRead() {
    if (base_path_.empty())
      return false;

    if (unzip_handle_) {
      // unzGoToFirstFile can reset error flags of the handle.
      if (unzGoToFirstFile(unzip_handle_) == UNZ_OK)
        return true;
      // The unzip handle is not usable. Reopen it.
      unzClose(unzip_handle_);
    }

    if (zip_handle_) {
      zipClose(zip_handle_, kZipGlobalComment);
      zip_handle_ = NULL;
    }

    unzip_handle_ = unzOpen(base_path_.c_str());
    if (!unzip_handle_)
      LOG("Can't open zip archive %s for reading.", base_path_.c_str());

    return unzip_handle_ != NULL;
  }

  bool SwitchToWrite() {
    if (base_path_.empty())
      return false;

    if (zip_handle_)
      return true;

    if (unzip_handle_) {
      unzClose(unzip_handle_);
      unzip_handle_ = NULL;
    }

    // If the file already exists, then try to open in append mode,
    // otherwise open in create mode.
    if (ggadget::access(base_path_.c_str(), F_OK) == 0) {
      zip_handle_ = zipOpen(base_path_.c_str(), APPEND_STATUS_ADDINZIP);
    } else {
      zip_handle_ = zipOpen(base_path_.c_str(), APPEND_STATUS_CREATE);
      if (zip_handle_)
        AddReadMeFileInZip(zip_handle_, base_path_.c_str());
    }

    if (!zip_handle_)
      LOG("Can't open zip archive %s for writing.", base_path_.c_str());

    return zip_handle_ != NULL;
  }

  bool AddFileInZip(zipFile zip, const char *zip_path,
                    const char *file, const char *data, size_t size) {
    ASSERT(zip);
    zip_fileinfo info;
    memset(&info, 0, sizeof(info));
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);  // Daylight savings time may be in effect.
    info.tmz_date.tm_sec = tm->tm_sec;
    info.tmz_date.tm_min = tm->tm_min;
    info.tmz_date.tm_hour = tm->tm_hour;
    info.tmz_date.tm_mday = tm->tm_mday;
    info.tmz_date.tm_mon = tm->tm_mon;
    info.tmz_date.tm_year = tm->tm_year + 1900;
    if (ggadget::ZipOpenNewFileInZip(zip, file, &info, NULL, 0,
                                     NULL, 0, NULL, Z_DEFLATED,
                                     Z_DEFAULT_COMPRESSION) != ZIP_OK) {
      LOG("Can't add new file %s in zip archive %s.", file, zip_path);
      return false;
    }

    int result = zipWriteInFileInZip(zip, data, static_cast<unsigned>(size));
    zipCloseFileInZip(zip);
    if (result != ZIP_OK) {
      LOG("Error when adding %s file in zip archive %s.", file, zip_path);
      return false;
    }
    return true;
  }

  // At least one file must be added to an empty zip archive, otherwise the
  // archive will become invalid and can't be opened again.
  bool AddReadMeFileInZip(zipFile zip, const char *zip_path) {
    return AddFileInZip(zip, zip_path, kZipReadMeFile,
                        kZipGlobalComment, sizeof(kZipGlobalComment) - 1);
  }

  std::string temp_dir_;
  std::string base_path_;

  unzFile unzip_handle_;
  zipFile zip_handle_;
};


ZipFileManager::ZipFileManager()
  : impl_(new Impl()){
}

ZipFileManager::~ZipFileManager() {
  delete impl_;
}

bool ZipFileManager::IsValid() {
  return impl_->IsValid();
}

bool ZipFileManager::Init(const char *base_path, bool create) {
  return impl_->Init(base_path, create);
}

bool ZipFileManager::ReadFile(const char *file, std::string *data) {
  return impl_->ReadFile(file, data);
}

bool ZipFileManager::WriteFile(const char *file, const std::string &data,
                               bool overwrite) {
  return impl_->WriteFile(file, data, overwrite);
}

bool ZipFileManager::RemoveFile(const char *file) {
  return impl_->RemoveFile(file);
}

bool ZipFileManager::ExtractFile(const char *file, std::string *into_file) {
  return impl_->ExtractFile(file, into_file);
}

bool ZipFileManager::FileExists(const char *file, std::string *path) {
  return impl_->FileExists(file, path);
}

bool ZipFileManager::IsDirectlyAccessible(const char *file,
                                          std::string *path) {
  return impl_->IsDirectlyAccessible(file, path);
}

std::string ZipFileManager::GetFullPath(const char *file) {
  return impl_->GetFullPath(file);
}

uint64_t ZipFileManager::GetLastModifiedTime(const char *file) {
  return impl_->GetLastModifiedTime(file);
}

bool ZipFileManager::EnumerateFiles(const char *dir,
                                    Slot1<bool, const char *> *callback) {
  // Errors during enumeration are ignored.
  return impl_->EnumerateFiles(dir, callback) != 1;
}

FileManagerInterface *ZipFileManager::Create(const char *base_path,
                                             bool create) {
  FileManagerInterface *fm = new ZipFileManager();
  if (fm->Init(base_path, create))
    return fm;

  delete fm;
  return NULL;
}

} // namespace ggadget
