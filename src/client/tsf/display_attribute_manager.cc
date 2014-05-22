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

#include "tsf/display_attribute_manager.h"
#include "base/logging.h"
#include "common/debug.h"
#include "common/framework_interface.h"
#include "common/scoped_lock.h"
#include "common/smart_com_ptr.h"

namespace ime_goopy {
namespace tsf {
class ATL_NO_VTABLE DisplayAttributeManager::Information
    : public CComObjectRootEx<CComMultiThreadModel>,
      public ITfDisplayAttributeInfo {
 public:
  DECLARE_NOT_AGGREGATABLE(Information)
  BEGIN_COM_MAP(Information)
    COM_INTERFACE_ENTRY(ITfDisplayAttributeInfo)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  Information() : index_(-1) {}
  HRESULT Initialize(DisplayAttributeManager *owner, int index) {
    if (!owner) {
      DCHECK(false);
      return E_INVALIDARG;
    }
    if (index < 0 || index > owner->entries_.size()) {
      DCHECK(false);
      return E_INVALIDARG;
    }
    owner_ = owner;
    index_ = index;
    attribute_ = owner_->entries_[index_].attr;
    return S_OK;
  }

  // ITfDisplayAttributeInfo
  STDMETHODIMP GetGUID(GUID* guid) {
    if (index_ == -1) {
      DCHECK(false);
      return E_UNEXPECTED;
    }
    if (!guid) return E_INVALIDARG;
    *guid = owner_->entries_[index_].guid;
    return S_OK;
  }

  STDMETHODIMP GetDescription(BSTR* description) {
    if (index_ == -1) {
      DCHECK(false);
      return E_UNEXPECTED;
    }
    if (!description) return E_INVALIDARG;
    *description = ::SysAllocString(L"Google IME Display Attribute");
    return *description ? S_OK : E_OUTOFMEMORY;
  }

  STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE* attribute) {
    if (index_ == -1) {
      DCHECK(false);
      return E_UNEXPECTED;
    }
    if (!attribute) {
      DCHECK(false);
      return E_INVALIDARG;
    }
    ScopedLock<Information> lock(this);
    *attribute = attribute_;
    return S_OK;
  }

  STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE* attribute) {
    if (index_ == -1) {
      DCHECK(false);
      return E_UNEXPECTED;
    }
    if (!attribute) {
      DCHECK(false);
      return E_INVALIDARG;
    }
    ScopedLock<Information> lock(this);
    attribute_ = *attribute;
    return S_OK;
  }

  STDMETHODIMP Reset() {
    if (index_ == -1) {
      DCHECK(false);
      return E_UNEXPECTED;
    }
    ScopedLock<Information> lock(this);
    attribute_ = owner_->entries_[index_].attr;
    return S_OK;
  }
 private:
  TF_DISPLAYATTRIBUTE attribute_;
  int index_;
  DisplayAttributeManager *owner_;
};

class ATL_NO_VTABLE DisplayAttributeManager::Enumerator
    : public CComObjectRootEx<CComMultiThreadModel>,
      public IEnumTfDisplayAttributeInfo {
 public:
  DECLARE_NOT_AGGREGATABLE(Enumerator)
  BEGIN_COM_MAP(Enumerator)
    COM_INTERFACE_ENTRY(IEnumTfDisplayAttributeInfo)
  END_COM_MAP()
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  Enumerator() : index_(0), owner_(NULL) {}
  void set_owner(DisplayAttributeManager *owner) { owner_ = owner; }

  // IEnumTfDisplayAttributeInfo
  STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo** enum_info) {
    if (!enum_info)
      return E_INVALIDARG;
    *enum_info = NULL;
    SmartComObjPtr<Enumerator> enumerator;
    HRESULT hr = enumerator.CreateInstance();
    if (FAILED(hr)) return hr;
    {
      ScopedLock<Enumerator> lock(this);
      enumerator->index_ = index_;
      enumerator->owner_ = owner_;
    }
    *enum_info = enumerator;
    (*enum_info)->AddRef();
    return S_OK;
  }

  STDMETHODIMP Next(ULONG count,
                    ITfDisplayAttributeInfo** info,
                    ULONG* fetched) {
    if (!count) return S_OK;
    if (!info) return E_INVALIDARG;
    if (!owner_) {
      DCHECK(false);
      return E_UNEXPECTED;
    }
    ScopedLock<Enumerator> lock(this);
    int fetching = 0;
    int total = owner_->entries_.size();
    while (index_ < total && fetching < count) {
      *info = owner_->entries_[index_].info;
      (*info)->AddRef();
      ++info;
      ++index_;
      ++fetching;
    }
    if (fetched)
      *fetched = fetching;

    if (fetching < count)
      return S_FALSE;
    else
      return S_OK;
  }

  STDMETHODIMP Reset() {
    ScopedLock<Enumerator> lock(this);
    index_ = 0;
    return S_OK;
  }

  STDMETHODIMP Skip(ULONG count) {
    if (!owner_) {
      DCHECK(false);
      return E_UNEXPECTED;
    }
    ScopedLock<Enumerator> lock(this);
    int total = owner_->entries_.size();
    index_ += count;
    if (index_ <= total) {
      return S_OK;
    } else {
      index_ = total;
      return S_FALSE;
    }
  }
 private:
  DisplayAttributeManager *owner_;
  LONG index_;
};

DisplayAttributeManager::DisplayAttributeManager() {
  // Initialize implicitly for singleton.
  HRESULT hr = Initialize();
  DCHECK(SUCCEEDED(hr));
}

DisplayAttributeManager::~DisplayAttributeManager() {
}

HRESULT DisplayAttributeManager::GetEnumerator(
    IEnumTfDisplayAttributeInfo** enumerator) {
  if (!enumerator) {
    DCHECK(false);
    return E_INVALIDARG;
  }
  *enumerator = enumerator_;
  (*enumerator)->AddRef();
  return S_OK;
}

HRESULT DisplayAttributeManager::GetAttribute(REFGUID guid,
                                              ITfDisplayAttributeInfo** info) {
  if (!info) return E_INVALIDARG;
  *info = NULL;

  for (size_t i = 0; i < entries_.size(); ++i) {
    if (IsEqualGUID(guid, entries_[i].guid)) {
      *info = entries_[i].info;
      (*info)->AddRef();
      return S_OK;
    }
  }

  return E_INVALIDARG;
}

// Apply input attribute to a range of text.
HRESULT DisplayAttributeManager::ApplyInputAttribute(
    ITfContext* context,
    ITfRange* range,
    TfEditCookie cookie,
    int index) {
  SmartComPtr<ITfProperty> attribute_property;
  HRESULT hr = context->GetProperty(
      GUID_PROP_ATTRIBUTE, attribute_property.GetAddress());
  if (FAILED(hr))
    return hr;

  VARIANT var;
  var.vt = VT_I4;
  var.lVal = entries_[index].atom;
  hr = attribute_property->SetValue(cookie, range, &var);
  return hr;
}

// Clear display attribute on the range of text.
HRESULT DisplayAttributeManager::ClearAttribute(ITfContext* context,
                                                ITfRange* range,
                                                TfEditCookie cookie) {
  SmartComPtr<ITfProperty> attribute_property;
  HRESULT hr = context->GetProperty(
      GUID_PROP_ATTRIBUTE, attribute_property.GetAddress());
  if (FAILED(hr)) return hr;

  hr = attribute_property->Clear(cookie, range);
  return hr;
}

HRESULT DisplayAttributeManager::Initialize() {
  int count = InputMethod::GetTextStyleCount();
  entries_.resize(count);
  for (int i = 0; i < count; ++i) {
    TextStyle style;
    InputMethod::GetTextStyle(i, &style);
    entries_[i].guid = style.guid;

    // Registration in the category manager.
    SmartComPtr<ITfCategoryMgr> category_manager;
    HRESULT hr = category_manager.CoCreate(CLSID_TF_CategoryMgr,
                                           NULL, CLSCTX_ALL);
    if (FAILED(hr))
      return hr;
    hr = category_manager->RegisterGUID(entries_[i].guid, &entries_[i].atom);
    if (FAILED(hr)) {
      DVLOG(1) << "Failed to register guid atom in " << __SHORT_FUNCTION__;
      return hr;
    }
    MakeDisplayAttribute(style, &entries_[i].attr);

    // Creates attribute object.
    SmartComObjPtr<Information> info;
    hr = info.CreateInstance();
    if (FAILED(hr)) {
      DVLOG(1) << "Failed to create info object in " << __SHORT_FUNCTION__;
      return hr;
    }
    info->Initialize(this, i);
    entries_[i].info = info.p();
  }

  // Creates enumerator.
  SmartComObjPtr<Enumerator> enumerator;
  HRESULT hr = enumerator.CreateInstance();
  if (FAILED(hr))
    return hr;
  enumerator->set_owner(this);
  enumerator_ = enumerator.p();

  return S_OK;
}

void DisplayAttributeManager::MakeDisplayAttribute(const TextStyle &style,
                                                   TF_DISPLAYATTRIBUTE *attr) {
  if (!attr) {
    DCHECK(false);
    return;
  }

  // Text color.
  if (style.color_field_mask & TextStyle::FIELD_TEXT_COLOR) {
    attr->crText.type = TF_CT_COLORREF;
    attr->crText.cr = style.text_color;
  } else {
    attr->crText.type = TF_CT_NONE;
    attr->crText.cr = 0;
  }

  // Background color.
  if (style.color_field_mask & TextStyle::FIELD_BACKGROUND_COLOR) {
    attr->crBk.type = TF_CT_COLORREF;
    attr->crBk.cr = style.background_color;
  } else {
    attr->crBk.type = TF_CT_NONE;
    attr->crBk.cr = 0;
  }

  // Line color.
  if (style.color_field_mask & TextStyle::FIELD_LINE_COLOR) {
    attr->crLine.type = TF_CT_COLORREF;
    attr->crLine.cr = style.line_color;
  } else {
    attr->crLine.type = TF_CT_NONE;
    attr->crLine.cr = 0;
  }

  // Line style.
  static TF_DA_LINESTYLE kLineStyleTable[] = {
    TF_LS_NONE,
    TF_LS_SOLID,
    TF_LS_DOT,
    TF_LS_DASH,
    TF_LS_SQUIGGLE
  };
  attr->lsStyle = kLineStyleTable[style.line_style];
  attr->fBoldLine = style.bold_line ? TRUE : FALSE;
  attr->bAttr = TF_ATTR_INPUT;
}

}  // namespace tsf
}  // namespace ime_goopy
