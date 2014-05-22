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

#ifndef GOOPY_BASE_SCOPED_PTR_H_
#define GOOPY_BASE_SCOPED_PTR_H_
#include <memory>

#if !defined(_SCOPED_PTR_)
#define _SCOPED_PTR_
template<typename T>
class scoped_ptr : public std::unique_ptr<T> {
 public:
 scoped_ptr() : std::unique_ptr<T>() {
  }
  explicit scoped_ptr(T* t) : std::unique_ptr<T>(t) {
  }
};
template<typename T>
class scoped_ptr<T[]> : public std::unique_ptr<T> {
 public:
  scoped_ptr() : std::unique_ptr<T>() {
  }
    explicit scoped_ptr(T* t) : std::unique_ptr<T>(t) {
  }
};
#endif  // _SCOPED_PTR_

#if !defined(_SCOPED_ARRAY_)
#define _SCOPED_ARRAY_
template<typename T>
class scoped_array : public scoped_ptr<T[]> {
 public:
  scoped_array() : scoped_ptr<T[]>() {
  }
  explicit scoped_array(T* t) : scoped_ptr<T[]>(t) {
  }
  T& operator[](int index) {
    return get()[index];
  }
};
#endif
#endif  // GOOPY_BASE_SCOPED_PTR_H_
