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

#ifndef GOOPY_COMMON_SINGLETON_H_
#define GOOPY_COMMON_SINGLETON_H_

#include "base/mutex.h"

namespace ime_goopy {
namespace tsf {

template <class T>
class Singleton {
 public:
  static T *GetInstance() {
    if (instance_)
      return instance_;

    MutexLock lock(&mutex_);
    if (instance_)
      return instance_;
    instance_ = new T();
    return instance_;
  }
  static void ClearInstance() {
    MutexLock lock(&mutex_);
    if (instance_) {
      delete instance_;
      instance_ = NULL;
    }
  }
 private:
  static T *instance_;
  static Mutex mutex_;
};
template <class T> T *Singleton<T>::instance_ = NULL;
template <class T> Mutex Singleton<T>::mutex_(NULL);

}  // namespace tsf
}  // namespace ime_goopy

#endif  // GOOPY_COMMON_SINGLETON_H_
