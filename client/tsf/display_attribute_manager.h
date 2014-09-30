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

#ifndef GOOPY_TSF_DISPLAY_ATTRIBUTE_MANAGER_H_
#define GOOPY_TSF_DISPLAY_ATTRIBUTE_MANAGER_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>
#include <vector>
#include "common/smart_com_ptr.h"
#include "common/framework_interface.h"

namespace ime_goopy {
namespace tsf {
// DisplayAttributeManager class maintain display attribute enumerator and
// informations. Display attributes are supported by TSF to change the display
// style of the text in host application.
class DisplayAttributeManager {
 public:
  HRESULT GetEnumerator(IEnumTfDisplayAttributeInfo** enumerator);
  HRESULT GetAttribute(REFGUID guid, ITfDisplayAttributeInfo** info);

  // Following functions must be called within an edit session, because a
  // read / write edit cookie is required.
  HRESULT ApplyInputAttribute(ITfContext* context,
                              ITfRange* range,
                              TfEditCookie cookie,
                              int index);
  HRESULT ClearAttribute(ITfContext* context,
                         ITfRange* range,
                         TfEditCookie cookie);
 private:
  template <class T> friend class Singleton;
  struct AttributeEntry {
    GUID guid;
    TfGuidAtom atom;
    TF_DISPLAYATTRIBUTE attr;
    SmartComPtr<ITfDisplayAttributeInfo> info;
  };

  // Information subclass is used to provide information for a specific
  // display attribute. This is a COM object, TSF create this object by
  // ITfDisplayAttributeProvider interface.
  class ATL_NO_VTABLE Information;
  // Enumerator subclass is provided for TSF to get all display attributes
  // in this text service. This is a COM object, TSF create this object by
  // ITfDisplayAttributeProvider interface.
  class ATL_NO_VTABLE Enumerator;

  DisplayAttributeManager();
  ~DisplayAttributeManager();

  HRESULT Initialize();

  // Makes a TF_DISPLAYATTRIBUTE structure from a TextStyle.
  void MakeDisplayAttribute(const TextStyle &style, TF_DISPLAYATTRIBUTE *attr);

  SmartComPtr<IEnumTfDisplayAttributeInfo> enumerator_;
  std::vector<AttributeEntry> entries_;
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_DISPLAY_ATTRIBUTE_MANAGER_H_
