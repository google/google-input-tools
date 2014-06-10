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
// Precompile header for platform-independent targets.

#ifndef GOOPY_COMMON_PRECOMPILE_WIN_H_
#define GOOPY_COMMON_PRECOMPILE_WIN_H_

// w/o _ATL_NO_HOSTING, <atlwin.h> includes headers as follows,
//   <atlhost.h> -> <mshtml.h> -> <dimm.h>
// where <dimm.h> may conflict with "immdev.h".
// TODO(zengjian): Remove duplicated part in immdev.h and dimm.h
// and avoid this trick.
#ifndef _ATL_NO_HOSTING
#define _ATL_NO_HOSTING
#endif

// In order to use windows.h and winsock2.h at the same time and place
// windows.h in front of winsock2.h, we need to define _WINSOCKAPI_ before
// including windows.h to avoid including winsock.h in windows.h.
// FIXME(linxinfen)
//#define WIN32_LEAN_AND_MEAN
#define _WINSOCKAPI_
// Common windows headres
#include <windows.h>
#include <winsock2.h>

// C++ headers
#include <algorithm>
#include <cstdio>
#include <functional>
#include <hash_map>
#include <map>
#include <string>
#include <set>
#include <utility>
#include <vector>

// Goopy headers. Only add files that do not change often.
#include "base/basictypes.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/scoped_handle.h"
#include "base/scoped_handle_win.h"
#include "base/stringprintf.h"
#include "base/string_utils_win.h"
#include "common/atl.h"
#include "common/debug.h"
#include "common/linked_ptr.h"
#include "common/mutex.h"
#include "common/scoped_lock.h"
#include "common/smart_com_ptr.h"

#endif  // GOOPY_COMMON_PRECOMPILE_WIN_H_
