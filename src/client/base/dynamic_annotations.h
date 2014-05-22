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

// This file includes the corresponding file from third_party.
// In debug mode (NDEBUG not defined) the dynamic annotations are enabled;
// in opt mode they are disabled.

#ifndef BASE_DYNAMIC_ANNOTATIONS_H_
#define BASE_DYNAMIC_ANNOTATIONS_H_

// We always disable dynamic annotations under googleclient.
#define DYNAMIC_ANNOTATIONS_ENABLED 0
#define NVALGRIND 1
#include "third_party/dynamic_annotations/dynamic_annotations.h"  // NOLINT
#undef NVALGRIND
#undef DYNAMIC_ANNOTATIONS_ENABLED

#endif  // BASE_DYNAMIC_ANNOTATIONS_H_
