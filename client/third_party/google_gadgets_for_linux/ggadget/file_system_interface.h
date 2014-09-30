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

#ifndef GGADGET_FILE_SYSTEM_INTERFACE_H__
#define GGADGET_FILE_SYSTEM_INTERFACE_H__

#include <ggadget/common.h>
#include <ggadget/variant.h>
#include <ggadget/small_object.h>

#if defined(OS_WIN)
#ifdef GetFreeSpace
#undef GetFreeSpace
#endif
#endif

namespace ggadget {

namespace framework {

class DrivesInterface;
class DriveInterface;
class FilesInterface;
class FileInterface;
class FoldersInterface;
class FolderInterface;
class TextStreamInterface;
class BinaryStreamInterface;

/**
 * @defgroup FileSystemInterfaces FileSystem Interfaces
 * @ingroup FrameworkInterfaces
 * @{
 */

enum IOMode {
  IO_MODE_READING = 1,
  IO_MODE_WRITING = 2,
  IO_MODE_APPENDING = 8
};

enum Tristate {
  TRISTATE_USE_DEFAULT = -2,
  TRISTATE_MIXED = -2,
  TRISTATE_TRUE = -1,
  TRISTATE_FALSE = 0,
};

enum FileAttribute {
  FILE_ATTR_NORMAL = 0,
  FILE_ATTR_READONLY = 1,
  FILE_ATTR_HIDDEN = 2,
  FILE_ATTR_SYSTEM = 4,
  FILE_ATTR_VOLUME = 8,
  FILE_ATTR_DIRECTORY = 16,
  FILE_ATTR_ARCHIVE = 32,
  FILE_ATTR_ALIAS = 1024,
  FILE_ATTR_COMPRESSED = 2048
};

enum SpecialFolder {
  SPECIAL_FOLDER_WINDOWS = 0,
  SPECIAL_FOLDER_SYSTEM = 1,
  SPECIAL_FOLDER_TEMPORARY = 2
};

enum StandardStreamType {
  STD_STREAM_IN = 0,
  STD_STREAM_OUT = 1,
  STD_STREAM_ERR = 2
};

enum DriveType {
  DRIVE_TYPE_UNKNOWN = 0,
  DRIVE_TYPE_REMOVABLE = 1,
  DRIVE_TYPE_FIXED = 2,
  DRIVE_TYPE_REMOTE = 3,
  DRIVE_TYPE_CDROM = 4,
  DRIVE_TYPE_RAM_DISK = 5
};

/**
 * Simulates the Microsoft IFileSystem3 interface.
 * Used for framework.filesystem.
 *
 * NOTE: if a method returns <code>const char *</code>, the pointer must be
 * used transiently or made a copy. The pointer may become invalid after
 * another call to a method also returns <code>const char *</code> in some
 * implementations.
 */
class FileSystemInterface : public SmallObject<> {
 public:
  virtual ~FileSystemInterface() { }

 public:
  /** Get drives collection. */
  virtual DrivesInterface *GetDrives() = 0;
  /** Generate a path from an existing path and a name. */
  virtual std::string BuildPath(const char *path, const char *name) = 0;
  /** Return drive from a path. */
  virtual std::string GetDriveName(const char *path) = 0;
  /** Return path to the parent folder. */
  virtual std::string GetParentFolderName(const char *path) = 0;
  /** Return the file name from a path. */
  virtual std::string GetFileName(const char *path) = 0;
  /** Return base name from a path. */
  virtual std::string GetBaseName(const char *path) = 0;
  /** Return extension from path. */
  virtual std::string GetExtensionName(const char *path) = 0;
  /** Return the canonical representation of the path. */
  virtual std::string GetAbsolutePathName(const char *path) = 0;
  /** Generate name that can be used to name a temporary file. */
  virtual std::string GetTempName() = 0;
  /** Check if a drive or a share exists. */
  virtual bool DriveExists(const char *drive_spec) = 0;
  /** Check if a file exists. */
  virtual bool FileExists(const char *file_spec) = 0;
  /** Check if a path exists. */
  virtual bool FolderExists(const char *folder_spec) = 0;
  /** Get drive or UNC share. */
  virtual DriveInterface *GetDrive(const char *drive_spec) = 0;
  /** Get file. */
  virtual FileInterface *GetFile(const char *file_path) = 0;
  /** Get folder. */
  virtual FolderInterface *GetFolder(const char *folder_path) = 0;
  /** Get location of various system folders. */
  virtual FolderInterface *GetSpecialFolder(SpecialFolder special_folder) = 0;
  /** Delete a file. */
  virtual bool DeleteFile(const char *file_spec, bool force) = 0;
  /** Delete a folder. */
  virtual bool DeleteFolder(const char *folder_spec, bool force) = 0;
  /** Move a file. */
  virtual bool MoveFile(const char *source, const char *dest) = 0;
  /** Move a folder. */
  virtual bool MoveFolder(const char *source, const char *dest) = 0;
  /** Copy a file. */
  virtual bool CopyFile(const char *source, const char *dest,
                        bool overwrite) = 0;
  /** Copy a folder. */
  virtual bool CopyFolder(const char *source, const char *dest,
                          bool overwrite) = 0;
  /** Create a folder. */
  virtual FolderInterface *CreateFolder(const char *path) = 0;
  /** Create a file as a TextStream. */
  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite,
                                              bool unicode) = 0;
  /** Open a file as a TextStream. */
  virtual TextStreamInterface *OpenTextFile(const char *filename,
                                            IOMode mode,
                                            bool create,
                                            Tristate format) = 0;
  /** Create a file as a BinaryStream. */
  virtual BinaryStreamInterface *CreateBinaryFile(const char *filename,
                                                  bool overwrite) = 0;
  /** Open a file as a BinaryStream. */
  virtual BinaryStreamInterface *OpenBinaryFile(const char *filename,
                                                IOMode mode,
                                                bool create) = 0;
  /** Retrieve the standard input, output or error stream. */
  virtual TextStreamInterface *GetStandardStream(StandardStreamType type,
                                                 bool unicode) = 0;
  /** Retrieve the file version of the specified file into a string. */
  virtual std::string GetFileVersion(const char *filename) = 0;
};

/** IDriveCollection. */
class DrivesInterface : public SmallObject<> {
 protected:
  virtual ~DrivesInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  virtual int GetCount() const = 0;
  virtual bool AtEnd() = 0;
  virtual DriveInterface *GetItem() = 0;
  virtual void MoveFirst() = 0;
  virtual void MoveNext() = 0;

  typedef DriveInterface ItemType;
};

/** IDrive. */
class DriveInterface : public SmallObject<> {
 protected:
  virtual ~DriveInterface() { }

 public:
  virtual void Destroy() = 0;

 public:
  virtual std::string GetPath() = 0;
  virtual std::string GetDriveLetter() = 0;
  virtual std::string GetShareName() = 0;
  virtual DriveType GetDriveType() = 0;
  virtual FolderInterface *GetRootFolder() = 0;
  virtual int64_t GetAvailableSpace() = 0;
  virtual int64_t GetFreeSpace() = 0;
  virtual int64_t GetTotalSize() = 0;
  virtual std::string GetVolumnName() = 0;
  virtual bool SetVolumnName(const char *name) = 0;
  virtual std::string GetFileSystem() = 0;
  virtual int64_t GetSerialNumber() = 0;
  virtual bool IsReady() = 0;
};

/** IFolderCollection. */
class FoldersInterface : public SmallObject<> {
 protected:
  virtual ~FoldersInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  virtual int GetCount() const = 0;
  virtual bool AtEnd() = 0;
  virtual FolderInterface *GetItem() = 0;
  virtual void MoveFirst() = 0;
  virtual void MoveNext() = 0;

  typedef FolderInterface ItemType;
};

/** IFolder. */
class FolderInterface : public SmallObject<> {
 protected:
  virtual ~FolderInterface() { }

 public:
  virtual void Destroy() = 0;

 public:
  virtual std::string GetPath() = 0;
  virtual std::string GetName() = 0;
  virtual bool SetName(const char *name) = 0;
  virtual std::string GetShortPath() = 0;
  virtual std::string GetShortName() = 0;
  virtual DriveInterface *GetDrive() = 0;
  virtual FolderInterface *GetParentFolder() = 0;
  virtual FileAttribute GetAttributes() = 0;
  virtual bool SetAttributes(FileAttribute attributes) = 0;
  virtual Date GetDateCreated() = 0;
  virtual Date GetDateLastModified() = 0;
  virtual Date GetDateLastAccessed() = 0;
  virtual std::string GetType() = 0;
  virtual bool Delete(bool force) = 0;
  virtual bool Copy(const char *dest, bool overwrite) = 0;
  virtual bool Move(const char *dest) = 0;
  virtual bool IsRootFolder() = 0;
  /** Sum of files and subfolders. */
  virtual int64_t GetSize() = 0;
  virtual FoldersInterface *GetSubFolders() = 0;
  virtual FilesInterface *GetFiles() = 0;
  virtual TextStreamInterface *CreateTextFile(const char *filename,
                                              bool overwrite, bool unicode) = 0;
  virtual BinaryStreamInterface *CreateBinaryFile(const char *filename,
                                                  bool overwrite) = 0;
};

/** IFileCollection. */
class FilesInterface : public SmallObject<> {
 protected:
  virtual ~FilesInterface() {}

 public:
  virtual void Destroy() = 0;

 public:
  virtual int GetCount() const = 0;
  virtual bool AtEnd() = 0;
  virtual FileInterface *GetItem() = 0;
  virtual void MoveFirst() = 0;
  virtual void MoveNext() = 0;

  typedef FileInterface ItemType;
};

/** IFile. */
class FileInterface : public SmallObject<> {
 protected:
  virtual ~FileInterface() { }

 public:
  virtual void Destroy() = 0;

 public:
  virtual std::string GetPath() = 0;
  virtual std::string GetName() = 0;
  virtual bool SetName(const char *name) = 0;
  virtual std::string GetShortPath() = 0;
  virtual std::string GetShortName() = 0;
  virtual DriveInterface *GetDrive() = 0;
  virtual FolderInterface *GetParentFolder() = 0;
  virtual FileAttribute GetAttributes() = 0;
  virtual bool SetAttributes(FileAttribute attributes) = 0;
  virtual Date GetDateCreated() = 0;
  virtual Date GetDateLastModified() = 0;
  virtual Date GetDateLastAccessed() = 0;
  virtual int64_t GetSize() = 0;
  virtual std::string GetType() = 0;
  virtual bool Delete(bool force) = 0;
  virtual bool Copy(const char *dest, bool overwrite) = 0;
  virtual bool Move(const char *dest) = 0;
  virtual TextStreamInterface *OpenAsTextStream(IOMode IOMode,
                                                Tristate Format) = 0;
  virtual BinaryStreamInterface *OpenAsBinaryStream(IOMode IOMode) = 0;
};

class TextStreamInterface : public SmallObject<> {
 protected:
  virtual ~TextStreamInterface() { }

 public:
  virtual void Destroy() = 0;

 public:
  /** Current line number. */
  virtual int GetLine() = 0;
  /** Current column number. */
  virtual int GetColumn() = 0;
  /** Is the current position at the end of the stream? */
  virtual bool IsAtEndOfStream() = 0;
  /** Is the current position at the end of a line? */
  virtual bool IsAtEndOfLine() = 0;
  /** Read a specific number of characters into a string. */
  virtual bool Read(int characters, std::string *result) = 0;
  /** Read an entire line into a string. */
  virtual bool ReadLine(std::string *result) = 0;
  /** Read the entire stream into a string. */
  virtual bool ReadAll(std::string *result) = 0;
  /** Write a string to the stream. */
  virtual bool Write(const std::string &text) = 0;
  /** Write a string and an end of line to the stream. */
  virtual bool WriteLine(const std::string &text) = 0;
  /** Write a number of blank lines to the stream. */
  virtual bool WriteBlankLines(int lines) = 0;
  /** Skip a specific number of characters. */
  virtual bool Skip(int characters) = 0;
  /** Skip a line. */
  virtual bool SkipLine() = 0;
  /** Close a text stream. */
  virtual void Close() = 0;
};

/**
 * Additional interface to support binary file. Not in Microsoft's
 * Scripting.FileSystemObject.
 */
class BinaryStreamInterface : public SmallObject<> {
 protected:
  virtual ~BinaryStreamInterface() { }

 public:
  virtual void Destroy() = 0;

 public:
  /** Get current position of the stream, in number of bytes. */
  virtual int64_t GetPosition() = 0;
  /** Is the current position at the end of the stream? */
  virtual bool IsAtEndOfStream() = 0;
  /** Read a specific number of bytes. */
  virtual bool Read(int64_t bytes, std::string *result) = 0;
  /** Read the entire stream into a string. */
  virtual bool ReadAll(std::string *result) = 0;
  /** Write a string to the stream. */
  virtual bool Write(const std::string &data) = 0;
  /** Skip a specific number of bytes. */
  virtual bool Skip(int64_t bytes) = 0;
  /** Close the stream. */
  virtual void Close() = 0;
};

/** @} */

} // namespace framework

} // namespace ggadget

#endif // GGADGET_FILE_SYSTEM_INTERFACE_H__
