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

#ifndef GOOPY_BASE_POSIX_ENDIAN_H_
#define GOOPY_BASE_POSIX_ENDIAN_H_

#define __BIG_ENDIAN 4321
#define __LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#define LITTLE_ENDIAN 1234

#if defined(_M_IX86) || defined(_M_AMD64)

#define __BYTE_ORDER __LITTLE_ENDIAN
#define __FLOAT_WORD_ORDER __LITTLE_ENDIAN

#else
#error Unsupported processor family.
#endif

#endif  // GOOPY_BASE_POSIX_ENDIAN_H_
