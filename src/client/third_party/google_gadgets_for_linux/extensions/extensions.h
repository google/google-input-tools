/*
  Copyright 2013 Google Inc.

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

#ifndef EXTENSIONS_EXTENSIONS_H_
#define EXTENSIONS_EXTENSIONS_H_

// The extensions in Google Gadgets for Linux are designed to be loaded as
// dynamic library modules. In Goopy we need to link and initialize these
// extensions statically. Since the extensions don't provide .h files and we
// don't need all of them, we provide this header and implementation to
// initialize the extensions that we need in Goopy.

namespace ggadget {
namespace extensions {

bool Initialize();
void Finalize();

} // extensions
} // ggadget

#endif // EXTENSIONS_EXTENSIONS_H_
