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

#ifndef GOOPY_TSF_DISPLAY_ATTRIBUTE_H__
#define GOOPY_TSF_DISPLAY_ATTRIBUTE_H__

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>

namespace ime_goopy {
namespace tsf {
// DisplayAttribute class maintain display attribute enumerator and
// informations. Display attributes are supported by TSF to change the display
// style of the text in host application.
class DisplayAttribute {
 public:
  // Information sub class is used to provide information for a specific
  // display attribute. This is a COM object, TSF create this object by
  // ITfDisplayAttributeProvider interface.
  class ATL_NO_VTABLE Information
      : public CComObjectRootEx<CComSingleThreadModel>,
        public ITfDisplayAttributeInfo {
   public:
    DECLARE_NOT_AGGREGATABLE(Information)
    BEGIN_COM_MAP(Information)
      COM_INTERFACE_ENTRY(ITfDisplayAttributeInfo)
    END_COM_MAP()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    // ITfDisplayAttributeInfo
    STDMETHODIMP GetGUID(GUID* guid);
    STDMETHODIMP GetDescription(BSTR* description);
    STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE* attribute);
    STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE* attribute);
    STDMETHODIMP Reset();
  };

  // Enumerator sub class is provided for TSF to get all display attributes
  // in this text service. This is a COM object, TSF create this object by
  // ITfDisplayAttributeProvider interface.
  class ATL_NO_VTABLE Enumerator
      : public CComObjectRootEx<CComSingleThreadModel>,
        public IEnumTfDisplayAttributeInfo {
   public:
    DECLARE_NOT_AGGREGATABLE(Enumerator)
    BEGIN_COM_MAP(Enumerator)
      COM_INTERFACE_ENTRY(IEnumTfDisplayAttributeInfo)
    END_COM_MAP()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    Enumerator();

    // IEnumTfDisplayAttributeInfo
    STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo** enum_info);
    STDMETHODIMP Next(ULONG count,
                      ITfDisplayAttributeInfo** info,
                      ULONG* fetched);
    STDMETHODIMP Reset();
    STDMETHODIMP Skip(ULONG count);
   private:
    LONG index_;
  };

  DisplayAttribute();
  ~DisplayAttribute();

  HRESULT Initialize();

  static HRESULT CreateEnumerator(
      IEnumTfDisplayAttributeInfo** enumerator);
  static HRESULT CreateAttribute(REFGUID guid,
                                 ITfDisplayAttributeInfo** info);

  // Following functions must be called within an edit session, because a
  // read / write edit cookie is required.
  HRESULT ApplyInputAttribute(ITfContext* context,
                              ITfRange* range,
                              TfEditCookie cookie);
  HRESULT ClearAttribute(ITfContext* context,
                         ITfRange* range,
                         TfEditCookie cookie);

 private:
  TfGuidAtom input_atom_;
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_DISPLAY_ATTRIBUTE_H__
