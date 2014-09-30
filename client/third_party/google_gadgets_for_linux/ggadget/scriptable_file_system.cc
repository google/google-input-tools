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

#include "scriptable_binary_data.h"
#include "scriptable_file_system.h"
#include "scriptable_enumerator.h"
#include "file_system_interface.h"
#include "scriptable_array.h"
#include "string_utils.h"
#include "gadget_interface.h"
#include "permissions.h"
#include "small_object.h"

namespace ggadget {

namespace framework {

// Default args for File.Delete() and Folder.Delete().
static const Variant kDeleteDefaultArgs[] = {
  Variant(false)
};
// Default args for File.Copy() and Folder.Copy().
static const Variant kCopyDefaultArgs[] = {
  Variant(),
  Variant(true)
};
// Default args for File.OpenAsTextStream().
static const Variant kOpenAsTextStreamDefaultArgs[] = {
  Variant(IO_MODE_READING), Variant(TRISTATE_FALSE)
};
// Default args for File.OpenAsBinaryStream().
static const Variant kOpenAsBinaryStreamDefaultArgs[] = {
  Variant(IO_MODE_READING)
};
// Default args for FileSystem.CreateTextFile() and Folder.CreateTextFile().
static const Variant kCreateTextFileDefaultArgs[] = {
  Variant(),
  Variant(true), Variant(false)
};
// Default args for FileSystem.CreateBinaryFile() and Folder.CreateBinaryFile().
static const Variant kCreateBinaryFileDefaultArgs[] = {
  Variant(),
  Variant(true)
};
// Default args for FileSystem.OpenTextFile().
static const Variant kOpenTextFileDefaultArgs[] = {
  Variant(),
  Variant(IO_MODE_READING), Variant(false), Variant(TRISTATE_FALSE)
};
// Default args for FileSystem.OpenBinaryFile().
static const Variant kOpenBinaryFileDefaultArgs[] = {
  Variant(),
  Variant(IO_MODE_READING), Variant(false)
};
// Default args for FileSystem.DeleteFile() and FileSystem.DeleteFolder()
static const Variant kDeleteFileOrFolderDefaultArgs[] = {
  Variant(),
  Variant(false)
};
// Default args for FileSystem.CopyFile() and FileSystem.CopyFolder()
static const Variant kCopyFileOrFolderDefaultArgs[] = {
  Variant(), Variant(),
  Variant(true)
};
// Default args for FileSystem.GetStandardStream().
static const Variant kGetStandardStreamDefaultArgs[] = {
  Variant(),
  Variant(false)
};

class ScriptableFileSystem::Impl : public SmallObject<> {
 public:
  Impl(FileSystemInterface *filesystem,
       ScriptableFileSystem *owner,
       GadgetInterface *gadget)
      : filesystem_(filesystem), owner_(owner), gadget_(gadget) {
  }

  class FileSystemException : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x9c53dee0b2114ce4, ScriptableInterface);

    FileSystemException(const char *message)
        : message_(message) {
      message_ += " failed.";
      RegisterConstant("message", message_);
      RegisterMethod("toString", NewSimpleGetterSlot(&message_));
    }

    // Required by webkit-script-runtime.
    virtual bool IsStrict() const { return false; }

   private:
    std::string message_;
  };

  class ScriptableTextStream : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x34828c47e6a243c5, ScriptableInterface);
    ScriptableTextStream(TextStreamInterface *stream, Impl *impl)
        : stream_(stream) {
      ASSERT(stream);
      if (impl->CanRead()) {
        RegisterProperty("Line",
                         NewSlot(stream, &TextStreamInterface::GetLine),
                         NULL);
        RegisterProperty("Column",
                         NewSlot(stream, &TextStreamInterface::GetColumn),
                         NULL);
        RegisterProperty("AtEndOfStream",
                         NewSlot(stream, &TextStreamInterface::IsAtEndOfStream),
                         NULL);
        RegisterProperty("AtEndOfLine",
                         NewSlot(stream, &TextStreamInterface::IsAtEndOfLine),
                         NULL);
        RegisterMethod("Read",
                       NewSlot(this, &ScriptableTextStream::Read));
        RegisterMethod("ReadLine",
                       NewSlot(this, &ScriptableTextStream::ReadLine));
        RegisterMethod("ReadAll",
                       NewSlot(this, &ScriptableTextStream::ReadAll));
        RegisterMethod("Skip",
                       NewSlot(this, &ScriptableTextStream::Skip));
        RegisterMethod("SkipLine",
                       NewSlot(this, &ScriptableTextStream::SkipLine));
      }
      if (impl->CanWrite()) {
        RegisterMethod("Write",
                       NewSlot(this, &ScriptableTextStream::Write));
        RegisterMethod("WriteLine",
                       NewSlot(this, &ScriptableTextStream::WriteLine));
        RegisterMethod("WriteBlankLines",
                       NewSlot(this, &ScriptableTextStream::WriteBlankLines));
      }
      RegisterMethod("Close", NewSlot(stream, &TextStreamInterface::Close));
    }

    virtual ~ScriptableTextStream() {
      stream_->Destroy();
      stream_ = NULL;
    }

    std::string Read(int characters) {
      std::string result;
      if (!stream_->Read(characters, &result))
        SetPendingException(new FileSystemException("TextStream.Read"));
      return result;
    }

    std::string ReadLine() {
      std::string result;
      if (!stream_->ReadLine(&result))
        SetPendingException(new FileSystemException("TextStream.ReadLine"));
      return result;
    }

    std::string ReadAll() {
      std::string result;
      if (!stream_->ReadAll(&result))
        SetPendingException(new FileSystemException("TextStream.ReadAll"));
      return result;
    }

    void Write(const Variant &data) {
      std::string text;
      if (!data.ConvertToString(&text) || !stream_->Write(text))
        SetPendingException(new FileSystemException("TextStream.Write"));
    }

    void WriteLine(const Variant &data) {
      std::string text;
      if (!data.ConvertToString(&text) || !stream_->WriteLine(text))
        SetPendingException(new FileSystemException("TextStream.WriteLine"));
    }

    void WriteBlankLines(int lines) {
      if (!stream_->WriteBlankLines(lines))
        SetPendingException(
            new FileSystemException("TextStream.WriteBlankLines"));
    }

    void Skip(int characters) {
      if (!stream_->Skip(characters))
        SetPendingException(new FileSystemException("TextStream.Skip"));
    }

    void SkipLine() {
      if (!stream_->SkipLine())
        SetPendingException(new FileSystemException("TextStream.SkipLine"));
    }

    TextStreamInterface *stream_;
  };

  class ScriptableBinaryStream : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x6310117247434e8e, ScriptableInterface);
    ScriptableBinaryStream(BinaryStreamInterface *stream, Impl *impl)
        : stream_(stream) {
      ASSERT(stream);
      if (impl->CanRead()) {
        RegisterProperty("Position",
                         NewSlot(stream, &BinaryStreamInterface::GetPosition),
                         NULL);
        RegisterProperty("AtEndOfStream",
                         NewSlot(stream,
                                 &BinaryStreamInterface::IsAtEndOfStream),
                         NULL);
        RegisterMethod("Read",
                       NewSlot(this, &ScriptableBinaryStream::Read));
        RegisterMethod("ReadAll",
                       NewSlot(this, &ScriptableBinaryStream::ReadAll));
        RegisterMethod("Skip",
                       NewSlot(this, &ScriptableBinaryStream::Skip));
      }
      if (impl->CanWrite()) {
        RegisterMethod("Write",
                       NewSlot(this, &ScriptableBinaryStream::Write));
      }
      RegisterMethod("Close", NewSlot(stream, &BinaryStreamInterface::Close));
    }

    virtual ~ScriptableBinaryStream() {
      stream_->Destroy();
      stream_ = NULL;
    }

    ScriptableBinaryData *Read(int64_t bytes) {
      std::string data;
      ScriptableBinaryData *result = NULL;
      if (stream_->Read(bytes, &data))
        result = new ScriptableBinaryData(data);
      else
        SetPendingException(new FileSystemException("BinaryStream.Read"));

      return result;
    }

    ScriptableBinaryData *ReadAll() {
      std::string data;
      ScriptableBinaryData *result = NULL;
      if (stream_->ReadAll(&data))
        result = new ScriptableBinaryData(data);
      else
        SetPendingException(new FileSystemException("BinaryStream.Read"));

      return result;
    }

    void Write(const Variant &data) {
      bool result = false;
      if (data.type() == Variant::TYPE_STRING) {
        result = stream_->Write(VariantValue<const std::string &>()(data));
      } else if (data.type() == Variant::TYPE_SCRIPTABLE) {
        ScriptableInterface *scriptable =
            VariantValue<ScriptableInterface *>()(data);
        if (scriptable &&
            scriptable->IsInstanceOf(ScriptableBinaryData::CLASS_ID)) {
          result = stream_->Write(
              down_cast<ScriptableBinaryData *>(scriptable)->data());
        }
      }

      if (!result)
        SetPendingException(new FileSystemException("BinaryStream.Write"));
    }

    void Skip(int64_t bytes) {
      if (!stream_->Skip(bytes))
        SetPendingException(new FileSystemException("BinaryStream.Skip"));
    }

    BinaryStreamInterface *stream_;
  };

  class ScriptableFolder;
  class ScriptableDrive : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x0a34071a4804434b, ScriptableInterface);
    ScriptableDrive(DriveInterface *drive, Impl *impl)
        : drive_(drive), impl_(impl) {
      ASSERT(drive);
      if (impl->CanRead()) {
        RegisterMethod("toString",
                       NewSlot(drive, &DriveInterface::GetPath));
        RegisterProperty("Path",
                         NewSlot(drive, &DriveInterface::GetPath),
                         NULL);
        RegisterProperty("DriveLetter",
                         NewSlot(drive, &DriveInterface::GetDriveLetter),
                         NULL);
        RegisterProperty("ShareName",
                         NewSlot(drive, &DriveInterface::GetShareName),
                         NULL);
        RegisterProperty("DriveType",
                         NewSlot(drive, &DriveInterface::GetDriveType),
                         NULL);
        RegisterProperty("AvailableSpace",
                         NewSlot(drive, &DriveInterface::GetAvailableSpace),
                         NULL);
        RegisterProperty("FreeSpace",
                         NewSlot(drive, &DriveInterface::GetFreeSpace),
                         NULL);
        RegisterProperty("TotalSize",
                         NewSlot(drive, &DriveInterface::GetTotalSize),
                         NULL);
        RegisterProperty("FileSystem",
                         NewSlot(drive, &DriveInterface::GetFileSystem),
                         NULL);
        RegisterProperty("SerialNumber",
                         NewSlot(drive, &DriveInterface::GetSerialNumber),
                         NULL);
        RegisterProperty("IsReady",
                         NewSlot(drive, &DriveInterface::IsReady),
                         NULL);
        RegisterProperty("RootFolder",
                         NewSlot(this, &ScriptableDrive::GetRootFolder),
                         NULL);
      }
      if (impl->CanRead() || impl->CanWrite()) {
        RegisterProperty("VolumnName",
                         (impl->CanRead() ?
                          NewSlot(drive, &DriveInterface::GetVolumnName) :
                          NULL),
                         (impl->CanWrite() ?
                          NewSlot(this, &ScriptableDrive::SetVolumnName) :
                          NULL));
      }
    }

    virtual ~ScriptableDrive() {
      drive_->Destroy();
      drive_ = NULL;
    }

    ScriptableFolder *GetRootFolder();

    void SetVolumnName(const char *name) {
      if (!drive_->SetVolumnName(name))
        SetPendingException(new FileSystemException("Drive.SetVolumnName"));
    }

    DriveInterface *drive_;
    Impl *impl_;
  };

  class ScriptableFolder : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0xa2e7a3ef662a445c, ScriptableInterface);
    ScriptableFolder(FolderInterface *folder, Impl *impl)
        : folder_(folder), impl_(impl) {
      ASSERT(folder);
      if (impl->CanRead()) {
        RegisterMethod("toString",
                       NewSlot(folder, &FolderInterface::GetPath));
        RegisterProperty("Path",
                         NewSlot(folder, &FolderInterface::GetPath),
                         NULL);
        RegisterProperty("ShortPath",
                         NewSlot(folder, &FolderInterface::GetShortPath),
                         NULL);
        RegisterProperty("ShortName",
                         NewSlot(folder, &FolderInterface::GetShortName),
                         NULL);
        RegisterProperty("DateCreated",
                         NewSlot(folder, &FolderInterface::GetDateCreated),
                         NULL);
        RegisterProperty("DateLastModified",
                         NewSlot(folder, &FolderInterface::GetDateLastModified),
                         NULL);
        RegisterProperty("DateLastAccessed",
                         NewSlot(folder, &FolderInterface::GetDateLastAccessed),
                         NULL);
        RegisterProperty("Type",
                         NewSlot(folder, &FolderInterface::GetType),
                         NULL);
        RegisterProperty("Size",
                         NewSlot(folder, &FolderInterface::GetSize),
                         NULL);
        RegisterProperty("SubFolders",
                         NewSlot(this, &ScriptableFolder::GetSubFolders),
                         NULL);
        RegisterProperty("Files",
                         NewSlot(this, &ScriptableFolder::GetFiles),
                         NULL);
        RegisterProperty("ParentFolder",
                         NewSlot(this, &ScriptableFolder::GetParentFolder),
                         NULL);
        RegisterProperty("Drive",
                         NewSlot(this, &ScriptableFolder::GetDrive),
                         NULL);
      }
      if (impl->CanRead() || impl->CanWrite()) {
        RegisterProperty("Attributes",
                         (impl->CanRead() ?
                          NewSlot(folder, &FolderInterface::GetAttributes) :
                          NULL),
                         (impl->CanWrite() ?
                          NewSlot(this, &ScriptableFolder::SetAttributes) :
                          NULL));
        RegisterProperty("Name",
                         (impl->CanRead() ?
                          NewSlot(folder, &FolderInterface::GetName) :
                          NULL),
                         (impl->CanWrite() ?
                          NewSlot(this, &ScriptableFolder::SetName) :
                          NULL));
      }
      if (impl->CanWrite()) {
        RegisterMethod("Delete",
            NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFolder::Delete),
                                   kDeleteDefaultArgs));
        RegisterMethod("Copy",
            NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFolder::Copy),
                                   kCopyDefaultArgs));
        RegisterMethod("Move", NewSlot(this, &ScriptableFolder::Move));
        RegisterMethod("CreateTextFile",
                       NewSlotWithDefaultArgs(
                           NewSlot(this, &ScriptableFolder::CreateTextFile),
                           kCreateTextFileDefaultArgs));
        RegisterMethod("CreateBinaryFile",
                       NewSlotWithDefaultArgs(
                           NewSlot(this, &ScriptableFolder::CreateBinaryFile),
                           kCreateBinaryFileDefaultArgs));
      }
    }

    virtual ~ScriptableFolder() {
      folder_->Destroy();
      folder_ = NULL;
    }

    void SetName(const char *name) {
      if (!folder_->SetName(name))
        SetPendingException(new FileSystemException("Folder.SetName"));
    }

    ScriptableDrive *GetDrive() {
      DriveInterface *drive = folder_->GetDrive();
      if (!drive) {
        SetPendingException(new FileSystemException("Folder.GetDrive"));
        return NULL;
      }
      return new ScriptableDrive(drive, impl_);
    }

    ScriptableFolder *GetParentFolder() {
      FolderInterface *folder = folder_->GetParentFolder();
      if (!folder) {
        SetPendingException(new FileSystemException("Folder.GetParentFolder"));
        return NULL;
      }
      return new ScriptableFolder(folder, impl_);
    }

    void SetAttributes(FileAttribute attributes) {
      if (!folder_->SetAttributes(attributes))
        SetPendingException(new FileSystemException("Folder.SetAttributes"));
    }

    void Delete(bool force) {
      if (!folder_->Delete(force))
        SetPendingException(new FileSystemException("Folder.Delete"));
    }

    void Copy(const char *dest, bool overwrite) {
      if (!folder_->Copy(dest, overwrite))
        SetPendingException(new FileSystemException("Folder.Copy"));
    }

    void Move(const char *dest) {
      if (!folder_->Move(dest))
        SetPendingException(new FileSystemException("Folder.Move"));
    }

    ScriptableInterface *GetSubFolders() {
      FoldersInterface *folders = folder_->GetSubFolders();
      if (!folders) {
        SetPendingException(new FileSystemException("Folder.GetSubFolders"));
        return NULL;
      }
      return new ScriptableEnumerator<FoldersInterface,
                                      ScriptableFolder,
                                      Impl *,
                                      UINT64_C(0x6d148a105ed311dd)>(this,
                                                                    folders,
                                                                    impl_);
    }

    ScriptableInterface *GetFiles() {
      FilesInterface *files = folder_->GetFiles();
      if (!files) {
        SetPendingException(new FileSystemException("Folder.GetFiles"));
        return NULL;
      }
      return new ScriptableEnumerator<FilesInterface,
                                      ScriptableFile,
                                      Impl *,
                                      UINT64_C(0x86c5e4581be444a3)>(this,
                                                                    files,
                                                                    impl_);
    }

    ScriptableTextStream *CreateTextFile(const char *filename,
                                         bool overwrite,
                                         bool unicode) {
      TextStreamInterface *stream =
          folder_->CreateTextFile(filename, overwrite, unicode);
      if (!stream) {
        SetPendingException(new FileSystemException("Folder.CreateTextFile"));
        return NULL;
      }
      return new ScriptableTextStream(stream, impl_);
    }

    ScriptableBinaryStream *CreateBinaryFile(const char *filename,
                                             bool overwrite) {
      BinaryStreamInterface *stream =
          folder_->CreateBinaryFile(filename, overwrite);
      if (!stream) {
        SetPendingException(new FileSystemException("Folder.CreateBinaryFile"));
        return NULL;
      }
      return new ScriptableBinaryStream(stream, impl_);
    }

    FolderInterface *folder_;
    Impl *impl_;
  };

  class ScriptableFile : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0xd8071714bc0a4d2c, ScriptableInterface);
    ScriptableFile(FileInterface *file, Impl *impl)
        : file_(file), impl_(impl) {
      ASSERT(file);
      if (impl->CanRead()) {
        RegisterMethod("toString",
                       NewSlot(file, &FileInterface::GetPath));
        RegisterProperty("Path",
                         NewSlot(file, &FileInterface::GetPath),
                         NULL);
        RegisterProperty("ShortPath",
                         NewSlot(file, &FileInterface::GetShortPath),
                         NULL);
        RegisterProperty("ShortName",
                         NewSlot(file, &FileInterface::GetShortName),
                         NULL);
        RegisterProperty("Drive",
                         NewSlot(this, &ScriptableFile::GetDrive),
                         NULL);
        RegisterProperty("ParentFolder",
                         NewSlot(this, &ScriptableFile::GetParentFolder),
                         NULL);
        RegisterProperty("DateCreated",
                         NewSlot(file, &FileInterface::GetDateCreated),
                         NULL);
        RegisterProperty("DateLastModified",
                         NewSlot(file, &FileInterface::GetDateLastModified),
                         NULL);
        RegisterProperty("DateLastAccessed",
                         NewSlot(file, &FileInterface::GetDateLastAccessed),
                         NULL);
        RegisterProperty("Size",
                         NewSlot(file, &FileInterface::GetSize),
                         NULL);
        RegisterProperty("Type",
                         NewSlot(file, &FileInterface::GetType),
                         NULL);
      }
      if (impl->CanRead() || impl->CanWrite()) {
        RegisterMethod("OpenAsTextStream",
                       NewSlotWithDefaultArgs(
                           NewSlot(this, &ScriptableFile::OpenAsTextStream),
                           kOpenAsTextStreamDefaultArgs));
        RegisterMethod("OpenAsBinaryStream",
                       NewSlotWithDefaultArgs(
                           NewSlot(this, &ScriptableFile::OpenAsBinaryStream),
                           kOpenAsBinaryStreamDefaultArgs));
        RegisterProperty("Attributes",
                         (impl->CanRead() ?
                          NewSlot(file, &FileInterface::GetAttributes) :
                          NULL),
                         (impl->CanWrite() ?
                          NewSlot(this, &ScriptableFile::SetAttributes) :
                          NULL));
        RegisterProperty("Name",
                         (impl->CanRead() ?
                          NewSlot(file, &FileInterface::GetName) :
                          NULL),
                         (impl->CanWrite() ?
                          NewSlot(this, &ScriptableFile::SetName) :
                          NULL));
      }
      if (impl->CanWrite()) {
        RegisterMethod("Delete",
            NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFile::Delete),
                                   kDeleteDefaultArgs));
        RegisterMethod("Copy",
            NewSlotWithDefaultArgs(NewSlot(this, &ScriptableFile::Copy),
                                   kCopyDefaultArgs));
        RegisterMethod("Move", NewSlot(this, &ScriptableFile::Move));
      }
    }

    virtual ~ScriptableFile() {
      file_->Destroy();
      file_ = NULL;
    }

    void SetName(const char *name) {
      if (!file_->SetName(name))
        SetPendingException(new FileSystemException("File.SetName"));
    }

    ScriptableDrive *GetDrive() {
      DriveInterface *drive = file_->GetDrive();
      if (!drive) {
        SetPendingException(new FileSystemException("File.GetDrive"));
        return NULL;
      }
      return new ScriptableDrive(drive, impl_);
    }

    ScriptableFolder *GetParentFolder() {
      FolderInterface *folder = file_->GetParentFolder();
      if (!folder) {
        SetPendingException(new FileSystemException("File.GetParentFolder"));
        return NULL;
      }
      return new ScriptableFolder(folder, impl_);
    }

    void SetAttributes(FileAttribute attributes) {
      if (!file_->SetAttributes(attributes))
        SetPendingException(new FileSystemException("File.SetAttributes"));
    }

    void Delete(bool force) {
      if (!file_->Delete(force))
        SetPendingException(new FileSystemException("File.Delete"));
    }

    void Copy(const char *dest, bool overwrite) {
      if (!file_->Copy(dest, overwrite))
        SetPendingException(new FileSystemException("File.Copy"));
    }

    void Move(const char *dest) {
      if (!file_->Move(dest))
        SetPendingException(new FileSystemException("File.Move"));
    }

    ScriptableTextStream *OpenAsTextStream(IOMode mode, Tristate format) {
      TextStreamInterface *stream =
          file_->OpenAsTextStream(mode, format);
      if (!stream) {
        SetPendingException(new FileSystemException("File.OpenAsTextStream"));
        return NULL;
      }
      return new ScriptableTextStream(stream, impl_);
    }

    ScriptableBinaryStream *OpenAsBinaryStream(IOMode mode) {
      BinaryStreamInterface *stream = file_->OpenAsBinaryStream(mode);
      if (!stream) {
        SetPendingException(new FileSystemException("File.OpenAsBinaryStream"));
        return NULL;
      }
      return new ScriptableBinaryStream(stream, impl_);
    }

    FileInterface *file_;
    Impl *impl_;
  };

  ScriptableInterface *GetDrives() {
    DrivesInterface *drives = filesystem_->GetDrives();
    if (!drives) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetDrives"));
      return NULL;
    }
    return new ScriptableEnumerator<DrivesInterface,
                                    ScriptableDrive,
                                    Impl *,
                                    UINT64_C(0x98e7d3665ed011dd)>(owner_,
                                                                  drives,
                                                                  this);
  }

  ScriptableDrive *GetDrive(const char *drive_spec) {
    DriveInterface *drive = filesystem_->GetDrive(drive_spec);
    if (!drive) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetDrive"));
      return NULL;
    }
    return new ScriptableDrive(drive, this);
  }

  ScriptableFile *GetFile(const char *file_path) {
    FileInterface *file = filesystem_->GetFile(file_path);
    if (!file) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetFile"));
      return NULL;
    }
    return new ScriptableFile(file, this);
  }

  ScriptableFolder *GetFolder(const char *folder_path) {
    FolderInterface *folder = filesystem_->GetFolder(folder_path);
    if (!folder) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetFolder"));
      return NULL;
    }
    return new ScriptableFolder(folder, this);
  }

  ScriptableFolder *GetSpecialFolder(SpecialFolder special) {
    FolderInterface *folder = filesystem_->GetSpecialFolder(special);
    if (!folder) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.GetSpecialFolder"));
      return NULL;
    }
    return new ScriptableFolder(folder, this);
  }

  void DeleteFile(const char *file_spec, bool force) {
    if (!filesystem_->DeleteFile(file_spec, force))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.DeleteFile"));
  }

  void DeleteFolder(const char *folder_spec, bool force) {
    if (!filesystem_->DeleteFolder(folder_spec, force))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.DeleteFolder"));
  }

  void MoveFile(const char *source, const char *dest) {
    if (!filesystem_->MoveFile(source, dest))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.MoveFile"));
  }

  void MoveFolder(const char *source, const char *dest) {
    if (!filesystem_->MoveFolder(source, dest))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.MoveFolder"));
  }

  void CopyFile(const char *source, const char *dest, bool overwrite) {
    if (!filesystem_->CopyFile(source, dest, overwrite))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.CopyFile"));
  }

  void CopyFolder(const char *source, const char *dest, bool overwrite) {
    if (!filesystem_->CopyFolder(source, dest, overwrite))
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.CopyFolder"));
  }

  ScriptableFolder *CreateFolder(const char *path) {
    FolderInterface *folder = filesystem_->CreateFolder(path);
    if (!folder) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.CreateFolder"));
      return NULL;
    }
    return new ScriptableFolder(folder, this);
  }

  ScriptableTextStream *CreateTextFile(const char *filename,
                                       bool overwrite,
                                       bool unicode) {
    TextStreamInterface *stream =
        filesystem_->CreateTextFile(filename, overwrite, unicode);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "Filesystem.CreateTextFile"));
      return NULL;
    }
    return new ScriptableTextStream(stream, this);
  }

  ScriptableBinaryStream *CreateBinaryFile(const char *filename,
                                           bool overwrite) {
    BinaryStreamInterface *stream =
        filesystem_->CreateBinaryFile(filename, overwrite);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "Filesystem.CreateBinaryFile"));
      return NULL;
    }
    return new ScriptableBinaryStream(stream, this);
  }

  ScriptableTextStream *OpenTextFile(const char *filename, IOMode mode,
                                     bool create, Tristate format) {
    TextStreamInterface *stream =
        filesystem_->OpenTextFile(filename, mode, create, format);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.OpenTextFile"));
      return NULL;
    }
    return new ScriptableTextStream(stream, this);
  }

  ScriptableBinaryStream *OpenBinaryFile(const char *filename, IOMode mode,
                                         bool create) {
    BinaryStreamInterface *stream =
        filesystem_->OpenBinaryFile(filename, mode, create);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "FileSystem.OpenBinaryFile"));
      return NULL;
    }
    return new ScriptableBinaryStream(stream, this);
  }

  ScriptableTextStream *GetStandardStream(StandardStreamType type,
                                          bool unicode) {
    TextStreamInterface *stream =
        filesystem_->GetStandardStream(type, unicode);
    if (!stream) {
      owner_->SetPendingException(new FileSystemException(
          "Filesystem.GetStandardStream"));
      return NULL;
    }
    return new ScriptableTextStream(stream, this);
  }

  bool CanRead() const {
    const Permissions *permissions = gadget_ ? gadget_->GetPermissions() : NULL;
    return permissions &&
        permissions->IsRequiredAndGranted(Permissions::FILE_READ);
  }

  bool CanWrite() const {
    const Permissions *permissions = gadget_ ? gadget_->GetPermissions() : NULL;
    return permissions &&
        permissions->IsRequiredAndGranted(Permissions::FILE_WRITE);
  }

  FileSystemInterface *filesystem_;
  ScriptableFileSystem *owner_;
  GadgetInterface *gadget_;
};

ScriptableFileSystem::ScriptableFileSystem(FileSystemInterface *filesystem,
                                           GadgetInterface *gadget)
    : impl_(new Impl(filesystem, this, gadget)) {
  ASSERT(filesystem);
  if (impl_->CanRead()) {
    RegisterProperty("Drives", NewSlot(impl_, &Impl::GetDrives), NULL);
    RegisterMethod("BuildPath",
                   NewSlot(filesystem, &FileSystemInterface::BuildPath));
    RegisterMethod("GetDriveName",
                  NewSlot(filesystem,
                          &FileSystemInterface::GetDriveName));
    RegisterMethod("GetParentFolderName",
                  NewSlot(filesystem,
                          &FileSystemInterface::GetParentFolderName));
    RegisterMethod("GetFileName",
                  NewSlot(filesystem, &FileSystemInterface::GetFileName));
    RegisterMethod("GetBaseName",
                  NewSlot(filesystem, &FileSystemInterface::GetBaseName));
    RegisterMethod("GetExtensionName",
                  NewSlot(filesystem, &FileSystemInterface::GetExtensionName));
    RegisterMethod("GetAbsolutePathName",
                  NewSlot(filesystem,
                          &FileSystemInterface::GetAbsolutePathName));
    RegisterMethod("GetTempName",
                   NewSlot(filesystem, &FileSystemInterface::GetTempName));
    RegisterMethod("DriveExists",
                   NewSlot(filesystem, &FileSystemInterface::DriveExists));
    RegisterMethod("FileExists",
                   NewSlot(filesystem, &FileSystemInterface::FileExists));
    RegisterMethod("FolderExists",
                   NewSlot(filesystem, &FileSystemInterface::FolderExists));
    RegisterMethod("GetFileVersion",
                   NewSlot(filesystem, &FileSystemInterface::GetFileVersion));
    RegisterMethod("GetDrive", NewSlot(impl_, &Impl::GetDrive));
    RegisterMethod("GetFile", NewSlot(impl_, &Impl::GetFile));
    RegisterMethod("GetFolder", NewSlot(impl_, &Impl::GetFolder));
    RegisterMethod("GetSpecialFolder", NewSlot(impl_, &Impl::GetSpecialFolder));
  }
  if (impl_->CanRead() || impl_->CanWrite()) {
    RegisterMethod("OpenTextFile",
                   NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::OpenTextFile),
                                          kOpenTextFileDefaultArgs));
    RegisterMethod("OpenBinaryFile",
                   NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::OpenBinaryFile),
                                          kOpenBinaryFileDefaultArgs));
    RegisterMethod("GetStandardStream",
        NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::GetStandardStream),
                               kGetStandardStreamDefaultArgs));
  }
  if (impl_->CanWrite()) {
    RegisterMethod("DeleteFile",
                   NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::DeleteFile),
                                          kDeleteFileOrFolderDefaultArgs));
    RegisterMethod("DeleteFolder",
                   NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::DeleteFolder),
                                          kDeleteFileOrFolderDefaultArgs));
    RegisterMethod("MoveFile", NewSlot(impl_, &Impl::MoveFile));
    RegisterMethod("MoveFolder", NewSlot(impl_, &Impl::MoveFolder));
    RegisterMethod("CopyFile",
                   NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::CopyFile),
                                          kCopyFileOrFolderDefaultArgs));
    RegisterMethod("CopyFolder",
                   NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::CopyFolder),
                                          kCopyFileOrFolderDefaultArgs));
    RegisterMethod("CreateFolder", NewSlot(impl_, &Impl::CreateFolder));
    RegisterMethod("CreateTextFile",
                   NewSlotWithDefaultArgs(NewSlot(impl_, &Impl::CreateTextFile),
                                          kCreateTextFileDefaultArgs));
    RegisterMethod("CreateBinaryFile", NewSlotWithDefaultArgs(
        NewSlot(impl_, &Impl::CreateBinaryFile), kCreateBinaryFileDefaultArgs));
  }
}

ScriptableFileSystem::Impl::ScriptableFolder *
ScriptableFileSystem::Impl::ScriptableDrive::GetRootFolder() {
  FolderInterface *folder = drive_->GetRootFolder();
  if (!folder) {
    SetPendingException(new FileSystemException("Drive.GetRootFolder"));
    return NULL;
  }
  return new ScriptableFolder(folder, impl_);
}

ScriptableFileSystem::~ScriptableFileSystem() {
  delete impl_;
  impl_ = NULL;
}

} // namespace framework
} // namespace ggadget
