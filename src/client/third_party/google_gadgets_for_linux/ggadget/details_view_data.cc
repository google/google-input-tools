/*
  Copyright 2008 Google Inc.

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

#include "details_view_data.h"
#include "gadget_consts.h"
#include "logger.h"
#include "memory_options.h"
#include "scriptable_options.h"
#include "string_utils.h"
#include "small_object.h"

namespace ggadget {

class DetailsViewData::Impl : public SmallObject<> {
 public:
  Impl()
      : time_created_(0),
        scriptable_data_(&data_, true),
        external_object_(NULL),
        layout_(ContentItem::CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS),
        time_absolute_(false),
        is_html_(false),
        is_view_(false) {
  }

  ~Impl() {
    if (external_object_)
      external_object_->Unref();
  }

  Date time_created_;
  MemoryOptions data_;
  ScriptableOptions scriptable_data_;
  ScriptableInterface *external_object_;
  std::string source_;
  std::string text_;

  ContentItem::Layout layout_ : 2;
  bool time_absolute_         : 1;
  bool is_html_               : 1;
  bool is_view_               : 1;
};

DetailsViewData::DetailsViewData()
    : impl_(new Impl()) {
}

void DetailsViewData::DoClassRegister() {
  RegisterProperty("html_content",
                   NewSlot(&DetailsViewData::GetContentIsHTML),
                   NewSlot(&DetailsViewData::SetContentIsHTML));
  RegisterProperty("contentIsView",
                   NewSlot(&DetailsViewData::GetContentIsView),
                   NewSlot(&DetailsViewData::SetContentIsView));
  RegisterMethod("SetContent", NewSlot(&DetailsViewData::SetContent));
  RegisterMethod("SetContentFromItem",
                 NewSlot(&DetailsViewData::SetContentFromItem));
  RegisterProperty("detailsViewData", NewSlot(&DetailsViewData::GetData), NULL);
  RegisterProperty("external",
                   NewSlot(&DetailsViewData::GetExternalObject),
                   NewSlot(&DetailsViewData::SetExternalObject));
}

DetailsViewData::~DetailsViewData() {
  delete impl_;
  impl_ = NULL;
}

void DetailsViewData::SetContent(const char *source,
                                 Date time_created,
                                 const char *text,
                                 bool time_absolute,
                                 ContentItem::Layout layout) {
  impl_->source_ = source ? source : "";
  impl_->time_created_ = time_created;
  impl_->text_ = text ? text : "";
  impl_->time_absolute_ = time_absolute;
  impl_->layout_ = layout;

  size_t ext_len = strlen(kXMLExt);
  impl_->is_view_ =
      impl_->text_.length() > ext_len &&
      GadgetStrCmp(impl_->text_.c_str() + impl_->text_.length() - ext_len,
                   kXMLExt) == 0;
}

void DetailsViewData::SetContentFromItem(ContentItem *item) {
  if (item) {
    int flags = item->GetFlags();
    impl_->source_ = item->GetDisplaySource();
    impl_->time_created_ = item->GetTimeCreated();
    impl_->layout_ = item->GetLayout();
    impl_->time_absolute_ =
        (flags & ContentItem::CONTENT_ITEM_FLAG_TIME_ABSOLUTE);
    impl_->is_html_ = (item->GetFlags() & ContentItem::CONTENT_ITEM_FLAG_HTML);
    impl_->text_ = impl_->is_html_ ?
                   item->GetSnippet() : item->GetDisplaySnippet();
    impl_->is_view_ = false;
  }
}

std::string DetailsViewData::GetSource() const {
  return impl_->source_;
}

Date DetailsViewData::GetTimeCreated() const {
  return impl_->time_created_;
}

std::string DetailsViewData::GetText() const {
  return impl_->text_;
}

bool DetailsViewData::IsTimeAbsolute() const {
  return impl_->time_absolute_;
}

ContentItem::Layout DetailsViewData::GetLayout() const {
  return impl_->layout_;
}

bool DetailsViewData::GetContentIsHTML() const {
  return impl_->is_html_;
}

void DetailsViewData::SetContentIsHTML(bool is_html) {
  impl_->is_html_ = is_html;
}

bool DetailsViewData::GetContentIsView() const {
  return impl_->is_view_;
}

void DetailsViewData::SetContentIsView(bool is_view) {
  impl_->is_view_ = is_view;
}

ScriptableOptions *DetailsViewData::GetData() const {
  return &impl_->scriptable_data_;
}

ScriptableInterface *DetailsViewData::GetExternalObject() const {
  return impl_->external_object_;
}

void DetailsViewData::SetExternalObject(ScriptableInterface *external_object) {
  if (impl_->external_object_)
    impl_->external_object_->Unref();
  if (external_object)
    external_object->Ref();
  impl_->external_object_ = external_object;
}

DetailsViewData *DetailsViewData::CreateInstance() {
  return new DetailsViewData();
}

} // namespace ggadget
