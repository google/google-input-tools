/*
  Copyright 2014 Google Inc.

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
//
// This file contains the implementation of VersionReader class. This class
// provides helper method to retieve the version information of the
// specified execuable file. The information includes the size and last
// modificated date, as well as the build version in the meta data. These
// information would be used to determine whether the running application
// is the target application to apply special rules.

#include "appsensorapi/versionreader.h"
#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include "base/logging.h"
#include "base/stringprintf.h"
#include "base/string_utils_win.h"
#include "appsensorapi/common.h"

namespace ime_goopy {
// Define the translation string for use in version reading.
static const TCHAR *kTranslationString = TEXT("\\VarFileInfo\\Translation");
// Define the file info string for use in version reading.
static const TCHAR *kFileInfoString = TEXT("\\StringFileInfo\\%04x%04x\\%s");

// Obtain the version info from the specified filename.
// filename: the file to obtain version information from
// version_info: the version info structure to store at
// retval: return TRUE if success, FALSE if error occurs.
BOOL VersionReader::GetVersionInfo(const TCHAR *filename,
                                   VersionInfo* version_info) {
  DCHECK(filename != NULL);
  DCHECK(version_info != NULL);

  // Obtain the size and last modified time of the given file.
  struct __stat64 stat_buffer;
  if (_tstat64(filename, &stat_buffer) == 0) {
    version_info->file_size = stat_buffer.st_size;
    version_info->modified_time = stat_buffer.st_mtime;
  }

  // Calculate the size needed to store the version info.
  DWORD version_info_size = ::GetFileVersionInfoSize(filename, NULL);
  if (version_info_size > 0) {
    // Allocate memory for the version info.
    scoped_ptr<TCHAR> file_info(new TCHAR[version_info_size]);
    // Obtain the info and store in file_info.
    if (!::GetFileVersionInfo(filename, NULL, version_info_size,
                              file_info.get())) {
      DLOG(WARNING) << "Cannot get file info, filename: " << filename;
      return FALSE;
    }
    // Obtain the translation information.
    UINT lang;
    WORD *lang_info;
    if (!::VerQueryValue(file_info.get(),
                         const_cast<LPTSTR>(kTranslationString),
                         reinterpret_cast<LPVOID*>(&lang_info), &lang)) {
      DLOG(WARNING) << "Cannot get translation info, filename: " << filename;
      return FALSE;
    }
    // Obtain each subsection from file_info and store in our data map.
    for (int i = 0; i < arraysize(FileInfoKey::kAllInfoStrings); i++) {
      GetSubBlockInfo(file_info.get(),
                      lang_info,
                      FileInfoKey::kAllInfoStrings[i],
                      &version_info->file_info);
    }
  }
  return TRUE;
}

// Obtain one predefined version info specified by sub_block.
// The result will be store at the map.
// retval: return TRUE if success, FALSE if error occurs.
BOOL VersionReader::GetSubBlockInfo(const TCHAR *file_info,
                                    const WORD *lang_info,
                                    const TCHAR *sub_block,
                                    FileInfoMap *file_info_map) {
  wstring version_string_name;
  wstring buffer;
  UINT buffer_size = 0;
  LPVOID block_info = NULL;

  DCHECK(file_info != NULL);
  DCHECK(lang_info != NULL);
  DCHECK(sub_block != NULL);

  // Construct the file info string from the language information.
  version_string_name = WideStringPrintf(kFileInfoString, lang_info[0],
      lang_info[1], sub_block);
  // Retrieve the block information of specified section
  if (!::VerQueryValue(const_cast<TCHAR *>(file_info),
                       const_cast<LPWSTR>(version_string_name.c_str()),
                       &block_info,
                       &buffer_size)) {
    return FALSE;
  }
  (*file_info_map)[sub_block].assign(static_cast<LPTSTR>(block_info));
  return TRUE;
}
}  // namespace ime_goopy
