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

#ifndef GOOPY_SKIN_SKIN_LIBRARY_INITIALIZER_H_
#define GOOPY_SKIN_SKIN_LIBRARY_INITIALIZER_H_

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace ime_goopy {
namespace skin {

// A class that contains the global initializations of the skin library.
// you need to call SkinLibraryInitializer::Initialize before loading skin and
// call UnIntialize before the thread terminates and after all skin operations.
// These initializations should only be call once for each thread.
class SkinLibraryInitializer {
 public:
  // Returns true is initialize success or already initialized.
  static bool Initialize();
  static void Finalize();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SkinLibraryInitializer);
};

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_SKIN_LIBRARY_INITIALIZER_H_
