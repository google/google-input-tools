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

#ifndef GGADGET_NPAPI_NPAPI_WRAPPER_H__
#define GGADGET_NPAPI_NPAPI_WRAPPER_H__

#define MDCPUCFG <ggadget/sysdeps.h>
#define PR_BYTES_PER_BYTE GGL_SIZEOF_CHAR
#define PR_BYTES_PER_SHORT GGL_SIZEOF_SHORT
#define PR_BYTES_PER_INT GGL_SIZEOF_INT
#define PR_BYTES_PER_LONG GGL_SIZEOF_LONG_INT

#ifndef XP_UNIX
#define XP_UNIX
#endif

#ifdef HAVE_X11
#ifndef MOZ_X11
#define MOZ_X11
#endif
#endif

#include <third_party/npapi/npapi.h>
#include <third_party/npapi/npupp.h>

#endif // GGADGET_NPAPI_NPAPI_WRAPPER_H__
