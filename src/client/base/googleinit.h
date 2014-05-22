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

#ifndef BASE_GOOGLEINIT_H_
#define BASE_GOOGLEINIT_H_

class GoogleInitializer {
 public:
  typedef void (*void_function)(void);
  GoogleInitializer(const char* name, void_function f) {
    f();
  }
};

#define REGISTER_MODULE_INITIALIZER(name, body)                 \
  namespace {                                                   \
    static void google_init_module_##name() { body; }           \
    GoogleInitializer google_initializer_module_##name(#name,   \
        google_init_module_##name);                             \
  }

#endif  // BASE_GOOGLEINIT_H_
