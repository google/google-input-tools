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

// This file contains the declaration of VersionReader class. This class
// provides helper method to retieve the version information of the
// specified execuable file. The information includes the size and last
// modificated date, as well as the build version in the meta data. These
// information would be used to determine whether the running application
// is the target application to apply special rules.

#ifndef GOOPY_APPSENSORAPI_VERSIONREADER_H__
#define GOOPY_APPSENSORAPI_VERSIONREADER_H__

#include <map>
#include "base/basictypes.h"

namespace ime_goopy {
typedef map<wstring, wstring> FileInfoMap;
struct VersionInfo {
  // The size of the file.
  size_t file_size;
  // The last modified time of the file.
  // See __stat.st_mtime for reference.
  uint64 modified_time;
  // A map containing any of the predefined version info
  FileInfoMap file_info;
};

class VersionReader {
 public:
  // Obtain the version info from the specified filename.
  // filename: the file to obtain version information from
  // version_info: the version info structure to store at
  // retval: return true if success, false if error occurs.
  static BOOL GetVersionInfo(const TCHAR *filename, VersionInfo *version_info);
 private:
  // Obtain one predefined version info specified by sub_block.
  // The result will be store at the map.
  // retval: return true if success, false if error occurs.
  static BOOL GetSubBlockInfo(const TCHAR *file_info,
                              const WORD *lang_info,
                              const TCHAR *sub_block,
                              FileInfoMap *file_info_map);

  DISALLOW_EVIL_CONSTRUCTORS(VersionReader);
};
}

#endif  // GOOPY_APPSENSORAPI_VERSIONREADER_H__
