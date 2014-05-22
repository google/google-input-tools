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

#include <cmath>
#include <ctime>
#include "content_item.h"
#include "contentarea_element.h"
#include "details_view_data.h"
#include "image_interface.h"
#include "messages.h"
#include "scriptable_image.h"
#include "string_utils.h"
#include "text_frame.h"
#include "texture.h"
#include "view.h"
#include "small_object.h"

namespace ggadget {

static const int kMinWidthToUseLongVersionOfTimeString = 125;
// -1 means to use the gadget default font size.
static const int kNormalFontSize = -1;
static const int kExtraInfoFontSize = -1;
static const int kSnippetFontSize = -1;

// In this order: y top, x right, y bottom, x left
static const int kItemBorderOffsets[] = {2, 3, 3, 3};
static const int kItemBorderWidthOffset =
  (kItemBorderOffsets[1] + kItemBorderOffsets[3]);
static const int kItemBorderHeightOffset =
  (kItemBorderOffsets[0] + kItemBorderOffsets[2]);

const Color ScriptableCanvas::kColorNormalBackground(0.984, 0.984, 0.984);
const Color ScriptableCanvas::kColorNormalText(0, 0, 0);
const Color ScriptableCanvas::kColorExtraInfo(0.133, 0.267, 0.6); // #224499
const Color ScriptableCanvas::kColorSnippet(0.4, 0.4, 0.4); // #666666

class ContentItem::Impl : public SmallObject<> {
 public:
  Impl(View *view)
      : view_(view),
        content_area_(NULL),
        image_(NULL), notifier_image_(NULL),
        time_created_(0),
        x_(0), y_(0), width_(0), height_(0),
        layout_x_(0), layout_y_(0), layout_width_(0), layout_height_(0),
        heading_text_(NULL, view),
        source_text_(NULL, view),
        time_text_(NULL, view),
        snippet_text_(NULL, view),
        flags_(CONTENT_ITEM_FLAG_NONE),
        layout_(CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS),
        display_text_changed_(false),
        x_relative_(false), y_relative_(false),
        width_relative_(false), height_relative_(false) {
    ASSERT(view);
    heading_text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    heading_text_.SetColor(ScriptableCanvas::kColorNormalText, 1.0);
    heading_text_.SetSize(kNormalFontSize);
    source_text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    source_text_.SetColor(ScriptableCanvas::kColorExtraInfo, 1.0);
    source_text_.SetSize(kExtraInfoFontSize);
    time_text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    time_text_.SetColor(ScriptableCanvas::kColorExtraInfo, 1.0);
    time_text_.SetAlign(CanvasInterface::ALIGN_RIGHT);
    time_text_.SetSize(kExtraInfoFontSize);
    snippet_text_.SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
    snippet_text_.SetColor(ScriptableCanvas::kColorSnippet, 1.0);
    snippet_text_.SetWordWrap(true);
    snippet_text_.SetSize(kSnippetFontSize);
  }

  ~Impl() {
  }

  void UpdateTimeText(double width) {
    time_text_.SetText(GetTimeDisplayString(
        time_created_,
        (((flags_ & CONTENT_ITEM_FLAG_TIME_ABSOLUTE) || !view_) ?
         0 : view_->GetCurrentTime()),
        width < kMinWidthToUseLongVersionOfTimeString));
  }

#define PARSE_PIXEL_OR_RELATIVE(input, result, relative, default_rel) { \
  double _d;                                                            \
  switch (BasicElement::ParsePixelOrRelative((input), &_d)) {           \
    case BasicElement::PR_PIXEL:                                        \
      (result) = (_d);                                                  \
      (relative) = false;                                               \
      break;                                                            \
    case BasicElement::PR_RELATIVE:                                     \
      (result) = (_d * 100.);                                           \
      (relative) = true;                                                \
      break;                                                            \
    case BasicElement::PR_UNSPECIFIED:                                  \
      (result) = (default_rel);                                         \
      (relative) = true;                                                \
      break;                                                            \
    default:                                                            \
      break;                                                            \
  }                                                                     \
}

  void SetRect(const Variant &x, const Variant &y,
               const Variant &width, const Variant &height) {
    PARSE_PIXEL_OR_RELATIVE(x, x_, x_relative_, 0);
    PARSE_PIXEL_OR_RELATIVE(y, y_, y_relative_, 0);
    PARSE_PIXEL_OR_RELATIVE(width, width_, width_relative_, 100);
    PARSE_PIXEL_OR_RELATIVE(height, height_, height_relative_, 100);
    QueueDraw();
  }

  void QueueDraw() {
    if (content_area_)
      content_area_->QueueDraw();
  }

  void DisplayTextChanged() {
    display_text_changed_ = true;
    QueueDraw();
  }

  static std::string StripHTML(const std::string &s) {
    return ContainsHTML(s.c_str()) ? ExtractTextFromHTML(s.c_str()) : s;
  }

  void UpdateDisplayText() {
    if (display_text_changed_) {
      if (flags_ & CONTENT_ITEM_FLAG_DISPLAY_AS_IS) {
        heading_text_.SetText(heading_);
        source_text_.SetText(source_);
        snippet_text_.SetText(snippet_);
      } else {
        heading_text_.SetText(StripHTML(heading_));
        source_text_.SetText(StripHTML(source_));
        snippet_text_.SetText(StripHTML(snippet_));
      }
      display_text_changed_ = false;
    }
  }

  void SetContentArea(ContentAreaElement *content_area) {
    if (content_area_ == content_area)
      return;

    content_area_ = content_area;
    view_ = content_area ? content_area->GetView() : NULL;

    heading_text_.SetView(view_);
    source_text_.SetView(view_);
    time_text_.SetView(view_);
    snippet_text_.SetView(view_);
  }

  DEFINE_DELEGATE_GETTER(GetHeadingText, &src->impl_->heading_text_,
                         ContentItem, TextFrame);
  DEFINE_DELEGATE_GETTER(GetSourceText, &src->impl_->source_text_,
                         ContentItem, TextFrame);
  DEFINE_DELEGATE_GETTER(GetTimeText, &src->impl_->time_text_,
                         ContentItem, TextFrame);
  DEFINE_DELEGATE_GETTER(GetSnippetText, &src->impl_->snippet_text_,
                         ContentItem, TextFrame);

  View *view_;
  ContentAreaElement *content_area_;
  ScriptableHolder<ScriptableImage> image_;
  ScriptableHolder<ScriptableImage> notifier_image_;

  uint64_t time_created_;
  double x_, y_, width_, height_;
  double layout_x_, layout_y_, layout_width_, layout_height_;

  std::string open_command_, tooltip_, heading_, source_, snippet_;
  TextFrame heading_text_, source_text_, time_text_, snippet_text_;

  Signal7<void, ContentItem *, Gadget::DisplayTarget, ScriptableCanvas *,
          double, double, double, double> on_draw_item_signal_;
  Signal4<double, ContentItem *, Gadget::DisplayTarget,
          ScriptableCanvas *, double> on_get_height_signal_;
  Signal1<Variant, ContentItem *> on_open_item_signal_;
  Signal1<Variant, ContentItem *> on_toggle_item_pinned_state_signal_;
  Signal7<Variant, ContentItem *, Gadget::DisplayTarget, ScriptableCanvas *,
          double, double, double, double> on_get_is_tooltip_required_signal_;
  Signal1<ScriptableInterface *, ContentItem *> on_details_view_signal_;
  Signal2<Variant, ContentItem *, int> on_process_details_view_feedback_signal_;
  Signal1<Variant, ContentItem *> on_remove_item_signal_;

  ContentItem::Flags flags_  : 16;
  Layout layout_             : 2;
  bool display_text_changed_ : 1;
  bool x_relative_           : 1;
  bool y_relative_           : 1;
  bool width_relative_       : 1;
  bool height_relative_      : 1;
};

ContentItem::ContentItem(View *view)
    : impl_(new Impl(view)) {
}

void ContentItem::DoClassRegister() {
  RegisterProperty("image",
                   NewSlot(&ContentItem::GetImage),
                   NewSlot(&ContentItem::SetImage));
  RegisterProperty("notifier_image",
                   NewSlot(&ContentItem::GetNotifierImage),
                   NewSlot(&ContentItem::SetNotifierImage));
  RegisterProperty("time_created",
                   NewSlot(&ContentItem::GetTimeCreated),
                   NewSlot(&ContentItem::SetTimeCreated));
  RegisterProperty("heading",
                   NewSlot(&ContentItem::GetHeading),
                   NewSlot(&ContentItem::SetHeading));
  RegisterProperty("source",
                   NewSlot(&ContentItem::GetSource),
                   NewSlot(&ContentItem::SetSource));
  RegisterProperty("snippet",
                   NewSlot(&ContentItem::GetSnippet),
                   NewSlot(&ContentItem::SetSnippet));
  // Don't use the xxxColor properties until they are in the public API.
  RegisterProperty("headingColor",
                   NewSlot(&TextFrame::GetColor, Impl::GetHeadingTextConst),
                   NewSlot<void, const Variant &>(&TextFrame::SetColor,
                                                  Impl::GetHeadingText));
  RegisterProperty("sourceColor",
                   NewSlot(&TextFrame::GetColor, Impl::GetSourceTextConst),
                   NewSlot<void, const Variant &>(&TextFrame::SetColor,
                                                  Impl::GetSourceText));
  RegisterProperty("timeColor",
                   NewSlot(&TextFrame::GetColor, Impl::GetTimeTextConst),
                   NewSlot<void, const Variant &>(&TextFrame::SetColor,
                                                  &Impl::GetTimeText));
  RegisterProperty("snippetColor",
                   NewSlot(&TextFrame::GetColor, Impl::GetSnippetTextConst),
                   NewSlot<void, const Variant &>(&TextFrame::SetColor,
                                                  &Impl::GetSnippetText));
  RegisterProperty("open_command",
                   NewSlot(&ContentItem::GetOpenCommand),
                   NewSlot(&ContentItem::SetOpenCommand));
  RegisterProperty("layout",
                   NewSlot(&ContentItem::GetLayout),
                   NewSlot(&ContentItem::SetLayout));
  RegisterProperty("flags", NULL, // Write only.
                   NewSlot(&ContentItem::SetFlags));
  RegisterProperty("tooltip", NULL, // Write only.
                   NewSlot(&ContentItem::SetTooltip));
  RegisterMethod("SetRect", NewSlot(&Impl::SetRect, &ContentItem::impl_));

  RegisterClassSignal("onDrawItem", &Impl::on_draw_item_signal_,
                      &ContentItem::impl_);
  RegisterClassSignal("onGetHeight", &Impl::on_get_height_signal_,
                      &ContentItem::impl_);
  RegisterClassSignal("onOpenItem", &Impl::on_open_item_signal_,
                      &ContentItem::impl_);
  RegisterClassSignal("onToggleItemPinnedState",
                      &Impl::on_toggle_item_pinned_state_signal_,
                      &ContentItem::impl_);
  RegisterClassSignal("onGetIsTooltipRequired",
                      &Impl::on_get_is_tooltip_required_signal_,
                      &ContentItem::impl_);
  RegisterClassSignal("onDetailsView", &Impl::on_details_view_signal_,
                      &ContentItem::impl_);
  RegisterClassSignal("onProcessDetailsViewFeedback",
                      &Impl::on_process_details_view_feedback_signal_,
                      &ContentItem::impl_);
  RegisterClassSignal("onRemoveItem", &Impl::on_remove_item_signal_,
                      &ContentItem::impl_);
}

ContentItem::~ContentItem() {
  delete impl_;
  impl_ = NULL;
}

void ContentItem::AttachContentArea(ContentAreaElement *content_area) {
  ASSERT(impl_->content_area_ == NULL);
  impl_->SetContentArea(content_area);
  Ref();
}

void ContentItem::DetachContentArea(ContentAreaElement *content_area) {
  GGL_UNUSED(content_area);
  ASSERT(impl_->content_area_ == content_area);
  impl_->SetContentArea(NULL);
  Unref();
}

ScriptableImage *ContentItem::GetImage() const {
  return impl_->image_.Get();
}

void ContentItem::SetImage(ScriptableImage *image) {
  impl_->image_.Reset(image);
  impl_->QueueDraw();
}

ScriptableImage *ContentItem::GetNotifierImage() const {
  return impl_->notifier_image_.Get();
}

void ContentItem::SetNotifierImage(ScriptableImage *image) {
  impl_->notifier_image_.Reset(image);
  impl_->QueueDraw();
}

Date ContentItem::GetTimeCreated() const {
  return Date(impl_->time_created_);
}

void ContentItem::SetTimeCreated(const Date &time) {
  if (impl_->time_created_ != time.value) {
    impl_->time_created_ = time.value;
    impl_->QueueDraw();
  }
}

std::string ContentItem::GetHeading() const {
  return impl_->heading_;
}

void ContentItem::SetHeading(const char *heading) {
  if (AssignIfDiffer(heading, &impl_->heading_))
    impl_->DisplayTextChanged();
}

std::string ContentItem::GetSource() const {
  return impl_->source_;
}

void ContentItem::SetSource(const char *source) {
  if (AssignIfDiffer(source, &impl_->source_))
    impl_->DisplayTextChanged();
}

std::string ContentItem::GetSnippet() const {
  return impl_->snippet_;
}

void ContentItem::SetSnippet(const char *snippet) {
  if (AssignIfDiffer(snippet, &impl_->snippet_))
    impl_->DisplayTextChanged();
}

std::string ContentItem::GetDisplayHeading() const {
  impl_->UpdateDisplayText();
  return impl_->heading_text_.GetText();
}

std::string ContentItem::GetDisplaySource() const {
  impl_->UpdateDisplayText();
  return impl_->source_text_.GetText();
}

std::string ContentItem::GetDisplaySnippet() const {
  impl_->UpdateDisplayText();
  return impl_->snippet_text_.GetText();
}

std::string ContentItem::GetOpenCommand() const {
  return impl_->open_command_;
}

void ContentItem::SetOpenCommand(const char *open_command) {
  impl_->open_command_ = open_command;
}

ContentItem::Layout ContentItem::GetLayout() const {
  return impl_->layout_;
}

void ContentItem::SetLayout(Layout layout) {
  if (layout != impl_->layout_) {
    impl_->layout_ = layout;
    impl_->heading_text_.SetWordWrap(layout == CONTENT_ITEM_LAYOUT_NEWS);
    impl_->QueueDraw();
  }
}

int ContentItem::GetFlags() const {
  return impl_->flags_;
}

void ContentItem::SetFlags(int flags) {
  if (flags != impl_->flags_) {
    // We don't think this is necessary to auto set AS_IS flag for HTML.
    // Sometimes the user may want HTML but not AS_IS.
    // if (flags & CONTENT_ITEM_FLAG_HTML)
    //   flags |= CONTENT_ITEM_FLAG_DISPLAY_AS_IS;
    // Casting to Flags to avoid conversion warning when compiling by the
    // latest gcc.
    impl_->flags_ = static_cast<Flags>(flags);
    impl_->heading_text_.SetBold(flags & CONTENT_ITEM_FLAG_HIGHLIGHTED);
    impl_->DisplayTextChanged();
  }
}

std::string ContentItem::GetTooltip() const {
  return impl_->tooltip_;
}

void ContentItem::SetTooltip(const std::string &tooltip) {
  impl_->tooltip_ = tooltip;
}

void ContentItem::SetRect(double x, double y, double width, double height,
                          bool x_relative, bool y_relative,
                          bool width_relative, bool height_relative) {
  impl_->x_ = x;
  impl_->y_ = y;
  impl_->width_ = width;
  impl_->height_ = height;
  impl_->x_relative_ = x_relative;
  impl_->y_relative_ = y_relative;
  impl_->width_relative_ = width_relative;
  impl_->height_relative_ = height_relative;
}

void ContentItem::GetRect(double *x, double *y, double *width, double *height,
                          bool *x_relative, bool *y_relative,
                          bool *width_relative, bool *height_relative) {
  ASSERT(x && y && width && height && x_relative && y_relative &&
         width_relative && height_relative);
  *x = impl_->x_;
  *y = impl_->y_;
  *width = impl_->width_;
  *height = impl_->height_;
  *x_relative = impl_->x_relative_;
  *y_relative = impl_->y_relative_;
  *width_relative = impl_->width_relative_;
  *height_relative = impl_->height_relative_;
}

void ContentItem::SetLayoutRect(double x, double y, double width, double height) {
  impl_->layout_x_ = x;
  impl_->layout_y_ = y;
  impl_->layout_width_ = width;
  impl_->layout_height_ = height;
}

void ContentItem::GetLayoutRect(double *x, double *y,
                                double *width, double *height) {
  ASSERT(x && y && width && height);
  *x = impl_->layout_x_;
  *y = impl_->layout_y_;
  *width = impl_->layout_width_;
  *height = impl_->layout_height_;
}

bool ContentItem::CanOpen() const {
  return !(impl_->flags_ & CONTENT_ITEM_FLAG_HIDDEN) &&
         !(impl_->flags_ & CONTENT_ITEM_FLAG_STATIC) &&
         (!impl_->open_command_.empty() ||
          impl_->on_open_item_signal_.HasActiveConnections());
}

void ContentItem::Draw(Gadget::DisplayTarget target,
                       CanvasInterface *canvas,
                       double x, double y, double width, double height) {
  if (!impl_->view_)
    return;

  // Rect adjustments apply even if OnDraw handler is set
  x += kItemBorderOffsets[3];
  y += kItemBorderOffsets[0];
  if (width > kItemBorderWidthOffset)
    width -= kItemBorderWidthOffset;
  if (height > kItemBorderHeightOffset)
    height -= kItemBorderHeightOffset;

  // Try script handler first.
  if (impl_->on_draw_item_signal_.HasActiveConnections()) {
    ScriptableCanvas scriptable_canvas(canvas, impl_->view_);
    impl_->on_draw_item_signal_(this, target, &scriptable_canvas,
                                x, y, width, height);
    return;
  }

  // Then the default logic.
  double heading_space_width = width;
  double heading_left = x;
  double image_height = 0;
  double heading_width = 0, heading_height = 0;
  impl_->heading_text_.GetSimpleExtents(&heading_width, &heading_height);
  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_NEWS &&
      heading_width > heading_space_width) {
    // Heading can wrap up to 2 lines under news layout mode.
    heading_height *= 2;
  }

  if (impl_->image_.Get()) {
    const ImageInterface *image = impl_->image_.Get()->GetImage();
    if (image) {
      double image_width = image->GetWidth();
      heading_space_width -= image_width;
      image_height = image->GetHeight();
      double image_y = y;
      if (heading_height > image_height)
        image_y += (heading_height - image_height) / 2;
      if (impl_->flags_ & CONTENT_ITEM_FLAG_LEFT_ICON) {
        image->Draw(canvas, x, image_y);
        heading_left += image_width;
      } else {
        image->Draw(canvas, x + width - image_width, image_y);
      }
    }
  }

  impl_->heading_text_.Draw(canvas, heading_left, y,
                            heading_space_width, heading_height);
  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS ||
      impl_->layout_ > CONTENT_ITEM_LAYOUT_EMAIL) {
    return;
  }

  impl_->UpdateTimeText(width);
  y += std::max(heading_height, image_height);
  double source_width = 0, source_height = 0;
  impl_->source_text_.GetSimpleExtents(&source_width, &source_height);
  double time_width = 0, time_height = 0;
  impl_->time_text_.GetSimpleExtents(&time_width, &time_height);
  time_width += 3;
  if (time_width > width) time_width = width;

  impl_->time_text_.Draw(canvas, x + width - time_width, y,
                         time_width, time_height);
  if (width > time_width)
    impl_->source_text_.Draw(canvas, x, y, width - time_width, source_height);

  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_EMAIL) {
    y += std::max(source_height, time_height);
    double snippet_width = 0, snippet_height = 0;
    impl_->snippet_text_.GetSimpleExtents(&snippet_width, &snippet_height);
    if (snippet_width > width)
      snippet_height *= 2;
    impl_->snippet_text_.Draw(canvas, x, y, width, snippet_height);
  }
}

Connection *ContentItem::ConnectOnDrawItem(
    Slot7<void, ContentItem *, Gadget::DisplayTarget,
          ScriptableCanvas *, double, double, double, double> *handler) {
  return impl_->on_draw_item_signal_.Connect(handler);
}

double ContentItem::GetHeight(Gadget::DisplayTarget target,
                           CanvasInterface *canvas, double width) {
  if (!impl_->view_)
    return 0;

  impl_->UpdateDisplayText();

  // Try script handler first.
  if (impl_->on_get_height_signal_.HasActiveConnections()) {
    ScriptableCanvas scriptable_canvas(canvas, impl_->view_);
    return impl_->on_get_height_signal_(this, target, &scriptable_canvas,
                                        width);
  }

  if (width > kItemBorderWidthOffset)
    width -= kItemBorderWidthOffset;
  double heading_space_width = width;
  double image_height = 0;
  if (impl_->image_.Get()) {
    const ImageInterface *image = impl_->image_.Get()->GetImage();
    if (image) {
      heading_space_width -= image->GetWidth();
      image_height = image->GetHeight();
    }
  }

  // Then the default logic.
  double heading_width = 0, heading_height = 0;
  impl_->heading_text_.GetSimpleExtents(&heading_width, &heading_height);
  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS ||
      impl_->layout_ > CONTENT_ITEM_LAYOUT_EMAIL) {
    // Only heading and icon.
    return std::max(heading_height, image_height) + kItemBorderHeightOffset;
  }

  impl_->UpdateTimeText(width);
  double source_width = 0, source_height = 0;
  impl_->source_text_.GetSimpleExtents(&source_width, &source_height);
  double time_width = 0, time_height = 0;
  impl_->time_text_.GetSimpleExtents(&time_width, &time_height);
  double extra_info_height = std::max(source_height, time_height);
  if (impl_->layout_ == CONTENT_ITEM_LAYOUT_NEWS) {
    // Heading can wrap up to 2 lines. Show extra info.
    if (heading_width > heading_space_width)
      heading_height *= 2;
    return std::max(heading_height, image_height) +
           extra_info_height + kItemBorderHeightOffset;
  }

  // Heading doesn't wrap. Show extra info. Snippet can wrap up to 2 lines.
  double snippet_width = 0, snippet_height = 0;
  impl_->snippet_text_.GetSimpleExtents(&snippet_width, &snippet_height);
  if (snippet_width > width)
    snippet_height *= 2;
  return std::max(heading_height, image_height) +
         extra_info_height + snippet_height + kItemBorderHeightOffset;
}

Connection *ContentItem::ConnectOnGetHeight(
    Slot4<double, ContentItem *, Gadget::DisplayTarget,
          ScriptableCanvas *, double> *handler) {
  return impl_->on_get_height_signal_.Connect(handler);
}


// ContentItem callbacks use special return value rule:
// If false, the result is false;
// If any other values, the result is true (which means 'cancel the action').
static bool VariantToBool(const Variant &v) {
  return v.type() != Variant::TYPE_BOOL || VariantValue<bool>()(v);
}

void ContentItem::OpenItem() {
  bool result = false;
  if (impl_->on_open_item_signal_.HasActiveConnections())
    result = VariantToBool(impl_->on_open_item_signal_(this));

  if (!result && impl_->view_)
    impl_->view_->OpenURL(impl_->open_command_.c_str());
}

Connection *ContentItem::ConnectOnOpenItem(
    Slot1<bool, ContentItem *> *handler) {
  return impl_->on_open_item_signal_.ConnectGeneral(handler);
}

void ContentItem::ToggleItemPinnedState() {
  bool result = false;
  if (impl_->on_toggle_item_pinned_state_signal_.HasActiveConnections())
    result = VariantToBool(impl_->on_toggle_item_pinned_state_signal_(this));

  if (!result) {
    impl_->flags_ =
        static_cast<Flags>(impl_->flags_ ^ CONTENT_ITEM_FLAG_PINNED);
    impl_->QueueDraw();
  }
}

Connection *ContentItem::ConnectOnToggleItemPinnedState(
    Slot1<bool, ContentItem *> *handler) {
  return impl_->on_toggle_item_pinned_state_signal_.ConnectGeneral(handler);
}

bool ContentItem::IsTooltipRequired(Gadget::DisplayTarget target,
                                    CanvasInterface *canvas,
                                    double x, double y,
                                    double width, double height) {
  if (!impl_->view_)
    return false;

  if (impl_->on_get_is_tooltip_required_signal_.HasActiveConnections()) {
    ScriptableCanvas scriptable_canvas(canvas, impl_->view_);
    return VariantToBool(impl_->on_get_is_tooltip_required_signal_(
        this, target, &scriptable_canvas, x, y, width, height));
  }
  return !impl_->tooltip_.empty();
}

Connection *ContentItem::ConnectOnGetIsTooltipRequired(
    Slot7<bool, ContentItem *, Gadget::DisplayTarget,
          ScriptableCanvas *, double, double, double, double> *handler) {
  return impl_->on_get_is_tooltip_required_signal_.ConnectGeneral(handler);
}

bool ContentItem::OnDetailsView(std::string *title,
                                DetailsViewData **details_view_data,
                                int *flags) {
  ASSERT(title && details_view_data && flags);
  bool cancel = false;
  title->clear();
  *details_view_data = NULL;
  *flags = ViewInterface::DETAILS_VIEW_FLAG_NONE;

  if (impl_->on_details_view_signal_.HasActiveConnections()) {
    Variant param(this);
    ResultVariant details_info_v =
        impl_->on_details_view_signal_.Emit(1, &param);
    ScriptableInterface *details_info =
        VariantValue<ScriptableInterface *>()(details_info_v.v());
    if (details_info) {
      details_info->GetProperty("title").v().ConvertToString(title);
      details_info->GetProperty("cancel").v().ConvertToBool(&cancel);
      // The conversion rule of the flags property is the same as our default.
      details_info->GetProperty("flags").v().ConvertToInt(flags);
      ResultVariant v = details_info->GetProperty("details_control");
      if (v.v().type() == Variant::TYPE_SCRIPTABLE) {
        *details_view_data = VariantValue<DetailsViewData *>()(v.v());
        if (*details_view_data) {
          // Add a ref to details_view_data to prevent unexpected deletion.
          (*details_view_data)->Ref();
        }
      }
    } else {
      cancel = true;
    }
  }

  // Default logics.
  if (!*details_view_data) {
    *details_view_data = new DetailsViewData();
    (*details_view_data)->SetContentFromItem(this);
  } else {
    // Remove the transient ref becuase other part of program don't expect it.
    (*details_view_data)->Unref(true);
  }
  if (title->empty()) {
    *title = GetDisplayHeading();
  }
  if (*flags == ViewInterface::DETAILS_VIEW_FLAG_NONE) {
    if (impl_->flags_ & CONTENT_ITEM_FLAG_NEGATIVE_FEEDBACK)
      *flags |= ViewInterface::DETAILS_VIEW_FLAG_NEGATIVE_FEEDBACK;
    if (!(impl_->flags_ & CONTENT_ITEM_FLAG_NO_REMOVE))
      *flags |= ViewInterface::DETAILS_VIEW_FLAG_REMOVE_BUTTON;
    if (impl_->flags_ & CONTENT_ITEM_FLAG_SHAREABLE)
      *flags |= ViewInterface::DETAILS_VIEW_FLAG_SHARE_WITH_BUTTON;
    *flags |= ViewInterface::DETAILS_VIEW_FLAG_TOOLBAR_OPEN;
  }
  return cancel;
}

Connection *ContentItem::ConnectOnDetailsView(OnDetailsViewHandler *handler) {
  return impl_->on_details_view_signal_.Connect(handler);
}

bool ContentItem::ProcessDetailsViewFeedback(int flags) {
  if (impl_->on_process_details_view_feedback_signal_.HasActiveConnections())
    return VariantToBool(impl_->on_process_details_view_feedback_signal_(
        this, flags));
  return false;
}

Connection *ContentItem::ConnectOnProcessDetailsViewFeedback(
    Slot2<bool, ContentItem *, int> *handler) {
  return impl_->on_process_details_view_feedback_signal_.ConnectGeneral(
      handler);
}

bool ContentItem::OnUserRemove() {
  return impl_->on_remove_item_signal_.HasActiveConnections() ?
         VariantToBool(impl_->on_remove_item_signal_(this)) : false;
}

Connection *ContentItem::ConnectOnRemoveItem(
    Slot1<bool, ContentItem *> *handler) {
  return impl_->on_remove_item_signal_.ConnectGeneral(handler);
}

std::string ContentItem::GetTimeDisplayString(uint64_t time,
                                              uint64_t current_time,
                                              bool short_form) {
  static const unsigned int kMsPerDay = 86400000U;
  static const unsigned int kMsPerHour = 3600000U;
  static const unsigned int kMsPerMinute = 60000U;

  if (time == 0)
    return "";

  if (current_time == 0) {
    // Show absolute time as HH:MMam/pm.
    char buffer[20];
    time_t t = static_cast<time_t>(time / 1000);
    strftime(buffer, sizeof(buffer), GM_("TIME_FORMAT_SHORT"), localtime(&t));
    std::string utf8;
    ConvertLocaleStringToUTF8(buffer, &utf8);
    return utf8;
  }

  uint64_t time_diff = 0;
  if (time < current_time)
    time_diff = current_time - time;

  if (time_diff >= 4 * kMsPerDay) {
    // > 4 days, show like 'Mar 20'.
    char buffer[20];
    time_t t = static_cast<time_t>(time / 1000);
    strftime(buffer, sizeof(buffer), GM_("DATE_FORMAT_SHORT"), localtime(&t));
    std::string utf8;
    ConvertLocaleStringToUTF8(buffer, &utf8);
    return utf8;
  }

  if (time_diff >= kMsPerDay) {
    int days = static_cast<int>(time_diff / kMsPerDay);
    return StringPrintf(GM_(short_form ?
                           (days > 1 ? "DAYS_AGO_SHORT" : "DAY_AGO_SHORT") :
                           (days > 1 ? "DAYS_AGO_LONG" : "DAY_AGO_LONG")),
                        days);
  }
  if (time_diff >= kMsPerHour) {
    int hours = static_cast<int>(time_diff / kMsPerHour);
    return StringPrintf(GM_(short_form ?
                           (hours > 1 ? "HOURS_AGO_SHORT" : "HOUR_AGO_SHORT") :
                           (hours > 1 ? "HOURS_AGO_LONG" : "HOUR_AGO_LONG")),
                        hours);
  }
  int mins = static_cast<int>(time_diff / kMsPerMinute);
  return StringPrintf(GM_(short_form ?
                         (mins > 1 ? "MINUTES_AGO_SHORT" : "MINUTE_AGO_SHORT") :
                         (mins > 1 ? "MINUTES_AGO_LONG" : "MINUTE_AGO_LONG")),
                      mins);
}

static const Variant kDrawLineDefaultArgs[] = {
  Variant(), Variant(), Variant(), Variant(), Variant("#000000")
};
static const Variant kDrawRectDefaultArgs[] = {
  Variant(), Variant(), Variant(), Variant(), Variant(), Variant()
};
static const Variant kDrawImageDefaultArgs[] = {
  Variant(), Variant(), Variant(), Variant(), Variant(), Variant(100)
};
static const Variant kDrawTextDefaultArgs[] = {
  Variant(), Variant(), Variant(), Variant(), Variant(),
  Variant("#000000"), Variant(0), Variant(ScriptableCanvas::FONT_NORMAL)
};
static const Variant kGetTextWidthDefaultArgs[] = {
  Variant(), Variant(0), Variant(ScriptableCanvas::FONT_NORMAL)
};
static const Variant kGetTextHeightDefaultArgs[] = {
  Variant(), Variant(), Variant(0), Variant(ScriptableCanvas::FONT_NORMAL)
};

ScriptableCanvas::ScriptableCanvas(CanvasInterface *canvas, View *view)
    : canvas_(canvas), view_(view) {
}

void ScriptableCanvas::DoClassRegister() {
  RegisterMethod("DrawLine",
                 NewSlotWithDefaultArgs(
                     NewSlot(&ScriptableCanvas::DrawLineWithColorName),
                     kDrawLineDefaultArgs));
  RegisterMethod("DrawRect",
                 NewSlotWithDefaultArgs(
                     NewSlot(&ScriptableCanvas::DrawRectWithColorName),
                     kDrawRectDefaultArgs));
  RegisterMethod("DrawImage",
                 NewSlotWithDefaultArgs(
                     NewSlot(&ScriptableCanvas::DrawImage),
                     kDrawImageDefaultArgs));
  RegisterMethod("DrawText",
                 NewSlotWithDefaultArgs(
                     NewSlot(&ScriptableCanvas::DrawTextWithColorName),
                     kDrawTextDefaultArgs));
  RegisterMethod("GetTextWidth",
                 NewSlotWithDefaultArgs(
                     NewSlot(&ScriptableCanvas::GetTextWidth),
                     kGetTextWidthDefaultArgs));
  RegisterMethod("GetTextHeight",
                 NewSlotWithDefaultArgs(
                     NewSlot(&ScriptableCanvas::GetTextHeight),
                     kGetTextHeightDefaultArgs));
}

ScriptableCanvas::~ScriptableCanvas() {
}

void ScriptableCanvas::DrawLine(double x1, double y1, double x2, double y2,
                                const Color &color) {
  canvas_->DrawLine(x1, y1, x2, y2, 1, color);
}

void ScriptableCanvas::DrawRect(double x1, double y1, double width, double height,
                                const Color *line_color,
                                const Color *fill_color) {
  if (fill_color) {
    canvas_->DrawFilledRect(x1, y1, width, height, *fill_color);
  }
  if (line_color) {
    double x2 = x1 + width;
    double y2 = y1 + height;

    // Make the border actually inside the rect.
    x1++; x2--; y1++; y2--;

    canvas_->DrawLine(x1, y1, x1, y2, 1, *line_color);
    canvas_->DrawLine(x1, y1, x2, y1, 1, *line_color);
    canvas_->DrawLine(x2, y1, x2, y2, 1, *line_color);
    canvas_->DrawLine(x1, y2, x2, y2, 1, *line_color);
  }
}

void ScriptableCanvas::DrawImage(double x, double y, double width, double height,
                                 ScriptableImage *image, int alpha_percent) {
  if (image) {
    const ImageInterface *real_image = image->GetImage();
    if (real_image) {
      canvas_->PushState();
      canvas_->MultiplyOpacity(alpha_percent / 100.0);
      real_image->StretchDraw(canvas_, x, y, width, height);
      canvas_->PopState();
    }
  }
}

static void SetupTextFrame(TextFrame *text_frame,
                           const char *text, const Color &color,
                           int flags, ScriptableCanvas::FontID font) {
  text_frame->SetText(text);
  text_frame->SetTrimming(CanvasInterface::TRIMMING_CHARACTER_ELLIPSIS);
  text_frame->SetAlign(flags & ScriptableCanvas::TEXT_FLAG_CENTER ?
                          CanvasInterface::ALIGN_CENTER :
                       flags & ScriptableCanvas::TEXT_FLAG_RIGHT ?
                           CanvasInterface::ALIGN_RIGHT :
                       CanvasInterface::ALIGN_LEFT);
  text_frame->SetVAlign(flags & ScriptableCanvas::TEXT_FLAG_VCENTER ?
                            CanvasInterface::VALIGN_MIDDLE :
                        flags & ScriptableCanvas::TEXT_FLAG_BOTTOM ?
                            CanvasInterface::VALIGN_BOTTOM :
                        CanvasInterface::VALIGN_TOP);
  text_frame->SetColor(color, 1.0);
  // TODO: ScriptableCanvas::TEXT_FLAG_WORD_BREAK
  text_frame->SetWordWrap(!(flags & ScriptableCanvas::TEXT_FLAG_SINGLE_LINE));

  switch (font) {
    case ScriptableCanvas::FONT_NORMAL:
      text_frame->SetSize(kNormalFontSize);
      break;
    case ScriptableCanvas::FONT_BOLD:
      text_frame->SetSize(kNormalFontSize);
      text_frame->SetBold(true);
      break;
    case ScriptableCanvas::FONT_SNIPPET:
      text_frame->SetSize(kSnippetFontSize);
      break;
    case ScriptableCanvas::FONT_EXTRA_INFO:
      text_frame->SetSize(kExtraInfoFontSize);
      break;
    default:
      text_frame->SetSize(kNormalFontSize);
      break;
  }
}

void ScriptableCanvas::DrawText(double x, double y, double width, double height,
                                const char *text, const Color &color,
                                int flags, FontID font) {
  TextFrame text_frame(NULL, view_);
  SetupTextFrame(&text_frame, text, color, flags, font);
  text_frame.Draw(canvas_, x, y, width, height);
}

double ScriptableCanvas::GetTextWidth(const char *text, int flags, FontID font) {
  TextFrame text_frame(NULL, view_);
  SetupTextFrame(&text_frame, text, Color(0, 0, 0), flags, font);
  double width = 0, height = 0;
  text_frame.GetSimpleExtents(&width, &height);
  return width;
}

double ScriptableCanvas::GetTextHeight(const char *text, double width,
                                    int flags, FontID font) {
  TextFrame text_frame(NULL, view_);
  SetupTextFrame(&text_frame, text, Color(0, 0, 0), flags, font);
  double ret_width = 0, height = 0;
  text_frame.GetExtents(width, &ret_width, &height);
  return height;
}

void ScriptableCanvas::DrawLineWithColorName(double x1, double y1,
                                             double x2, double y2,
                                             const char *color) {
  Color c;
  Color::FromString(color, &c, NULL);
  DrawLine(x1, y1, x2, y2, c);
}

void ScriptableCanvas::DrawRectWithColorName(double x1, double y1,
                                             double width, double height,
                                             const char *line_color,
                                             const char *fill_color) {
  Color lc, fc;
  bool lc_parsed = Color::FromString(line_color, &lc, NULL);
  bool fc_parsed = Color::FromString(fill_color, &fc, NULL);
  DrawRect(x1, y1, width, height, lc_parsed ? &lc : NULL,
                                  fc_parsed ? &fc : NULL);
}

void ScriptableCanvas::DrawTextWithColorName(double x, double y,
                                             double width, double height,
                                             const char *text,
                                             const char *color,
                                             int flags, FontID font) {
  Color c;
  Color::FromString(color, &c, NULL);
  DrawText(x, y, width, height, text, c, flags, font);
}

ContentItem *ContentItem::CreateInstance(View *view) {
  return new ContentItem(view);
}

} // namespace ggadget
