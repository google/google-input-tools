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

#include "tsf/display_attribute.h"
#include "common/framework_interface.h"

namespace ime_goopy {
namespace tsf {
// We only support one display attribute for now. It's the dot underline style
// when composing.
static const wchar_t kInputAttributeDescription[] = L"GPY Input Attribute";
static const TF_DISPLAYATTRIBUTE kInputAttribute = {
  {TF_CT_NONE, 0},  // text color
  {TF_CT_NONE, 0},  // background color
  TF_LS_DOT,        // underline style
  FALSE,            // underline boldness
  {TF_CT_NONE, 0},  // underline color
  TF_ATTR_INPUT     // attribute info
};


// DisplayAttribute::Information

STDAPI DisplayAttribute::Information::GetGUID(GUID* guid) {
  if (!guid) return E_INVALIDARG;
  *guid = InputMethod::kInputAttributeGUID;
  return S_OK;
}

STDAPI DisplayAttribute::Information::GetDescription(BSTR* description) {
  if (!description) return E_INVALIDARG;
  *description = SysAllocString(kInputAttributeDescription);
  return *description ? S_OK : E_OUTOFMEMORY;
}

STDAPI DisplayAttribute::Information::GetAttributeInfo(
    TF_DISPLAYATTRIBUTE* attribute) {
  if (!attribute) return E_INVALIDARG;
  *attribute = kInputAttribute;
  return S_OK;
}

// We don't support modification of attributes.
STDAPI DisplayAttribute::Information::SetAttributeInfo(
    const TF_DISPLAYATTRIBUTE *attribute) {
  return E_NOTIMPL;
}

// We don't support modification of attributes.
STDAPI DisplayAttribute::Information::Reset() {
  return E_NOTIMPL;
}


// DisplayAttribute::Enumerator
DisplayAttribute::Enumerator::Enumerator() : index_(0) {
}

// Returns a copy of the object. Internal index should be copied too.
STDAPI DisplayAttribute::Enumerator::Clone(
    IEnumTfDisplayAttributeInfo** enum_info) {
  if (!enum_info) return E_INVALIDARG;
  *enum_info = NULL;

  CComObject<Enumerator>* object = NULL;
  HRESULT hr = CComObject<Enumerator>::CreateInstance(&object);
  if (FAILED(hr)) return hr;
  object->AddRef();
  object->index_ = index_;
  *enum_info = object;

  return S_OK;
}

// Returns an array of display attribute info objects.
// Return S_FALSE if reached the end of the enumeration.
STDAPI DisplayAttribute::Enumerator::Next(ULONG count,
                                          ITfDisplayAttributeInfo** info,
                                          ULONG *fetched) {
  if (!count) return S_OK;
  if (!info) return E_INVALIDARG;
  if (index_ == 1) return S_FALSE;

  CComObject<Information>* object = NULL;
  HRESULT hr = CComObject<Information>::CreateInstance(&object);
  if (FAILED(hr)) return hr;
  object->AddRef();
  *info = object;

  index_++;
  if (fetched) *fetched = 1;
  return S_OK;
}

// Resets the enumeration.
STDAPI DisplayAttribute::Enumerator::Reset() {
  index_ = 0;
  return S_OK;
}

// Skips past objects in the enumeration. Return S_FALSE if we reached the end
// of the enumeration.
STDAPI DisplayAttribute::Enumerator::Skip(ULONG count) {
  if (!count) return S_OK;
  if (index_ != 0) return S_FALSE;
  index_++;
  return S_OK;
}

DisplayAttribute::DisplayAttribute() : input_atom_(TF_INVALID_GUIDATOM) {
}

DisplayAttribute::~DisplayAttribute() {
}

HRESULT DisplayAttribute::CreateEnumerator(
    IEnumTfDisplayAttributeInfo** enumerator) {
  if (!enumerator) return E_INVALIDARG;

  CComObject<Enumerator>* object = NULL;
  HRESULT hr = CComObject<Enumerator>::CreateInstance(&object);
  if (FAILED(hr)) return hr;
  object->AddRef();
  *enumerator = object;
  return S_OK;
}

HRESULT DisplayAttribute::CreateAttribute(REFGUID guid,
                                          ITfDisplayAttributeInfo** info) {
  if (!info) return E_INVALIDARG;
  *info = NULL;

  if (IsEqualGUID(guid, InputMethod::kInputAttributeGUID)) {
    CComObject<Information>* object = NULL;
    HRESULT hr = CComObject<Information>::CreateInstance(&object);
    if (FAILED(hr)) return hr;
    object->AddRef();
    *info = object;
    return S_OK;
  }

  return E_INVALIDARG;
}

HRESULT DisplayAttribute::Initialize() {
  // Register the attribute guid, so we can refer to this attribute using an
  // integer from now on.
  CComPtr<ITfCategoryMgr> category_manager;
  HRESULT hr = category_manager.CoCreateInstance(CLSID_TF_CategoryMgr);
  if (FAILED(hr)) return hr;
  hr = category_manager->RegisterGUID(InputMethod::kInputAttributeGUID,
                                      &input_atom_);
  return hr;
}

// Apply input attribute to a range of text.
HRESULT DisplayAttribute::ApplyInputAttribute(ITfContext* context,
                                              ITfRange* range,
                                              TfEditCookie cookie) {
  CComPtr<ITfProperty> attribute_property;
  HRESULT hr = context->GetProperty(GUID_PROP_ATTRIBUTE, &attribute_property);
  if (FAILED(hr)) return hr;

  VARIANT var;
  var.vt = VT_I4;
  var.lVal = input_atom_;
  hr = attribute_property->SetValue(cookie, range, &var);
  return hr;
}

// Clear display attribute on the range of text.
HRESULT DisplayAttribute::ClearAttribute(ITfContext* context,
                                         ITfRange* range,
                                         TfEditCookie cookie) {
  CComPtr<ITfProperty> attribute_property;
  HRESULT hr = context->GetProperty(GUID_PROP_ATTRIBUTE, &attribute_property);
  if (FAILED(hr)) return hr;

  hr = attribute_property->Clear(cookie, range);
  return hr;
}
}  // namespace tsf
}  // namespace ime_goopy
