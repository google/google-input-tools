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
#ifndef GOOPY_SKIN_SKIN_HOST_MAC_H_
#define GOOPY_SKIN_SKIN_HOST_MAC_H_

#include "skin/skin_host.h"

#include "ggadget/host_interface.h"

namespace ime_goopy {
namespace skin {

class Skin;

class SkinHostMac : public SkinHost {
 public:
  SkinHostMac();
  virtual ~SkinHostMac();

  virtual ggadget::ViewHostInterface* NewViewHost(
      ggadget::GadgetInterface* gadget,
      ggadget::ViewHostInterface::Type type);

  virtual bool LoadFont(const char* filename);

  virtual bool OpenURL(const ggadget::GadgetInterface* gadget,
                       const char* url);

 private:
  DISALLOW_COPY_AND_ASSIGN(SkinHostMac);
};

}  // namespace skin
}  // namespace ime_goopy

#endif  // GOOPY_SKIN_SKIN_HOST_MAC_H_
