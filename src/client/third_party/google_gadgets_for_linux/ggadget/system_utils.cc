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
#include "build_config.h"  // It defines OS_WIN

#include <clocale>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <string>
#include <vector>

#include "common.h"
#include "scoped_ptr.h"
#include "file_manager_factory.h"
#include "gadget_consts.h"
#include "logger.h"
#include "system_utils.h"
#include "string_utils.h"
#include "system_file_functions.h"

#if defined(OS_WIN)
#include <lmcons.h>
#include <shlobj.h>
#include <shlwapi.h>
#elif defined(OS_POSIX)
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(OS_WIN)
namespace {
static const wchar_t kWideDirSeparator = L'\\';
}
#endif

namespace ggadget {
std::string BuildPath(const char *separator, const char *element, ...) {
  std::string result;
  va_list args;
  va_start(args, element);
  result = BuildPathV(separator, element, args);
  va_end(args);
  return result;
}

std::string BuildPathV(const char *separator, const char *element, va_list ap) {
  std::string result;

  if (!separator || !*separator)
    separator = kDirSeparatorStr;

  size_t sep_len = strlen(separator);

  while (element) {
    size_t elm_len = strlen(element);

    bool has_leading_sep = false;
    // Remove leading separators in element.
    while (elm_len >= sep_len && strncmp(element, separator, sep_len) == 0) {
      element += sep_len;
      elm_len -= sep_len;
      has_leading_sep = true;
    }

    // Remove trailing separators in element.
    while (elm_len >= sep_len &&
           strncmp(element + elm_len - sep_len, separator, sep_len) == 0) {
      elm_len -= sep_len;
    }

    // If the first element has leading separator, then means that the part
    // starts from root.
    if (!result.length() && has_leading_sep) {
      result.append(separator, sep_len);
    }

    // skip empty element.
    if (elm_len) {
      if (result.length() && (result.length() < sep_len ||
          strncmp(result.c_str() + result.length() - sep_len,
                  separator, sep_len) != 0)) {
        result.append(separator, sep_len);
      }
      result.append(element, elm_len);
    }

    element = va_arg(ap, const char *);
  }

  return result;
}

std::string BuildFilePath(const char *element, ...) {
  std::string result;
  va_list args;
  va_start(args, element);
  result = BuildPathV(kDirSeparatorStr, element, args);
  va_end(args);
  return result;
}

std::string BuildFilePathV(const char *element, va_list ap) {
  return BuildPathV(kDirSeparatorStr, element, ap);
}

bool SplitFilePath(const char *path, std::string *dir, std::string *filename) {
#if defined(OS_WIN)
  std::wstring utf16_path;
  ConvertStringUTF8ToUTF16(path, strlen(path), &utf16_path);
  ASSERT(utf16_path.size() < MAX_PATH);
  wchar_t utf16_directory[MAX_PATH] = {0};
  wcsncpy(utf16_directory, utf16_path.c_str(), utf16_path.size());
  ::PathRemoveFileSpecW(utf16_directory);
  size_t split_position = wcslen(utf16_directory);
  if (dir) {
    // Sometimes, there are a few '\\'s between directory name and file name.
    // PathRemoveFileSpecA only removes at most 2 '\\'s at end end of directory.
    // We need to remove rest trailing '\\'s.
    while (true) {
      wchar_t* result = ::PathRemoveBackslashW(utf16_directory);
      // If empty or no more backslash, stop
      if (*utf16_directory == L'\0' || *result != L'\0')
        break;
    }
    // dir should not have trailing backslash.
    ConvertStringUTF16ToUTF8(utf16_directory,
                             wcslen(utf16_directory),
                             dir);
  }
  // Skips "\\" before filename
  while (split_position < utf16_path.size() &&
         utf16_path[split_position] == kWideDirSeparator) {
    ++split_position;
  }
  if (filename) {
    ConvertStringUTF16ToUTF8(utf16_path.c_str() + split_position,
                             utf16_path.size() - split_position,
                             filename);
  }
  return split_position != 0 &&  // It has directory part.
         split_position != utf16_path.size();  // It has filename part.
#elif defined(OS_POSIX)
  if (!path || !*path) return false;

  if (dir) *dir = std::string();
  if (filename) *filename = std::string();

  size_t len = strlen(path);
  size_t sep_len = strlen(kDirSeparatorStr);

  // No dir part.
  if (len < sep_len) {
    if (filename) filename->assign(path, len);
    return false;
  }

  const char *last_sep = path + len - sep_len;
  bool has_sep = false;

  for (; last_sep >= path; --last_sep) {
    if (strncmp(last_sep, kDirSeparatorStr, sep_len) == 0) {
      has_sep = true;
      break;
    }
  }

  if (has_sep) {
    // If the path refers to a file in root directory, then the root directory
    // will be returned.
    if (dir) {
      const char *first_sep = last_sep;
      for (; first_sep - sep_len >= path; first_sep -= sep_len) {
        if (strncmp(first_sep - sep_len, kDirSeparatorStr, sep_len) != 0)
          break;
      }
      dir->assign(path, (first_sep == path ? sep_len : first_sep - path));
    }
    last_sep += sep_len;
  } else {
    last_sep++;
  }

  if (filename && *last_sep)
    filename->assign(last_sep);

  return has_sep && *last_sep;
#endif
}

bool EnsureDirectories(const char *path) {
  if (!path || !*path) {
    LOG("Can't create empty path.");
    return false;
  }
#if defined(OS_WIN)
  std::wstring utf16_path;
  ConvertStringUTF8ToUTF16(path, strlen(path), &utf16_path);
  if (::PathIsDirectoryW(utf16_path.c_str()) == FILE_ATTRIBUTE_DIRECTORY)
    return true;
  int result_code = ::SHCreateDirectoryExW(NULL,  // No parent window
                                           utf16_path.c_str(),  // folder path
                                           NULL);  // default security attribs
  if (result_code == ERROR_BAD_PATHNAME) {  // Path's probably relative.
    wchar_t utf16_full_path[MAX_PATH] = {0};
    wchar_t utf16_current_path[MAX_PATH] = {0};
    if (::GetCurrentDirectoryW(MAX_PATH, utf16_current_path) > 0 &&
        ::PathCombineW(utf16_full_path, utf16_current_path,
                       utf16_path.c_str()) != NULL) {
      result_code = ::SHCreateDirectoryExW(NULL, utf16_full_path, NULL);
    }
  }
  if (result_code != ERROR_SUCCESS) {
     LOG("Can not create directory: '%s' return_code: %d", path, result_code);
     return false;
  }
  return true;
#elif defined(OS_POSIX)
  struct stat stat_value;
  memset(&stat_value, 0, sizeof(stat_value));
  if (::stat(path, &stat_value) == 0) {
    if (S_ISDIR(stat_value.st_mode))
      return true;
    LOG("Path is not a directory: '%s'", path);
    return false;
  }
  if (errno != ENOENT) {
    LOG("Failed to access directory: '%s' error: %s", path, strerror(errno));
    return false;
  }

  std::string dir, file;
  SplitFilePath(path, &dir, &file);
  if (!dir.empty() && file.empty()) {
    // Deal with the case that the path has trailing '/'.
    std::string temp(dir);
    SplitFilePath(temp.c_str(), &dir, &file);
  }
  // dir will be empty if the input path is the upmost level of a relative path.
  if (!dir.empty() && !EnsureDirectories(dir.c_str()))
    return false;
  if (mkdir(path, 0700) == 0)
    return true;
  LOG("Failed to create directory: '%s' error: %s", path, strerror(errno));
  return false;
#endif
}

bool ReadFileContents(const char *path, std::string *content) {
  ASSERT(content);
  if (!path || !*path || !content)
    return false;

  content->clear();
#if defined(OS_WIN)
  std::wstring utf16_path;
  ConvertStringUTF8ToUTF16(path, strlen(path), &utf16_path);
  HANDLE handle = ::CreateFileW(utf16_path.c_str(),
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    DLOG("Can't open file %s for reading: %d", path, GetLastError());
    return false;
  }
  LARGE_INTEGER large_integer = {0};
  if (!::GetFileSizeEx(handle, &large_integer)) {
    LOG("Error when getting file size %s: %d", path, GetLastError());
    CloseHandle(handle);
    return false;
  }
  if (large_integer.QuadPart > kMaxFileSize) {
    LOG("File is too big (>%d) : %s", kMaxFileSize, path);
    CloseHandle(handle);
    return false;
  }
  DWORD size_to_load = static_cast<DWORD>(large_integer.QuadPart);
  scoped_array<char> buffer(new char[size_to_load]);
  DWORD size_loaded = 0;
  if (!ReadFile(handle, buffer.get(), size_to_load, &size_loaded, NULL) ||
      size_loaded != size_to_load) {
    LOG("Error when loading file %s: %d", path, GetLastError());
    CloseHandle(handle);
    return false;
  }
  content->append(buffer.get(), size_loaded);
  CloseHandle(handle);
  return true;
#elif defined(OS_POSIX)
  FILE *datafile = fopen(path, "rb");
  if (!datafile) {
    //DLOG("Failed to open file: %s: %s", path, strerror(errno));
    return false;
  }

  // The approach below doesn't really work for large files, so we limit the
  // file size. A memory-mapped file scheme might be better here.
  const size_t kChunkSize = 8192;
  char buffer[kChunkSize];
  while (true) {
    size_t read_size = fread(buffer, 1, kChunkSize, datafile);
    content->append(buffer, read_size);
    if (content->length() > kMaxFileSize || read_size < kChunkSize)
      break;
  }

  if (ferror(datafile)) {
    LOG("Error when reading file: %s: %s", path, strerror(errno));
    content->clear();
    fclose(datafile);
    return false;
  }

  if (content->length() > kMaxFileSize) {
    LOG("File is too big (> %zu): %s", kMaxFileSize, path);
    content->clear();
    fclose(datafile);
    return false;
  }

  fclose(datafile);
  return true;
#endif
}

bool WriteFileContents(const char *path, const std::string &content) {
  if (!path || !*path)
    return false;
#if defined(OS_WIN)
  std::wstring utf16_path;
  ConvertStringUTF8ToUTF16(path, strlen(path), &utf16_path);
  HANDLE handle = ::CreateFileW(utf16_path.c_str(),
                                GENERIC_WRITE,
                                0,  // exclusive
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    DLOG("Can't open file %s for writing: %d", path, GetLastError());
    return false;
  }
  bool result = true;
  DWORD size = 0;
  if (!::WriteFile(handle, content.c_str(), content.size(), &size, NULL) ||
      size != content.size()) {
    result = false;
    LOG("Error when writing to file %s: %d", path, GetLastError());
  }
  ::CloseHandle(handle);
  if (!result)
    ::DeleteFileW(utf16_path.c_str());
  return result;
#elif defined(OS_POSIX)
  FILE *out_fp = fopen(path, "wb");
  if (!out_fp) {
    DLOG("Can't open file %s for writing: %s", path, strerror(errno));
    return false;
  }

  bool result = true;
  if (!content.empty() &&
      fwrite(content.c_str(), content.size(), 1, out_fp) != 1) {
    result = false;
    LOG("Error when writing to file %s: %s", path, strerror(errno));
  }
  // fclose() is placed first to ensure it's always called.
  result = (fclose(out_fp) == 0 && result);

  if (!result)
    unlink(path);
  return result;
#endif
}

std::string NormalizeFilePath(const char *path) {
  if (!path || !*path)
    return std::string("");

  std::string working_path(path);

  // Replace '\\' or '/' with kDirSeparator
  for (std::string::iterator it = working_path.begin();
       it != working_path.end(); ++it)
    if (*it == '\\' || *it == '/') *it = kDirSeparator;
#if defined(OS_WIN)
  std::wstring utf16_working_path;
  ConvertStringUTF8ToUTF16(working_path, &utf16_working_path);
  wchar_t utf16_new_path[MAX_PATH] = {0};
  // Remove "." and ".."
  ::PathCanonicalizeW(utf16_new_path, utf16_working_path.c_str());
  size_t scanned = 0, left = 0;
  while (utf16_new_path[scanned] != L'\0') {
    if (scanned > 0 &&
        utf16_new_path[scanned] == kWideDirSeparator &&
        utf16_new_path[scanned-1] == kWideDirSeparator) {
      ++scanned;
      continue;  // Ignores redundent L'\\'s.
    }
    utf16_new_path[left++] = utf16_new_path[scanned++];
  }
  utf16_new_path[left] = L'\0';
  ::PathRemoveBackslashW(utf16_new_path);
  std::string result_path;
  ConvertStringUTF16ToUTF8(utf16_new_path,
                           wcslen(utf16_new_path),
                           &result_path);
  return result_path;
#elif defined(OS_POSIX)
  // Remove all "." and ".." components, and consecutive '/'s.
  std::string result;
  size_t start = 0;

  while (start < working_path.length()) {
    size_t end = working_path.find(kDirSeparator, start);
    bool omit_part = false;
    if (end == std::string::npos)
      end = working_path.length();

    size_t part_length = end - start;
    switch (part_length) {
      case 0:
        // Omit consecutive '/'s.
        omit_part = true;
        break;
      case 1:
        // Omit part in /./
        omit_part = (working_path[start] == '.');
        break;
      case 2:
        // Omit part in /../, and remove the last part in result.
        if (working_path[start] == '.' && working_path[start + 1] == '.') {
          omit_part = true;
          size_t last_sep_pos = result.find_last_of(kDirSeparator);
          if (last_sep_pos == std::string::npos)
            // No separator in the result, remove all.
            result.clear();
          else
            result.erase(last_sep_pos);
        }
        break;
      default:
        break;
    }

    if (!omit_part) {
      if (result.length() || working_path[0] == kDirSeparator)
        result += kDirSeparator;
      result += working_path.substr(start, part_length);
    }

    start = end + 1;
  }

  // Handle special case: path is pointed to root.
  if (result.empty() && working_path[0] == kDirSeparator)
    result += kDirSeparator;

  return result;
#endif
}

std::string GetCurrentDirectory() {
#if defined(OS_WIN)
  wchar_t utf16_path_buffer[MAX_PATH] = {0};
  DWORD size = ::GetCurrentDirectoryW(MAX_PATH, utf16_path_buffer);
  std::string path;
  ConvertStringUTF16ToUTF8(utf16_path_buffer, size, &path);
  return path;
#elif defined(OS_POSIX)
  char buf[4096];
  if (::getcwd(buf, sizeof(buf)) == buf) {
    // it's fit.
    return std::string(buf);
  } else {
    std::string result;
    size_t length = sizeof(buf);
    while (true) {
      length *= 2;
      char *tmp = new char[length];
      if (::getcwd(tmp, length) == tmp) {
        // it's fit.
        result = std::string(tmp);
        delete[] tmp;
        break;
      }
      delete[] tmp;
      // Other error occurred, stop trying.
      if (errno != ERANGE)
        break;
    }
    return result;
  }
#endif
}

std::string GetHomeDirectory() {
#if defined(OS_WIN)
  wchar_t utf16_path[MAX_PATH] = {0};
  if (::SHGetSpecialFolderPathW(NULL, utf16_path,
                                CSIDL_PROFILE, FALSE) == FALSE)
    return GetCurrentDirectory();
  std::string utf8_path;
  ConvertStringUTF16ToUTF8(utf16_path, wcslen(utf16_path), &utf8_path);
  return utf8_path;
#elif defined(OS_POSIX)
  const char * home = 0;
  struct passwd *pw;

  setpwent ();
  pw = getpwuid(getuid());
  endpwent ();

  if (pw)
    home = pw->pw_dir;

  if (!home)
    home = getenv("HOME");

  // If failed to get home directory, then use current directory.
  return home ? std::string(home) : GetCurrentDirectory();
#endif
}

std::string GetAbsolutePath(const char *path) {
  if (!path || !*path)
    return "";
#if defined(OS_WIN)
  if (IsAbsolutePath(path))
    return path;
  wchar_t utf16_full_path[MAX_PATH] = {0};
  wchar_t utf16_current_path[MAX_PATH] = {0};
  if (::GetCurrentDirectoryW(MAX_PATH, utf16_current_path) > 0) {
    std::wstring utf16_path;
    ConvertStringUTF8ToUTF16(path, strlen(path), &utf16_path);
    ::PathCombineW(utf16_full_path, utf16_current_path, utf16_path.c_str());
    std::string full_path;
    ConvertStringUTF16ToUTF8(utf16_full_path, &full_path);
    return full_path;
  }
  return "";
#elif defined(OS_POSIX)
  std::string result = path;
  // Not using kDirSeparator because Windows version should have more things
  // to do than simply replace the path separator.
  if (result[0] != '/') {
    std::string current_dir = GetCurrentDirectory();
    if (current_dir.empty())
      return "";
    result = current_dir + "/" + result;
  }
  result = NormalizeFilePath(result.c_str());
  return result;
#endif
}

bool IsAbsolutePath(const char *path) {
#if defined(OS_WIN)
  std::wstring utf16_path;
  ConvertStringUTF8ToUTF16(path, strlen(path), &utf16_path);
  return ::PathIsRelativeW(utf16_path.c_str()) == FALSE;
#elif defined(OS_POSIX)
  // Other system may use other method.
  return path && *path == '/';
#endif
}

bool CreateTempDirectory(const char *prefix, std::string *path) {
  ASSERT(path);
  bool result = false;
#if defined(OS_WIN)
  wchar_t utf16_temp_dir_root[MAX_PATH] = {0};
  if (::GetTempPathW(MAX_PATH, utf16_temp_dir_root) == 0)
    return false;
  std::string temp_dir_root_string;
  ConvertStringUTF16ToUTF8(utf16_temp_dir_root,
                           wcslen(utf16_temp_dir_root),
                           &temp_dir_root_string);
  const char* temp_dir_root = temp_dir_root_string.c_str();
  size_t len = temp_dir_root_string.size() + (prefix ? strlen(prefix) : 0) + 20;
#elif defined(OS_POSIX)
  static const char temp_dir_root[] = "/tmp/";
  size_t len = (prefix ? strlen(prefix) : 0) + 20;
#endif
  char *buf = new char[len];

#ifdef HAVE_MKDTEMP
  snprintf(buf, len, "%s%s-XXXXXX", temp_dir_root, (prefix ? prefix : ""));

  result = (::mkdtemp(buf) == buf);
  if (result && path)
    *path = std::string(buf);
#else
  while (true) {
    snprintf(buf, len, "%s%s-%06X", temp_dir_root, (prefix ? prefix : ""),
             ::rand() & 0xFFFFFF);
    if (ggadget::access(buf, F_OK) == 0)
      continue;

    if (errno == ENOENT) {
      // The temp name is available.
      result = (ggadget::mkdir(buf, 0700) == 0);
      if (result && path)
        *path = std::string(buf);
    }

    break;
  }
#endif

  delete[] buf;
  return result;
}

#if defined(OS_WIN)
namespace {
bool RemoveDirectoryInternal(const wchar_t *utf16_dir, bool remove_readonly) {
  if (!utf16_dir || !*utf16_dir)
    return false;
  WIN32_FIND_DATAW file_data = {0};
  wchar_t itf16_search_path[MAX_PATH];
  ::PathCombineW(itf16_search_path, utf16_dir, L"*");
  HANDLE handle = ::FindFirstFileW(itf16_search_path, &file_data);
  if (handle != INVALID_HANDLE_VALUE) {
    do {
      wchar_t utf16_file_path[MAX_PATH] = {0};
      ::PathCombineW(utf16_file_path, utf16_dir, file_data.cFileName);
      if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (*file_data.cFileName != L'.') {  // Skips itself(.) and parent(..)
          if (!RemoveDirectoryInternal(utf16_file_path, remove_readonly)) {
            ::FindClose(handle);
            return false;
          }
        }
      } else {
        if (file_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
          if (!remove_readonly) {
            ::FindClose(handle);
            return false;
          }
          DWORD attributes =
              file_data.dwFileAttributes & (~FILE_ATTRIBUTE_READONLY);
          if (!::SetFileAttributesW(utf16_file_path, attributes)) {
            ::FindClose(handle);
            return false;
          }
        }
        if (!::DeleteFileW(utf16_file_path)) {
          FindClose(handle);
          return false;
        }
      }
    } while (::FindNextFileW(handle, &file_data));
    ::FindClose(handle);
    if (!::RemoveDirectoryW(utf16_dir)) {
      // It should not fail generally. But sometimes, OS may think the folder
      // is not empty if we delete the folder right after deleting files in it.
      // So, wait for a while and try to remove it again.
      ::Sleep(100);
      if (!::RemoveDirectoryW(utf16_dir)) {
#ifdef _DEBUG
        std::string dir;
        ConvertStringUTF16ToUTF8(utf16_dir, wcslen(utf16_dir), &dir);
        DLOG("Cannot Remove directory %s: %d", dir.c_str(), GetLastError());
#endif
        return false;
      }
    }
    return true;
  } else {
#ifdef _DEBUG
    std::string dir;
    ConvertStringUTF16ToUTF8(utf16_dir, wcslen(utf16_dir), &dir);
    DLOG("Cannot to list directory %s: %h.", dir.c_str(), GetLastError());
#endif
    // Enumeration non-exist directory succeeds with empty result.
    return true;
  }
}
}  // namespace
bool RemoveDirectory(const char *path, bool remove_readonly_files) {
  if (!path || !*path)
    return false;
  std::string dir_path = NormalizeFilePath(path);
  std::wstring utf16_dir_path;
  ConvertStringUTF8ToUTF16(dir_path, &utf16_dir_path);
  return RemoveDirectoryInternal(utf16_dir_path.c_str(), remove_readonly_files);
}
#elif defined(OS_POSIX)
bool RemoveDirectory(const char *path, bool remove_readonly_files) {
  if (!path || !*path)
    return false;

  std::string dir_path = NormalizeFilePath(path);

  if (dir_path == kDirSeparatorStr) {
    DLOG("Can't remove the whole root directory.");
    return false;
  }

  DIR *pdir = opendir(dir_path.c_str());
  if (!pdir) {
    DLOG("Can't read directory: %s", path);
    return false;
  }

  struct dirent *pfile = NULL;
  while ((pfile = readdir(pdir)) != NULL) {
    if (strcmp(pfile->d_name, ".") != 0 &&
        strcmp(pfile->d_name, "..") != 0) {
      std::string file_path =
          BuildFilePath(dir_path.c_str(), pfile->d_name, NULL);
      struct stat file_stat;
      bool result = false;
      if (!remove_readonly_files && ::access(file_path.c_str(), W_OK) != 0)
        return false;
      // Don't use dirent.d_type, it's a non-standard field.
      if (::lstat(file_path.c_str(), &file_stat) == 0) {
        if (S_ISDIR(file_stat.st_mode))
          result = RemoveDirectory(file_path.c_str(), remove_readonly_files);
        else
          result = (::unlink(file_path.c_str()) == 0);
      }
      if (!result) {
        closedir(pdir);
        return false;
      }
    }
  }

  closedir(pdir);
  return ::rmdir(dir_path.c_str()) == 0;
}
#endif

bool GetSystemLocaleInfo(std::string *language, std::string *territory) {
#if defined(OS_WIN)
  // We don't need this method in Windows.
  GGL_UNUSED(language);
  GGL_UNUSED(territory);
  ASSERT(false);
  return false;
#elif defined(OS_POSIX)
  char *locale = setlocale(LC_MESSAGES, NULL);
  if (!locale || !*locale) return false;

  // We don't want to support these standard locales.
  if (strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0) {
    DLOG("Probably setlocale() was not called at beginning of the program.");
    return false;
  }

  std::string locale_str(locale);

  // Remove encoding and variant part.
  std::string::size_type pos = locale_str.find('.');
  if (pos != std::string::npos)
    locale_str.erase(pos);

  pos = locale_str.find('_');
  if (language)
    language->assign(locale_str, 0, pos);

  if (territory) {
    if (pos != std::string::npos)
      territory->assign(locale_str, pos + 1, std::string::npos);
    else
      territory->clear();
  }
  return true;
#endif
}

void Daemonize() {
  // FIXME: How about other systems?
#ifdef GGL_HOST_LINUX
  if (daemon(0, 0) != 0) {
    LOGE("Failed to daemonize.");
  }
#endif
}

bool CopyFile(const char *src, const char *dest) {
  ASSERT(src && dest);
  if (!src || !dest)
    return false;

  if (strcmp(src, dest) == 0)
    return true;

#if defined(OS_WIN)
  std::wstring utf16_src, utf16_dest;
  ConvertStringUTF8ToUTF16(src, strlen(src), &utf16_src);
  ConvertStringUTF8ToUTF16(dest, strlen(dest), &utf16_dest);
  return ::CopyFileW(utf16_src.c_str(), utf16_dest.c_str(), FALSE) == TRUE;
#elif defined(OS_POSIX)
  FILE *in_fp = fopen(src, "rb");
  if (!in_fp) {
    LOG("Can't open file %s for reading.", src);
    return false;
  }

  FILE *out_fp = fopen(dest, "wb");
  if (!out_fp) {
    LOG("Can't open file %s for writing.", dest);
    fclose(in_fp);
    return false;
  }

  const size_t kChunkSize = 8192;
  char buffer[kChunkSize];
  bool result = true;
  while (true) {
    size_t read_size = fread(buffer, 1, kChunkSize, in_fp);
    if (read_size) {
      if (fwrite(buffer, read_size, 1, out_fp) != 1) {
        LOG("Error when writing to file %s", dest);
        result = false;
        break;
      }
    }
    if (read_size < kChunkSize)
      break;
  }

  if (ferror(in_fp)) {
    LOG("Error when reading file: %s", src);
    result = false;
  }

  fclose(in_fp);
  result = (fclose(out_fp) == 0 && result);

  if (!result)
    unlink(dest);

  return result;
#endif
}

std::string GetFullPathOfSystemCommand(const char *command) {
  if (IsAbsolutePath(command))
    return std::string(command);

  const char *env_path_value = getenv("PATH");
  if (env_path_value == NULL)
    return "";

  StringVector paths;
  SplitStringList(env_path_value, ":", &paths);
  for (StringVector::iterator i = paths.begin();
       i != paths.end(); ++i) {
    std::string path = BuildFilePath(i->c_str(), command, NULL);
    if (ggadget::access(path.c_str(), X_OK) == 0)
      return path;
  }
  return "";
}

static std::string GetSystemGadgetPathInResourceDir(const char *resource_dir,
                                                    const char *basename) {
  std::string path;
  FileManagerInterface *file_manager = GetGlobalFileManager();
  path = BuildFilePath(resource_dir, basename, NULL) + kGadgetFileSuffix;
  if (file_manager->FileExists(path.c_str(), NULL) &&
      file_manager->IsDirectlyAccessible(path.c_str(), NULL))
    return file_manager->GetFullPath(path.c_str());

  path = BuildFilePath(resource_dir, basename, NULL);
  if (file_manager->FileExists(path.c_str(), NULL) &&
      file_manager->IsDirectlyAccessible(path.c_str(), NULL))
    return file_manager->GetFullPath(path.c_str());

  return std::string();
}

std::string GetSystemGadgetPath(const char *basename) {
  std::string result;
#ifdef _DEBUG
  // Try current directory first in debug mode, to ease in place build/debug.
  result = GetSystemGadgetPathInResourceDir(".", basename);
  if (!result.empty())
    return result;
#endif
#ifdef GGL_RESOURCE_DIR
  result = GetSystemGadgetPathInResourceDir(GGL_RESOURCE_DIR, basename);
#endif
  return result;
}

#if defined(OS_WIN)
static const int kMaxUserNameLength = UNLEN + 1;
#endif

std::string GetUserRealName() {
#if defined(OS_WIN)
  // TODO(zkfan): what is the user real name?
  return GetUserLoginName();
#elif defined(OS_POSIX)
  struct passwd *pw;
  setpwent();
  pw = getpwuid(getuid());
  endpwent();
  return pw ? pw->pw_gecos : "";
#endif
}

std::string GetUserLoginName() {
#if defined(OS_WIN)
  wchar_t utf16_user_name[kMaxUserNameLength] = {0};
  DWORD user_name_length = kMaxUserNameLength;
  if (::GetUserNameW(utf16_user_name, &user_name_length)) {
    std::string user_name;
    ConvertStringUTF16ToUTF8(utf16_user_name,
                             wcslen(utf16_user_name),
                             &user_name);
    return user_name;
  }
  return "";
#elif defined(OS_POSIX)
  struct passwd *pw;
  setpwent();
  pw = getpwuid(getuid());
  endpwent();
  return pw ? pw->pw_name : "";
#endif
}

}  // namespace ggadget
