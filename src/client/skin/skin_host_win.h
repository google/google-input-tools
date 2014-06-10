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
#ifndef GOOPY_SKIN_SKIN_HOST_WIN_H_
#define GOOPY_SKIN_SKIN_HOST_WIN_H_

#include "base/basictypes.h"
#include "skin/skin_host.h"
#include "third_party/google_gadgets_for_linux/ggadget/host_interface.h"
#include "third_party/google_gadgets_for_linux/ggadget/win32/private_font_database.h"

namespace ggadget {

class GadgetInterface;
class ViewHostInterface;

}  // namespace ggadget

namespace ime_goopy {
namespace skin {

class Skin;

class SkinHostWin : public SkinHost {
 public:
  SkinHostWin();
  virtual ~SkinHostWin();

  virtual ggadget::ViewHostInterface* NewViewHost(
      ggadget::GadgetInterface* gadget,
      ggadget::ViewHostInterface::Type type);

  virtual bool LoadFont(const char* filename);

  virtual bool OpenURL(const ggadget::GadgetInterface* gadget,
                       const char* url);

 private:
  ggadget::win32::PrivateFontDatabase private_font_database_;
  DISALLOW_COPY_AND_ASSIGN(SkinHostWin);
};

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_SKIN_HOST_WIN_H_
