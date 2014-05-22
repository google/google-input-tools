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

#ifndef GOOPY_COMMON_EXCEPTION_H__
#define GOOPY_COMMON_EXCEPTION_H__

namespace google_breakpad {
class ExceptionHandler;
}

namespace ime_goopy {
class ExceptionHandler {
 public:
  static bool Init();
  static void Release();

 private:
  // No instances of this class should be created.
  // Disallow all constructors, destructors, and operator=.
  ExceptionHandler();
  explicit ExceptionHandler(const ExceptionHandler &);
  ~ExceptionHandler();
  void operator=(const ExceptionHandler &);
  static google_breakpad::ExceptionHandler* handler_;
};
}  // namespace ime_goopy
#endif  // GOOPY_COMMON_EXCEPTION_H__
