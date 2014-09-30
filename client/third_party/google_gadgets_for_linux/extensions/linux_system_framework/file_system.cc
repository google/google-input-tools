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

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include "ggadget/gadget_consts.h"
#include "ggadget/slot.h"
#include "ggadget/string_utils.h"
#include "ggadget/system_utils.h"
#include "ggadget/xdg/utilities.h"
#include "ggadget/scoped_ptr.h"
#include "file_system.h"

namespace ggadget {
namespace framework {
namespace linux_system {

void FixCRLF(std::string *data) {
  ASSERT(data);
  size_t position = 0;
  bool in_cr = false;
  for (size_t i = 0; i < data->size(); ++i) {
    if (in_cr) {
      switch ((*data)[i]) {
        case '\n':
          (*data)[position++] = '\n';
          break;
        default:
          (*data)[position++] = '\n';
          (*data)[position++] = (*data)[i];
          break;
      }
      in_cr = false;
    } else {
      switch ((*data)[i]) {
        case '\r':
          in_cr = true;
          break;
        default:
          if (i != position) {
            (*data)[position] = (*data)[i];
          }
          ++position;
          break;
      }
    }
  }
  if (in_cr)
    (*data)[position++] = '\n';
  data->resize(position);
}

// utility function for initializing the file path
static bool InitFilePath(const char *filename,
                         std::string *base_ptr,
                         std::string *name_ptr,
                         std::string *path_ptr) {
  ASSERT(filename);
  ASSERT(*filename);
  ASSERT(base_ptr);
  ASSERT(name_ptr);
  ASSERT(path_ptr);

  *path_ptr = ggadget::GetAbsolutePath(filename);
  return !path_ptr->empty() &&
         SplitFilePath(path_ptr->c_str(), base_ptr, name_ptr);
}

// Returns normalized dest if dest is not ended with dir separator, otherwise
// returns dest/source_name.
bool NormalizeSourceAndDest(const char *source, const char *dest,
                            std::string *result_source,
                            std::string *result_dest) {
  std::string base, name;
  if (!InitFilePath(source, &base, &name, result_source))
    return false;

  char last = dest[strlen(dest) - 1];
  if (last == '\\' || last == ggadget::kDirSeparator) {
    // Copy the source under the dest dir.
    *result_dest = ggadget::GetAbsolutePath(
         ggadget::BuildFilePath(dest, name.c_str(), NULL).c_str());
  } else {
    *result_dest = ggadget::GetAbsolutePath(dest);
  }
  return !result_dest->empty();
}

static bool CopyFile(const char *source, const char *dest, bool overwrite) {
  ASSERT(source);
  ASSERT(*source);
  ASSERT(dest);
  ASSERT(*dest);

  std::string sourcefile, destfile;
  if (!NormalizeSourceAndDest(source, dest, &sourcefile, &destfile))
    return false;

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(destfile.c_str(), &stat_value) == 0) {
    if (!overwrite)
      return false;
    if (S_ISDIR(stat_value.st_mode))
      return false;
  }
  return ggadget::CopyFile(sourcefile.c_str(), destfile.c_str());
}

static bool CopyFolder(const char *source, const char *dest, bool overwrite) {
  ASSERT(source);
  ASSERT(*source);
  ASSERT(dest);
  ASSERT(*dest);

  std::string sourcedir, destdir;
  if (!NormalizeSourceAndDest(source, dest, &sourcedir, &destdir))
    return false;

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(destdir.c_str(), &stat_value) == 0) {
    if (!overwrite)
      return false;
    if (!S_ISDIR(stat_value.st_mode))
      return false;
  } else if (mkdir(destdir.c_str(), 0755) != 0) {
    return false;
  }

  if (destdir.size() > sourcedir.size() &&
      destdir[sourcedir.size()] == '/' &&
      strncmp(sourcedir.c_str(), destdir.c_str(), sourcedir.size()) == 0)
    return false;

  DIR *dir = opendir(sourcedir.c_str());
  if (dir == NULL)
    return false;

  if (sourcedir == destdir) {
    closedir(dir);
    return overwrite;
  }
  struct dirent *entry = NULL;
  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;

    memset(&stat_value, 0, sizeof(stat_value));
    std::string file =
        ggadget::BuildFilePath(sourcedir.c_str(), entry->d_name, NULL);
    if (stat(file.c_str(), &stat_value) == 0) {
      std::string dest_file =
          ggadget::BuildFilePath(destdir.c_str(), entry->d_name, NULL);
      if (S_ISDIR(stat_value.st_mode)) {
        if (!CopyFolder(file.c_str(), dest_file.c_str(), overwrite)) {
          closedir(dir);
          return false;
        }
      } else {
        if (!CopyFile(file.c_str(), dest_file.c_str(), overwrite)) {
          closedir(dir);
          return false;
        }
      }
    }
  }

  closedir(dir);
  return true;
}

// The no_use parameter eases wildcard handling. See OperateWildcard.
static bool MoveFile(const char *source, const char *dest, bool no_use) {
  GGL_UNUSED(no_use);
  ASSERT(source);
  ASSERT(*source);
  ASSERT(dest);
  ASSERT(*dest);

  std::string sourcefile, destfile;
  if (!NormalizeSourceAndDest(source, dest, &sourcefile, &destfile))
    return false;

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(sourcefile.c_str(), &stat_value) != 0 || S_ISDIR(stat_value.st_mode))
    return false;
  if (sourcefile == destfile)
    return true;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(destfile.c_str(), &stat_value) == 0)
    return false;

  if (rename(sourcefile.c_str(), destfile.c_str()) == 0)
    return true;

  // Otherwise try to copy to dest and remove source.
  return CopyFile(source, dest, false) && unlink(source) == 0;
}

// The no_use parameter eases wildcard handling. See OperateWildcard.
static bool MoveFolder(const char *source, const char *dest, bool no_use) {
  GGL_UNUSED(no_use);
  ASSERT(source);
  ASSERT(*source);
  ASSERT(dest);
  ASSERT(*dest);

  std::string sourcedir, destdir;
  if (!NormalizeSourceAndDest(source, dest, &sourcedir, &destdir))
    return false;

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(sourcedir.c_str(), &stat_value) != 0 || !S_ISDIR(stat_value.st_mode))
    return false;
  if (sourcedir == destdir)
    return true;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(destdir.c_str(), &stat_value) == 0)
    return false;

  if (rename(sourcedir.c_str(), destdir.c_str()) == 0)
    return true;

  // Otherwise try to copy to dest and remove source.
  return CopyFolder(source, dest, false) &&
         ggadget::RemoveDirectory(source, true);
}

// The no_use parameter eases wildcard handling. See OperateWildcard.
static bool DeleteFile(const char *filename, const char *no_use, bool force) {
  GGL_UNUSED(no_use);
  ASSERT(filename);
  ASSERT(*filename);

  if (!force && access(filename, W_OK) != 0)
    return false;
  return unlink(filename) == 0;
}

// The no_use parameter eases wildcard handling. See OperateWildcard.
static bool DeleteFolder(const char *filename, const char *no_use, bool force) {
  GGL_UNUSED(no_use);
  ASSERT(filename);
  ASSERT(*filename);

  return ggadget::RemoveDirectory(filename, force);
}

static bool SetName(const char *path, const char *dir, const char *name) {
  ASSERT(path);
  ASSERT(*path);
  ASSERT(name);
  ASSERT(*name);

  std::string n(name);
  if (n.find('/') != std::string::npos
      || n.find('\\') != std::string::npos)
    return false;
  std::string newpath = ggadget::BuildFilePath(dir, name, NULL);
  return rename(path, newpath.c_str()) == 0;
}

static int64_t GetFileSize(const char *filename) {
  ASSERT(filename);
  ASSERT(*filename);

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  stat(filename, &stat_value);
  return stat_value.st_size;
}

static int64_t GetFolderSize(const char *filename) {
  // Gets the init-size of folder.
  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(filename, &stat_value))
    return 0;
  int64_t size = stat_value.st_size;

  DIR *dir = NULL;
  struct dirent *entry = NULL;

  dir = opendir(filename);
  if (dir == NULL)
    return 0;

  while ((entry = readdir(dir)) != NULL) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;

    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    std::string file =
        ggadget::BuildFilePath(filename, entry->d_name, NULL);
    if (stat(file.c_str(), &stat_value) == 0) {
      if (S_ISDIR(stat_value.st_mode)) {
        // sum up the folder's size
        size += GetFolderSize(file.c_str());
      } else {
        // sum up the file's size
        size += GetFileSize(file.c_str());
      }
    }
  }

  closedir(dir);

  return size;
}

class TextStream : public TextStreamInterface {
 public:
  TextStream(int fd, IOMode mode, bool unicode)
      : fd_(fd),
        mode_(mode),
        line_(-1),
        col_(-1),
        readingptr_(0) {
    GGL_UNUSED(unicode);          
    if (fd_ != -1) {
      line_ = 1;
      col_ = 1;
    }
  }
  ~TextStream() {
    Close();
  }

  bool Init() {
    if (mode_ == IO_MODE_READING) {
      std::string raw_content;
      const size_t kChunkSize = 8192;
      char buffer[kChunkSize];
      while (true) {
        ssize_t read_size = read(fd_, buffer, kChunkSize);
        if (read_size == -1)
          return false;
        raw_content.append(buffer, static_cast<size_t>(read_size));
        if (raw_content.length() > kMaxFileSize)
          return false;
        if (static_cast<size_t>(read_size) < kChunkSize)
          break;
      }
      if (!ConvertLocaleStringToUTF8(raw_content.c_str(), &content_) &&
          !DetectAndConvertStreamToUTF8(raw_content, &content_, &encoding_)) {
        return false;
      }
      FixCRLF(&content_);
    }
    return true;
  }

  virtual void Destroy() { delete this; }

 public:
  virtual int GetLine() {
    return line_;
  }
  virtual int GetColumn() {
    return col_;
  }

  virtual bool IsAtEndOfStream() {
    // FIXME: should throw exception in this situation.
    if (mode_ != IO_MODE_READING)
      return true;
    return readingptr_ >= content_.size();
  }
  virtual bool IsAtEndOfLine() {
    // FIXME: should throw exception in this situation.
    if (mode_ != IO_MODE_READING)
      return true;
    return content_[readingptr_] == '\n';
  }
  virtual bool Read(int characters, std::string *result) {
    if (mode_ != IO_MODE_READING || !result)
      return false;

    size_t size = GetUTF8CharsLength(&content_[readingptr_],
                                     characters,
                                     content_.size() - readingptr_);
    *result = content_.substr(readingptr_, size);
    readingptr_ += size;
    UpdatePosition(*result);
    return true;
  }

  virtual bool ReadLine(std::string *result) {
    if (mode_ != IO_MODE_READING || !result)
      return false;

    std::string::size_type position = content_.find('\n', readingptr_);
    if (position == std::string::npos) {
      *result = content_.substr(readingptr_);
      readingptr_ = content_.size();
      UpdatePosition(*result);
    } else {
      *result = content_.substr(readingptr_, position - readingptr_);
      readingptr_ = position + 1;
      col_ = 1;
      ++line_;
    }
    return true;
  }
  virtual bool ReadAll(std::string *result) {
    if (mode_ != IO_MODE_READING || !result)
      return false;

    *result = content_.substr(readingptr_);
    readingptr_ = content_.size();
    UpdatePosition(*result);
    return true;
  }

  virtual bool Write(const std::string &text) {
    if (mode_ == IO_MODE_READING)
      return false;

    std::string copy = text;
    FixCRLF(&copy);
    bool result = WriteString(copy);
    UpdatePosition(copy);
    return result;
  }
  virtual bool WriteLine(const std::string &text) {
    if (mode_ == IO_MODE_READING)
      return false;

    return Write(text) && Write("\n");
  }
  virtual bool WriteBlankLines(int lines) {
    if (mode_ == IO_MODE_READING)
      return false;

    for (int i = 0; i < lines; ++i)
      if (!Write("\n")) return false;

    return true;
  }

  virtual bool Skip(int characters) {
    if (mode_ != IO_MODE_READING)
      return false;
    std::string data;
    return Read(characters, &data);
  }
  virtual bool SkipLine() {
    if (mode_ != IO_MODE_READING)
      return false;
    std::string data;
    return ReadLine(&data);
  }

  virtual void Close() {
    if (fd_ == -1)
      return;
    if (fd_ > STDERR_FILENO) {
      close(fd_);
    }
    fd_ = -1;
  }

 private:
  void UpdatePosition(const std::string &character) {
    size_t position = 0;
    while (position < character.size()) {
      if (character[position] == '\n') {
        col_ = 1;
        ++line_;
        ++position;
      } else {
        position += GetUTF8CharLength(&character[position]);
        ++col_;
      }
    }
  }

  bool WriteString(const std::string &data) {
    std::string buffer;
    if (ConvertUTF8ToLocaleString(data.c_str(), &buffer)) {
      return write(fd_, buffer.c_str(), buffer.size()) ==
          static_cast<ssize_t>(buffer.size());
    }
    return false;
  }

 private:
  int fd_;
  IOMode mode_;
  int line_;
  int col_;
  std::string content_;
  std::string encoding_;
  size_t readingptr_;
};

class BinaryStream : public BinaryStreamInterface {
 public:
  BinaryStream(int fd, IOMode mode)
      : fd_(fd), mode_(mode), size_(0), pos_(0) {
  }
  ~BinaryStream() { Close(); }

  bool Init() {
    size_ = lseek(fd_, 0, SEEK_END);
    pos_ = lseek(fd_, 0, SEEK_SET);
    return (size_ != -1 && pos_ != -1);
  }

  virtual void Destroy() { delete this; }

  virtual int64_t GetPosition() {
    return static_cast<int64_t>(pos_);
  }

  virtual bool IsAtEndOfStream() {
    return pos_ >= size_;
  }

  virtual bool Read(int64_t bytes, std::string *result) {
    if (mode_ != IO_MODE_READING || !result ||
        bytes < 0 || bytes > static_cast<int64_t>(kMaxFileSize))
      return false;

    result->reserve(static_cast<size_t>(bytes));
    result->resize(static_cast<size_t>(bytes));
    ssize_t read_bytes = read(fd_, const_cast<char *>(result->c_str()),
                              static_cast<size_t>(bytes));
    if (read_bytes == -1) {
      *result = std::string();
      // Return to current position.
      lseek(fd_, pos_, SEEK_SET);
      return false;
    }

    result->resize(static_cast<size_t>(read_bytes));
    pos_ = lseek(fd_, 0, SEEK_CUR);
    return true;
  }

  virtual bool ReadAll(std::string *result) {
    if (mode_ != IO_MODE_READING || !result)
      return false;

    return Read(size_ - pos_, result);
  }

  virtual bool Write(const std::string &data) {
    if (mode_ == IO_MODE_READING)
      return false;

    size_t written_bytes = 0;
    size_t data_size = data.size();
    const char *data_ptr = data.c_str();
    while (written_bytes < data_size) {
      ssize_t write_bytes = write(fd_, data_ptr, data_size - written_bytes);
      if (write_bytes == -1) {
        lseek(fd_, pos_, SEEK_SET);
        return false;
      }
      written_bytes += write_bytes;
      data_ptr += write_bytes;
    }

    pos_ = lseek(fd_, 0, SEEK_CUR);
    size_ = lseek(fd_, 0, SEEK_END);
    lseek(fd_, pos_, SEEK_SET);
    return true;
  }

  virtual bool Skip(int64_t bytes) {
    if (mode_ != IO_MODE_READING)
      return false;

    if (bytes + pos_ > size_)
      pos_ = lseek(fd_, size_, SEEK_SET);
    else
      pos_ = lseek(fd_, static_cast<off_t>(bytes), SEEK_CUR);

    return pos_ != -1;
  }

  virtual void Close() {
    if (fd_ == -1)
      return;
    if (fd_ > STDERR_FILENO) {
      close(fd_);
    }
    fd_ = -1;
  }

 private:
  int fd_;
  IOMode mode_;
  off_t size_;
  off_t pos_;
};

// Returns fd of the opened file.
static int OpenFile(const char *filename, IOMode mode, bool create,
                    bool overwrite) {
  ASSERT(filename && *filename);
  int flags = 0;

  switch (mode) {
    case IO_MODE_READING:
      flags = O_RDONLY;
      break;
    case IO_MODE_APPENDING:
      flags = O_APPEND | O_WRONLY;
      break;
    case IO_MODE_WRITING:
      flags = O_TRUNC | O_WRONLY;
      break;
    default:
      ASSERT(false);
      break;
  }

  if (create) {
    flags |= O_CREAT;
  }

  if (!overwrite) {
    flags |= O_EXCL;
  }

  return open(filename, flags, S_IRUSR | S_IWUSR);
}

static TextStreamInterface *OpenTextFile(const char *filename,
                                         IOMode mode,
                                         bool create,
                                         bool overwrite,
                                         Tristate format) {
  int fd = OpenFile(filename, mode, create, overwrite);
  if (fd == -1)
    return NULL;

  TextStream *stream = new TextStream(fd, mode, format == TRISTATE_TRUE);
  if (stream->Init())
    return stream;
  stream->Destroy();
  return NULL;
}

static BinaryStreamInterface *OpenBinaryFile(const char *filename,
                                             IOMode mode,
                                             bool create,
                                             bool overwrite) {
  int fd = OpenFile(filename, mode, create, overwrite);
  if (fd == -1)
    return NULL;

  BinaryStream *stream = new BinaryStream(fd, mode);
  if (stream->Init())
    return stream;
  stream->Destroy();
  return NULL;
}

/**
 * Gets the attributes of a given file or directory.
 * @param path the full path of the file or directory.
 * @param base the base name of the file or directory, that is, the last part
 *     of the full path.  For example, the base of "/path/to/file" is "file".
 */
static FileAttribute GetAttributes(const char *path, const char *base) {
  ASSERT(path);
  ASSERT(*path);
  ASSERT(base);
  ASSERT(*base);

  // FIXME: Check whether path specifies a file or a folder.

  FileAttribute attribute = FILE_ATTR_NORMAL;

  if (base[0] == '.') {
    attribute = static_cast<FileAttribute>(attribute | FILE_ATTR_HIDDEN);
  }

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(path, &stat_value) == -1) {
    return attribute;
  }

  mode_t mode = stat_value.st_mode;

  if (S_ISLNK(mode)) {
    // it is a symbolic link
    attribute = static_cast<FileAttribute>(attribute | FILE_ATTR_ALIAS);
  }

  if (!(mode & S_IWUSR) && (mode & S_IRUSR)) {
    // it is read only by owner
    attribute = static_cast<FileAttribute>(attribute | FILE_ATTR_READONLY);
  }

  return attribute;
}

static bool SetAttributes(const char *filename,
                          FileAttribute attributes) {
  ASSERT(filename);
  ASSERT(*filename);

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(filename, &stat_value) == -1) {
    return false;
  }

  mode_t mode = stat_value.st_mode;
  bool should_change = false;

  // only accept FILE_ATTR_READONLY.
  if ((attributes & FILE_ATTR_READONLY) != 0 &&
      (mode & FILE_ATTR_READONLY) == 0) {
    // modify all the attributes to read only
    mode = static_cast<FileAttribute>((mode | S_IRUSR) & ~S_IWUSR);
    mode = static_cast<FileAttribute>((mode | S_IRGRP) & ~S_IWGRP);
    mode = static_cast<FileAttribute>((mode | S_IROTH) & ~S_IWOTH);
    should_change = true;
  } else if ((attributes & FILE_ATTR_READONLY) == 0 &&
             (mode & FILE_ATTR_READONLY) != 0) {
    mode = static_cast<FileAttribute>(mode | S_IRUSR | S_IWUSR);
    should_change = true;
  }

  if (should_change)
    return chmod(filename, mode) == 0;

  return true;
}

static Date GetDateLastModified(const char *path) {
  ASSERT(path);
  ASSERT(*path);

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(path, &stat_value))
    return Date(0);

  return Date(stat_value.st_mtim.tv_sec * 1000
              + stat_value.st_mtim.tv_nsec / 1000000);
}

static Date GetDateLastAccessed(const char *path) {
  ASSERT(path);
  ASSERT(*path);

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(path, &stat_value))
    return Date(0);

  return Date(stat_value.st_atim.tv_sec * 1000
              + stat_value.st_atim.tv_nsec / 1000000);
}

class Drive : public DriveInterface {
 public:
  Drive() { }

 public:
  virtual void Destroy() {
    // Deliberately does nothing.
  }

  virtual std::string GetPath() {
    return "/";
  }

  virtual std::string GetDriveLetter() {
    return "";
  }

  virtual std::string GetShareName() {
    // TODO: implement this.
    return "";
  }

  virtual DriveType GetDriveType() {
    // TODO: implement this.
    return DRIVE_TYPE_UNKNOWN;
  }

  virtual FolderInterface *GetRootFolder();

  virtual int64_t GetAvailableSpace() {
    // TODO: implement this.
    return 0;
  }

  virtual int64_t GetFreeSpace() {
    // TODO: implement this.
    return 0;
  }

  virtual int64_t GetTotalSize() {
    // TODO: implement this.
    return 0;
  }

  virtual std::string GetVolumnName() {
    // TODO: implement this.
    return "";
  }

  virtual bool SetVolumnName(const char *name) {
    GGL_UNUSED(name);
    // TODO: implement this.
    return false;
  }

  virtual std::string GetFileSystem() {
    // TODO: implement this.
    return "";
  }

  virtual int64_t GetSerialNumber() {
    // TODO: implement this.
    return 0;
  }

  virtual bool IsReady() {
    return true;
  }
};

static Drive root_drive;

// Drives object simulates a collection contains only one "root" drive.
class Drives : public DrivesInterface {
 public:
  Drives() : at_end_(false) { }
  virtual void Destroy() { }

 public:
  virtual int GetCount() const { return 1; }
  virtual bool AtEnd() { return at_end_; }
  virtual DriveInterface *GetItem() { return at_end_ ? NULL : &root_drive; }
  virtual void MoveFirst() { at_end_ = false; }
  virtual void MoveNext() { at_end_ = true; }

 private:
  bool at_end_;
};

class File : public FileInterface {
 public:
  explicit File(const char *filename) {
    ASSERT(filename);
    ASSERT(*filename);

    InitFilePath(filename, &base_, &name_, &path_);
    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    if (stat(path_.c_str(), &stat_value))
      path_.clear();
    if (S_ISDIR(stat_value.st_mode))
      // it is not a directory
      path_.clear();
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    return path_;
  }

  virtual std::string GetName() {
    return name_;
  }

  virtual bool SetName(const char *name) {
    if (!name || !*name)
      return false;
    if (path_.empty())
      return false;
    if (!strcmp(name, name_.c_str()))
      return true;
    if (linux_system::SetName(path_.c_str(), base_.c_str(), name)) {
      path_ = ggadget::BuildFilePath(base_.c_str(), name, NULL);
      InitFilePath(path_.c_str(), &base_, &name_, &path_);
      return true;
    }
    return false;
  }

  virtual std::string GetShortPath() {
    return GetPath();
  }

  virtual std::string GetShortName() {
    return GetName();
  }

  virtual DriveInterface *GetDrive() {
    return &root_drive;
  }

  virtual FolderInterface *GetParentFolder();

  virtual FileAttribute GetAttributes() {
    if (path_.empty())
      return FILE_ATTR_NORMAL;
    return linux_system::GetAttributes(path_.c_str(), name_.c_str());
  }

  virtual bool SetAttributes(FileAttribute attributes) {
    if (path_.empty())
      return false;
    return linux_system::SetAttributes(path_.c_str(), attributes);
  }

  virtual Date GetDateCreated() {
    // can't determine the created date under linux os
    return Date(0);
  }

  virtual Date GetDateLastModified() {
    if (path_.empty())
      return Date(0);
    return linux_system::GetDateLastModified(path_.c_str());
  }

  virtual Date GetDateLastAccessed() {
    if (path_.empty())
      return Date(0);
    return linux_system::GetDateLastAccessed(path_.c_str());
  }

  virtual int64_t GetSize() {
    if (path_.empty())
      return 0;
    return linux_system::GetFileSize(path_.c_str());
  }

  virtual std::string GetType() {
    if (path_.empty())
      return "";
    return ggadget::xdg::GetFileMimeType(path_.c_str());
  }

  virtual bool Delete(bool force) {
    if (path_.empty())
      return false;
    bool result = linux_system::DeleteFile(path_.c_str(), "no_use", force);
    if (result)
      path_.clear();
    return result;
  }

  virtual bool Copy(const char *dest, bool overwrite) {
    if (path_.empty())
      return false;
    if (!dest || !*dest)
      return false;
    return linux_system::CopyFile(path_.c_str(), dest, overwrite);
  }

  virtual bool Move(const char *dest) {
    if (path_.empty())
      return false;
    if (!dest || !*dest)
      return false;
    bool result = linux_system::MoveFile(path_.c_str(), dest, false);
    if (result) {
      std::string path = ggadget::GetAbsolutePath(dest);
      InitFilePath(path.c_str(), &base_, &name_, &path_);
    }
    return result;
  }

  virtual TextStreamInterface *OpenAsTextStream(IOMode mode,
                                                Tristate format) {
    if (path_.empty())
      return NULL;
    return linux_system::OpenTextFile(path_.c_str(), mode, false, true, format);
  }

  virtual BinaryStreamInterface *OpenAsBinaryStream(IOMode mode) {
    if (path_.empty())
      return NULL;
    return linux_system::OpenBinaryFile(path_.c_str(), mode, false, true);
  }

 private:
  std::string path_;
  std::string base_;
  std::string name_;
};

class Files : public FilesInterface {
 public:
  Files(const char *path)
      : path_(path),
        dir_(NULL),
        at_end_(true) {
  }

  ~Files() {
    if (dir_)
      closedir(dir_);
  }

  bool Init() {
    if (dir_)
      closedir(dir_);
    dir_ = opendir(path_.c_str());
    if (dir_ == NULL) {
      return errno == EACCES;
    }
    at_end_ = false;
    MoveNext();
    return true;
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual int GetCount() const {
    int count = 0;
    DIR *dir = opendir(path_.c_str());
    if (dir == NULL)
        return 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;
      struct stat stat_value;
      memset(&stat_value, 0, sizeof(stat_value));
      std::string filename =
          ggadget::BuildFilePath(path_.c_str(), entry->d_name, NULL);
      if (stat(filename.c_str(), &stat_value) == 0) {
        if (!S_ISDIR(stat_value.st_mode))
          ++count;
      }
    }
    closedir(dir);
    return count;
  }

  virtual bool AtEnd() {
    return at_end_;
  }

  virtual FileInterface *GetItem() {
    if (current_file_.empty())
      return NULL;
    return new File(current_file_.c_str());
  }

  virtual void MoveFirst() {
    Init();
  }

  virtual void MoveNext() {
    if (dir_ == NULL)
        return;
    struct dirent *entry;
    while ((entry = readdir(dir_)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;
      struct stat stat_value;
      memset(&stat_value, 0, sizeof(stat_value));
      std::string filename =
          ggadget::BuildFilePath(path_.c_str(), entry->d_name, NULL);
      if (stat(filename.c_str(), &stat_value) == 0) {
        if (!S_ISDIR(stat_value.st_mode)) {
          current_file_ = filename;
          return;
        }
      }
    }
    at_end_ = true;
    return;
  }

 private:
  std::string path_;
  DIR *dir_;
  bool at_end_;
  std::string current_file_;
};

class Folders : public FoldersInterface {
 public:
  Folders(const char *path)
      : path_(path),
        dir_(NULL),
        at_end_(true) {
  }

  ~Folders() {
    if (dir_)
      closedir(dir_);
  }

  bool Init() {
    if (dir_)
      closedir(dir_);
    dir_ = opendir(path_.c_str());
    if (dir_ == NULL) {
      return errno == EACCES;
    }
    at_end_ = false;
    MoveNext();
    return true;
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual int GetCount() const {
    int count = 0;
    DIR *dir = opendir(path_.c_str());
    if (dir == NULL)
        return 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;
      struct stat stat_value;
      memset(&stat_value, 0, sizeof(stat_value));
      std::string filename =
          ggadget::BuildFilePath(path_.c_str(), entry->d_name, NULL);
      if (stat(filename.c_str(), &stat_value) == 0) {
        if (S_ISDIR(stat_value.st_mode))
          ++count;
      }
    }
    closedir(dir);
    return count;
  }

  virtual bool AtEnd() {
    return at_end_;
  }

  virtual FolderInterface *GetItem();

  virtual void MoveFirst() {
    Init();
  }

  virtual void MoveNext() {
    if (dir_ == NULL)
        return;
    struct dirent *entry;
    while ((entry = readdir(dir_)) != NULL) {
      if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
        continue;
      struct stat stat_value;
      memset(&stat_value, 0, sizeof(stat_value));
      std::string folder =
          ggadget::BuildFilePath(path_.c_str(), entry->d_name, NULL);
      if (stat(folder.c_str(), &stat_value) == 0) {
        if (S_ISDIR(stat_value.st_mode)) {
          current_folder_ = folder;
          return;
        }
      }
    }
    at_end_ = true;
    return;
  }

 private:
  std::string path_;
  DIR *dir_;
  bool at_end_;
  std::string current_folder_;
};

class Folder : public FolderInterface {
 public:
  explicit Folder(const char *filename) {
    ASSERT(filename);
    ASSERT(*filename);

    InitFilePath(filename, &base_, &name_, &path_);
    struct stat stat_value;
    memset(&stat_value, 0, sizeof(stat_value));
    if (stat(path_.c_str(), &stat_value))
      path_.clear();
    if (!S_ISDIR(stat_value.st_mode))
      // it is not a directory
      path_.clear();
  }

  virtual void Destroy() {
    delete this;
  }

 public:
  virtual std::string GetPath() {
    return path_;
  }

  virtual std::string GetName() {
    return name_;
  }

  virtual bool SetName(const char *name) {
    if (!name || !*name)
      return false;
    if (path_.empty())
      return false;
    if (strcmp(name, name_.c_str()) == 0)
      return true;
    if (linux_system::SetName(path_.c_str(), base_.c_str(), name)) {
      path_ = ggadget::BuildFilePath(base_.c_str(), name, NULL);
      InitFilePath(path_.c_str(), &base_, &name_, &path_);
      return true;
    }
    return false;
  }

  virtual std::string GetShortPath() {
    return GetPath();
  }

  virtual std::string GetShortName() {
    return GetName();
  }

  virtual DriveInterface *GetDrive() {
    return NULL;
  }

  virtual FolderInterface *GetParentFolder() {
    if (path_.empty())
      return NULL;
    return new Folder(base_.c_str());
  }

  virtual FileAttribute GetAttributes() {
    if (path_.empty())
      return FILE_ATTR_DIRECTORY;
    return linux_system::GetAttributes(path_.c_str(), name_.c_str());
  }

  virtual bool SetAttributes(FileAttribute attributes) {
    if (path_.empty())
      return false;
    return linux_system::SetAttributes(path_.c_str(), attributes);
  }

  virtual Date GetDateCreated() {
    // can't determine the created date under linux os
    return Date(0);
  }

  virtual Date GetDateLastModified() {
    if (path_.empty())
      return Date(0);
    return linux_system::GetDateLastModified(path_.c_str());
  }

  virtual Date GetDateLastAccessed() {
    if (path_.empty())
      return Date(0);
    return linux_system::GetDateLastAccessed(path_.c_str());
  }

  virtual std::string GetType() {
    if (path_.empty())
      return "";
    return ggadget::xdg::GetFileMimeType(path_.c_str());
  }

  virtual bool Delete(bool force) {
    if (path_.empty())
      return false;
    return linux_system::DeleteFolder(path_.c_str(), "no_use", force);
  }

  virtual bool Copy(const char *dest, bool overwrite) {
    if (path_.empty())
      return false;
    if (!dest || !*dest)
      return false;
    return linux_system::CopyFolder(path_.c_str(), dest, overwrite);
  }

  virtual bool Move(const char *dest) {
    if (path_.empty())
      return false;
    if (!dest || !*dest)
      return false;
    bool result = linux_system::MoveFolder(path_.c_str(), dest, false);
    if (result) {
      std::string path = ggadget::GetAbsolutePath(dest);
      InitFilePath(path.c_str(), &base_, &name_, &path_);
    }
    return result;
  }

  virtual bool IsRootFolder() {
    return path_ == "/";
  }

  /** Sum of files and subfolders. */
  virtual int64_t GetSize() {
    if (path_.empty())
      return 0;
    return linux_system::GetFolderSize(path_.c_str());
  }

  virtual FoldersInterface *GetSubFolders() {
    if (path_.empty())
      return NULL;

    // creates the Folders instance
    Folders* folders_ptr = new Folders(path_.c_str());
    if (folders_ptr->Init())
      return folders_ptr;
    folders_ptr->Destroy();
    return NULL;
  }

  virtual FilesInterface *GetFiles() {
    if (path_.empty())
      return NULL;

    // Creates the Files instance.
    Files* files_ptr = new Files(path_.c_str());
    if (files_ptr->Init())
      return files_ptr;
    files_ptr->Destroy();
    return NULL;
  }

  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite, bool unicode) {
    if (!filename || !*filename)
      return NULL;
    if (path_.empty())
      return NULL;

    std::string str_path(ggadget::NormalizeFilePath(filename));
    std::string file;

    if (ggadget::IsAbsolutePath(str_path.c_str())) {
      // indicates filename is already the absolute path
      file = str_path;
    } else {
      // if not, generate the absolute path
      file = ggadget::BuildFilePath(path_.c_str(), str_path.c_str(), NULL);
    }
    return linux_system::OpenTextFile(file.c_str(), IO_MODE_WRITING,
                                      true, overwrite,
                                      unicode ? TRISTATE_TRUE : TRISTATE_FALSE);
  }

  virtual BinaryStreamInterface *CreateBinaryFile(const char *filename,
                                                  bool overwrite) {
    if (!filename || !*filename)
      return NULL;
    if (path_.empty())
      return NULL;

    std::string str_path(ggadget::NormalizeFilePath(filename));
    std::string file;

    if (ggadget::IsAbsolutePath(str_path.c_str())) {
      // indicates filename is already the absolute path
      file = str_path;
    } else {
      // if not, generate the absolute path
      file = ggadget::BuildFilePath(path_.c_str(), str_path.c_str(), NULL);
    }
    return linux_system::OpenBinaryFile(file.c_str(), IO_MODE_WRITING,
                                        true, overwrite);
  }

 private:
  std::string path_;
  std::string base_;
  std::string name_;
};

FolderInterface *Drive::GetRootFolder() {
  return new Folder("/");
}

FolderInterface *Folders::GetItem() {
  if (current_folder_.empty())
    return NULL;
  return new Folder(current_folder_.c_str());
}

// Implementation of FileSystem
FileSystem::FileSystem() {
}

FileSystem::~FileSystem() {
}

DrivesInterface *FileSystem::GetDrives() {
  return new Drives();
}

std::string FileSystem::BuildPath(const char *path, const char *name) {
  if (!path || !*path)
    return "";
  // Don't need to check name for NULL or empty string.
  return ggadget::BuildFilePath(path, name, NULL);
}

std::string FileSystem::GetDriveName(const char *path) {
  GGL_UNUSED(path);
  return "";
}

std::string FileSystem::GetParentFolderName(const char *path) {
  if (!path || !*path)
    return "";
  std::string base, name, realpath;
  InitFilePath(path, &base, &name, &realpath);
  // Returns "" for root directory.
  if (realpath == "/")
    return "";
  // Removes the trailing slash from the path.
  if (base.size() > 1 && base[base.size() - 1] == '/')
    base.resize(base.size() - 1);
  return base;
}

std::string FileSystem::GetFileName(const char *path) {
  if (!path || !*path)
    return "";
  std::string base, name, realpath;
  InitFilePath(path, &base, &name, &realpath);
  // Returns "" for root directory.
  if (realpath == "/")
    return "";
  return name;
}

std::string FileSystem::GetBaseName(const char *path) {
  if (!path || !*path)
    return "";
  std::string base, name, realpath;
  InitFilePath(path, &base, &name, &realpath);
  size_t end_index = name.find_last_of('.');
  if (end_index == std::string::npos)
    return name;
  return name.substr(0, end_index);
}

std::string FileSystem::GetExtensionName(const char *path) {
  if (!path || !*path)
    return "";
  std::string base, name, realpath;
  InitFilePath(path, &base, &name, &realpath);
  size_t end_index = name.find_last_of('.');
  if (end_index == std::string::npos)
    return "";
  return name.substr(end_index + 1);
}

// creates the character for filename
static char GetFileChar() {
  while (1) {
    char ch = static_cast<char>(random() % 123);
    if (ch == '_' ||
        ch == '.' ||
        ch == '-' ||
        ('A' <= ch && ch <= 'Z') ||
        ('a' <= ch && ch <= 'z'))
      return ch;
  }
}

std::string FileSystem::GetAbsolutePathName(const char *path) {
  return ggadget::GetAbsolutePath(path);
}

// FIXME: Use system timer to generate a more unique filename.
std::string FileSystem::GetTempName() {
  // Typically, however, file names only use alphanumeric characters(mostly
  // lower case), underscores, hyphens and periods. Other characters, such as
  // dollar signs, percentage signs and brackets, have special meanings to the
  // shell and can be distracting to work with. File names should never begin
  // with a hyphen.
  char filename[9] = {0};
  char character = 0;
  while((character = GetFileChar()) != '-')
    filename[0] = character;
  for (int i = 1; i < 8; ++i)
    filename[i] = GetFileChar();
  return std::string(filename) + ".tmp";
}

bool FileSystem::DriveExists(const char *drive_spec) {
  GGL_UNUSED(drive_spec);
  return false;
}

bool FileSystem::FileExists(const char *file_spec) {
  if (!file_spec || !*file_spec)
    return false;

  std::string str_path(ggadget::NormalizeFilePath(file_spec));
  if (access(str_path.c_str(), F_OK))
    return false;

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(str_path.c_str(), &stat_value))
    return false;

  if (S_ISDIR(stat_value.st_mode))
    // it is a directory
    return false;

  return true;
}

bool FileSystem::FolderExists(const char *folder_spec) {
  if (!folder_spec || !*folder_spec)
    return false;

  std::string str_path(ggadget::NormalizeFilePath(folder_spec));
  if (access(str_path.c_str(), F_OK))
    return false;

  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(str_path.c_str(), &stat_value))
    return false;
  if (!S_ISDIR(stat_value.st_mode))
    // it is not a directory
    return false;

  return true;
}

DriveInterface *FileSystem::GetDrive(const char *drive_spec) {
  GGL_UNUSED(drive_spec);
  return NULL;
}

// note: if file not exists, return NULL
FileInterface *FileSystem::GetFile(const char *file_path) {
  if (!FileExists(file_path))
    return NULL;
  return new File(file_path);
}

FolderInterface *FileSystem::GetFolder(const char *folder_path) {
  if (!FolderExists(folder_path))
    return NULL;
  return new Folder(folder_path);
}

FolderInterface *FileSystem::GetSpecialFolder(SpecialFolder special_folder) {
  switch (special_folder) {
  case SPECIAL_FOLDER_WINDOWS:
    return new Folder("/");
  case SPECIAL_FOLDER_SYSTEM:
    return new Folder("/");
  case SPECIAL_FOLDER_TEMPORARY:
    return new Folder("/tmp");
  default:
    break;
  }
  return new Folder("/tmp");
}

static bool OperateWildcard(
    const char *source, const char *dest, bool bool_param,
    bool (*operation)(const char *, const char *, bool)) {
  if (!source || !*source || !dest || !*dest)
    return false;

  if (!strchr(source, '*') && !strchr(source, '?'))
    return operation(source, dest, bool_param);

  // If source contains wildcard, the dest must be a directory.
  // Microsoft FileSystemObject requires that the target directory name must
  // be ended with a directory separator.
  std::string dest_str(dest);
  dest_str += ggadget::kDirSeparator;
  glob_t globbuf;
  if (glob(source,
           // GLOB_NOESCAPE because we have treated '\'s differently.
           GLOB_NOSORT | GLOB_NOCHECK | GLOB_NOESCAPE,
           NULL, &globbuf) != 0) {
    globfree(&globbuf);
    return false;
  }

  for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
    if (!operation(globbuf.gl_pathv[i], dest_str.c_str(), bool_param)) {
      globfree(&globbuf);
      return false;
    }
  }
  globfree(&globbuf);
  return true;
}

bool FileSystem::DeleteFile(const char *file_spec, bool force) {
  return OperateWildcard(file_spec, "no_use", force, linux_system::DeleteFile);
}

bool FileSystem::DeleteFolder(const char *folder_spec, bool force) {
  return OperateWildcard(folder_spec, "no_use", force,
                         linux_system::DeleteFolder);
}

bool FileSystem::MoveFile(const char *source, const char *dest) {
  return OperateWildcard(source, dest, false, linux_system::MoveFile);
}

bool FileSystem::MoveFolder(const char *source, const char *dest) {
  return OperateWildcard(source, dest, false, linux_system::MoveFolder);
}

bool FileSystem::CopyFile(const char *source, const char *dest,
                          bool overwrite) {
  return OperateWildcard(source, dest, overwrite, linux_system::CopyFile);
}

bool FileSystem::CopyFolder(const char *source, const char *dest,
                            bool overwrite) {
  return OperateWildcard(source, dest, overwrite, linux_system::CopyFolder);
}

FolderInterface *FileSystem::CreateFolder(const char *path) {
  if (!path || !*path)
    return NULL;
  std::string str_path(ggadget::NormalizeFilePath(path));
  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (stat(str_path.c_str(), &stat_value) == 0)
    return NULL; // File or directory existed.
  if (mkdir(str_path.c_str(), 0755) == 0)
    return new Folder(str_path.c_str());
  return NULL;
}

TextStreamInterface *FileSystem::CreateTextFile(const char *filename,
                                                bool overwrite,
                                                bool unicode) {
  if (filename == NULL || !*filename)
    return NULL;
  return linux_system::OpenTextFile(filename, IO_MODE_WRITING, true, overwrite,
                                    unicode ? TRISTATE_TRUE : TRISTATE_FALSE);
}

TextStreamInterface *FileSystem::OpenTextFile(const char *filename,
                                              IOMode mode,
                                              bool create,
                                              Tristate format) {
  if (filename == NULL || !*filename)
    return NULL;
  return linux_system::OpenTextFile(filename, mode, create, true, format);
}

BinaryStreamInterface *FileSystem::CreateBinaryFile(const char *filename,
                                                    bool overwrite) {
  if (filename == NULL || !*filename)
    return NULL;
  return linux_system::OpenBinaryFile(filename, IO_MODE_WRITING,
                                      true, overwrite);
}

BinaryStreamInterface *FileSystem::OpenBinaryFile(const char *filename,
                                                  IOMode mode,
                                                  bool create) {
  if (filename == NULL || !*filename)
    return NULL;
  return linux_system::OpenBinaryFile(filename, mode, create, true);
}

TextStreamInterface *
FileSystem::GetStandardStream(StandardStreamType type, bool unicode) {
  TextStream *stream = NULL;
  switch (type) {
  case STD_STREAM_IN:
    stream = new TextStream(STDIN_FILENO, IO_MODE_READING, unicode);
    break;
  case STD_STREAM_OUT:
    stream = new TextStream(STDOUT_FILENO, IO_MODE_WRITING, unicode);
    break;
  case STD_STREAM_ERR:
    stream = new TextStream(STDERR_FILENO, IO_MODE_WRITING, unicode);
  default:
    return NULL;
  }
  if (!stream->Init()) {
    stream->Destroy();
    return NULL;
  }
  return stream;
}

std::string FileSystem::GetFileVersion(const char *filename) {
  GGL_UNUSED(filename);
  return "";
}

FolderInterface *File::GetParentFolder() {
  if (path_.empty())
    return NULL;
  return new Folder(base_.c_str());
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
