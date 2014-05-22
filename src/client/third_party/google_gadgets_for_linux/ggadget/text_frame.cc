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

#include "text_frame.h"

#include "basic_element.h"
#include "color.h"
#include "gadget_consts.h"
#include "graphics_interface.h"
#include "slot.h"
#include "small_object.h"
#include "string_utils.h"
#include "texture.h"
#include "text_formats.h"
#include "text_renderer_interface.h"
#include "view.h"

namespace ggadget {

namespace {

FontInterface* CreateFontFromFormat(const GraphicsInterface* graphics,
                                    const TextFormat& format) {
  return graphics->NewFont(
      format.font(),
      format.size() * format.scale(),
      format.italic() ? FontInterface::STYLE_ITALIC :
                        FontInterface::STYLE_NORMAL,
      format.bold() ? FontInterface::WEIGHT_BOLD :
                      FontInterface::WEIGHT_NORMAL);
}

bool IsPlainText(const std::string& text) {
  return text.find("</") == text.npos ||
      text.find(">", text.find("</")) == text.npos;
}

}  // namespace

static const Color kDefaultColor(0, 0, 0);

static const char *kAlignNames[] = {
  "left", "center", "right", "justify"
};
static const char *kVAlignNames[] = {
  "top", "middle", "bottom"
};
static const char *kTrimmingNames[] = {
  "none",
  "character",
  "word",
  "character-ellipsis",
  "word-ellipsis",
  "path-ellipsis"
};

class TextFrame::Impl : public SmallObject<> {
 public:
  Impl(BasicElement *owner, View *view)
    : owner_(owner),
      view_(NULL),
      color_texture_(new Texture(kDefaultColor, 1.0)),
      on_theme_changed_connection_(NULL),
      width_(0.0),
      height_(0.0),
      trimming_(CanvasInterface::TRIMMING_NONE),
      align_(CanvasInterface::ALIGN_LEFT),
      valign_(CanvasInterface::VALIGN_TOP),
      size_is_default_(true),
      renderer_(NULL),
      rtl_(false),
      word_wrap_(false) {
    SetView(view);
  }

  ~Impl() {
    SetView(NULL);
    delete color_texture_;
    color_texture_ = NULL;
    ClearFont();
    if (renderer_) renderer_->Destroy();
  }

  void SetView(View *view) {
    if (view_ != view) {
      if (on_theme_changed_connection_) {
        on_theme_changed_connection_->Disconnect();
        on_theme_changed_connection_ = NULL;
      }

      view_ = view;
      if (view) {
        on_theme_changed_connection_ = view->ConnectOnThemeChangedEvent(
            NewSlot(this, &Impl::ResetFont));
        if (view_->GetGraphics()) {
          if (renderer_) renderer_->Destroy();
          renderer_ = view_->GetGraphics()->NewTextRenderer();
          if (renderer_) renderer_->SetTextAndFormat(text_, formats_);
        }
      }
      ResetFont();
    }
  }

  void ClearFont() {
    formats_.clear();
  }

  void ResetFont() {
    ClearFont();
    ResetExtents();
  }

  void ResetExtents() {
    width_ = height_ = 0;
    QueueDraw();
  }

  bool SetUpFont() {
    if (!view_)
      return false;

    GraphicsInterface *graphics = view_->GetGraphics();
    if (!graphics)
      return false;

    if (size_is_default_) {
      int default_size = view_->GetDefaultFontSize();
      if (default_size != size_) {
        size_ = default_size;
        ResetFont();
      }
    }

    if (renderer_) {
      renderer_->SetDefaultFormat(default_format_);
      renderer_->SetAlignment(align_);
      renderer_->SetVAlignment(valign_);
      renderer_->SetWordWrap(word_wrap_);
      renderer_->SetTrimming(trimming_);
    }
    if (text_.empty()) {
      width_ = height_ = 0;
    } else if (width_ == 0 && height_ == 0) {
      if (renderer_) {
        renderer_->SetLayoutRectangle(0, 0, -1, -1);
        renderer_->GetTextExtents(&width_, &height_);
      } else {
        FontInterface *font = CreateFontFromFormat(graphics, default_format_);
        CanvasInterface *canvas = graphics->NewCanvas(5, 5);
        int flags = 0;
        canvas->GetTextExtents(text_.c_str(), font, GetTextFlags(), 0,
                               &width_, &height_);
        canvas->Destroy();
        font->Destroy();
      }
    }
    return true;
  }

  void QueueDraw() {
    if (owner_)
      owner_->QueueDraw();
  }

  bool SetPlainText(const std::string& text) {
    if (text_ != text || formats_.size()) {
      text_ = text;
      ResetFont();
      if (renderer_)
        renderer_->SetTextAndFormat(text_, formats_);
      return true;
    }
    return false;
  }

  bool SetMarkUpText(const std::string& text) {
    if (mark_up_text_ != text) {
      ResetFont();
      mark_up_text_ = text;
      ParseMarkUpText(mark_up_text_, &default_format_, &text_, &formats_);
      if (renderer_)
        renderer_->SetTextAndFormat(text_, formats_);
      return true;
    }
    return false;
  }

  bool SetTextWithFormats(const std::string& text, const TextFormats& formats) {
    text_ = text;
    ResetFont();
    for (size_t i = 0; i < formats.size(); ++i) {
      formats_.push_back(formats[i]);
      formats_.back().format.set_default_format(&default_format_);
    }
    if (renderer_)
      renderer_->SetTextAndFormat(text_, formats_);
    return true;
  }

  int GetTextFlags() {
    int flags =
        (default_format_.strikeout() ?
            CanvasInterface::TEXT_FLAGS_STRIKEOUT : 0) |
        (default_format_.underline() ?
            CanvasInterface::TEXT_FLAGS_UNDERLINE : 0) |
        (word_wrap_ ? CanvasInterface::TEXT_FLAGS_WORDWRAP : 0);
    return flags;
  }

  BasicElement *owner_;
  View *view_;
  Texture *color_texture_;
  Connection *on_theme_changed_connection_;
  double width_;
  double height_;
  double size_;

  std::string text_;
  std::string mark_up_text_;

  TextFormat default_format_;
  TextFormats formats_;

  CanvasInterface::Trimming trimming_ : 3;
  CanvasInterface::Alignment align_   : 2;
  CanvasInterface::VAlignment valign_ : 2;
  bool size_is_default_               : 1;
  bool rtl_                           : 1;
  bool word_wrap_                     : 1;

  TextRendererInterface *renderer_;
};

TextFrame::TextFrame(BasicElement *owner, View *view)
    : impl_(new Impl(owner, view)) {
}

void TextFrame::RegisterClassProperties(
    TextFrame *(*delegate_getter)(BasicElement *),
    const TextFrame *(*delegate_getter_const)(BasicElement *)) {
  BasicElement *owner = impl_->owner_;
  ASSERT(owner);
  // Register Properties
  // All properties are registered except for GetText/SetText
  // since some elements call it "caption" while others call it "innerText."
  // Elements may also want to do special handling on SetText.
  owner->RegisterProperty("bold",
      NewSlot(&TextFrame::IsBold, delegate_getter_const),
      NewSlot(&TextFrame::SetBold, delegate_getter));
  owner->RegisterProperty("color",
      NewSlot(&TextFrame::GetColor, delegate_getter_const),
      NewSlot<void, const Variant &>(&TextFrame::SetColor, delegate_getter));
  owner->RegisterProperty("font",
      NewSlot(&TextFrame::GetFont, delegate_getter_const),
      NewSlot(&TextFrame::SetFont, delegate_getter));
  owner->RegisterProperty("italic",
      NewSlot(&TextFrame::IsItalic, delegate_getter_const),
      NewSlot(&TextFrame::SetItalic, delegate_getter));
  owner->RegisterProperty("size",
      NewSlot(&TextFrame::GetSize, delegate_getter_const),
      NewSlot(&TextFrame::SetSize, delegate_getter));
  owner->RegisterProperty("strikeout",
      NewSlot(&TextFrame::IsStrikeout, delegate_getter_const),
      NewSlot(&TextFrame::SetStrikeout, delegate_getter));
  owner->RegisterProperty("underline",
      NewSlot(&TextFrame::IsUnderline, delegate_getter_const),
      NewSlot(&TextFrame::SetUnderline, delegate_getter));
  owner->RegisterProperty("wordWrap",
      NewSlot(&TextFrame::IsWordWrap, delegate_getter_const),
      NewSlot(&TextFrame::SetWordWrap, delegate_getter));

  owner->RegisterStringEnumProperty("align",
      NewSlot(&TextFrame::GetAlign, delegate_getter_const),
      NewSlot(&TextFrame::SetAlign, delegate_getter),
      kAlignNames, arraysize(kAlignNames));
  owner->RegisterStringEnumProperty("vAlign",
      NewSlot(&TextFrame::GetVAlign, delegate_getter_const),
      NewSlot(&TextFrame::SetVAlign, delegate_getter),
      kVAlignNames, arraysize(kVAlignNames));
  owner->RegisterStringEnumProperty("trimming",
      NewSlot(&TextFrame::GetTrimming, delegate_getter_const),
      NewSlot(&TextFrame::SetTrimming, delegate_getter),
      kTrimmingNames, arraysize(kTrimmingNames));
}

TextFrame::~TextFrame() {
  delete impl_;
  impl_ = NULL;
}

void TextFrame::SetView(View *view) {
  impl_->SetView(view);
}

CanvasInterface::Alignment TextFrame::GetAlign() const {
  return impl_->align_;
}

void TextFrame::SetAlign(CanvasInterface::Alignment align) {
  if (align != impl_->align_) {
    impl_->align_ = align;
    impl_->QueueDraw();
  }
}

bool TextFrame::IsBold() const {
  return impl_->default_format_.bold();
}

void TextFrame::SetBold(bool bold) {
  if (bold != impl_->default_format_.bold()) {
    impl_->default_format_.set_bold(bold);
    impl_->ResetFont();
  }
}

Variant TextFrame::GetColor() const {
  return Variant(Texture::GetSrc(impl_->color_texture_));
}

void TextFrame::SetColor(const Variant &color) {
  delete impl_->color_texture_;
  impl_->color_texture_ = NULL;
  if (impl_->view_)
    impl_->color_texture_ = impl_->view_->LoadTexture(color);
  if (!impl_->color_texture_)
    impl_->color_texture_ = new Texture(kDefaultColor, 1.0);
  impl_->default_format_.set_foreground(
      Color(impl_->color_texture_->GetSrc().c_str()));
}

void TextFrame::SetColor(const Color &color, double opacity) {
  delete impl_->color_texture_;
  impl_->color_texture_ = new Texture(color, opacity);
  impl_->default_format_.set_foreground(color);
}

std::string TextFrame::GetFont() const {
  return impl_->default_format_.font();
}

void TextFrame::SetFont(const char *font) {
  if (impl_->default_format_.font() != font) {
    impl_->default_format_.set_font(font);
    impl_->ResetFont();
  }
}

bool TextFrame::IsItalic() const {
  return impl_->default_format_.italic();
}

void TextFrame::SetItalic(bool italic) {
  if (italic != impl_->default_format_.italic()) {
    impl_->default_format_.set_italic(italic);
    impl_->ResetFont();
  }
}

double TextFrame::GetSize() const {
  return impl_->size_is_default_ ? -1 : impl_->default_format_.size();
}

void TextFrame::SetSize(double size) {
  if (size == -1) {
    impl_->size_is_default_ = true;
    size = impl_->view_ ? impl_->view_->GetDefaultFontSize() : 0;
  } else {
    impl_->size_is_default_ = false;
  }
  if (size != impl_->default_format_.size()) {
    impl_->default_format_.set_size(size);
    impl_->ResetFont();
  }
}

double TextFrame::GetCurrentSize() const {
  return impl_->default_format_.size();
}

bool TextFrame::IsStrikeout() const {
  return impl_->default_format_.strikeout();
}

void TextFrame::SetStrikeout(bool strikeout) {
  if (strikeout != impl_->default_format_.strikeout()) {
    impl_->default_format_.set_strikeout(strikeout);
    impl_->ResetFont();
  }
}

CanvasInterface::Trimming TextFrame::GetTrimming() const {
  return impl_->trimming_;
}

void TextFrame::SetTrimming(CanvasInterface::Trimming trimming) {
  if (trimming != impl_->trimming_) {
    impl_->trimming_ = trimming;
    impl_->QueueDraw();
  }
}

bool TextFrame::IsUnderline() const {
  return impl_->default_format_.underline();
}

void TextFrame::SetUnderline(bool underline) {
  if (underline != impl_->default_format_.underline()) {
    impl_->default_format_.set_underline(underline);
    impl_->ResetFont();
  }
}

CanvasInterface::VAlignment TextFrame::GetVAlign() const {
  return impl_->valign_;
}

void TextFrame::SetVAlign(CanvasInterface::VAlignment valign) {
  if (valign != impl_->valign_) {
    impl_->valign_ = valign;
    impl_->QueueDraw();
  }
}

bool TextFrame::IsWordWrap() const {
  return impl_->word_wrap_;
}

void TextFrame::SetWordWrap(bool wrap) {
  if (wrap != impl_->word_wrap_) {
    impl_->word_wrap_ = wrap;
    impl_->ResetFont();
  }
}

std::string TextFrame::GetText() const {
  return impl_->text_;
}

bool TextFrame::SetText(const std::string &text) {
  if (IsPlainText(text))
    return impl_->SetPlainText(text);
  else
    return impl_->SetMarkUpText(text);
}

bool TextFrame::SetTextWithFormats(const std::string &text,
                                   const TextFormats &formats) {
  return impl_->SetTextWithFormats(text, formats);
}

void TextFrame::DrawWithTexture(CanvasInterface *canvas, double x, double y,
                                double width, double height, Texture *texture) {
  ASSERT(texture);
  impl_->SetUpFont();
  if (!impl_->text_.empty()) {
    if (impl_->renderer_) {
      impl_->renderer_->SetLayoutRectangle(x, y, width, height);
      texture->DrawText(canvas, impl_->renderer_);
    } else {
      FontInterface *font = CreateFontFromFormat(impl_->view_->GetGraphics(),
                                                 impl_->default_format_);
      texture->DrawText(canvas, x, y, width, height,
                        impl_->text_.c_str(), font,
                        impl_->align_, impl_->valign_,
                        impl_->trimming_, impl_->GetTextFlags());
      font->Destroy();
    }
  }
}

void TextFrame::Draw(CanvasInterface *canvas, double x, double y,
                     double width, double height) {
  DrawWithTexture(canvas, x, y, width, height, impl_->color_texture_);
}

void TextFrame::GetSimpleExtents(double *width, double *height) {
  impl_->SetUpFont();
  *width = impl_->width_;
  *height = impl_->height_;
}

void TextFrame::GetExtents(double in_width, double *width, double *height) {
  impl_->SetUpFont();
  *width = 0;
  *height = 0;
  if (in_width >= impl_->width_) {
    *width = impl_->width_;
    *height = impl_->height_;
  } else if (impl_->view_ && impl_->view_->GetGraphics()) {
    if (impl_->renderer_) {
      impl_->renderer_->SetLayoutRectangle(0, 0, in_width, -1);
      impl_->renderer_->GetTextExtents(width, height);
    } else {
      FontInterface *font = CreateFontFromFormat(impl_->view_->GetGraphics(),
                                                 impl_->default_format_);
      CanvasInterface *canvas = impl_->view_->GetGraphics()->NewCanvas(5, 5);
      canvas->GetTextExtents(impl_->text_.c_str(), font, impl_->GetTextFlags(),
                             in_width, width, height);
      canvas->Destroy();
      font->Destroy();
    }
  }
}

void TextFrame::DrawCaret(CanvasInterface* canvas,
                          int caret_pos, const Color& color) {
  impl_->SetUpFont();
  if (impl_->view_ && impl_->view_->GetGraphics() && impl_->renderer_)
    impl_->renderer_->DrawCaret(canvas, caret_pos, color);
  return;
}

bool TextFrame::IsRTL() const {
  return impl_->rtl_;
}

void TextFrame::SetRTL(bool rtl) {
  if (impl_->default_format_.text_rtl() != rtl) {
    impl_->default_format_.set_text_rtl(rtl);
    impl_->ResetFont();
  }
}

void TextFrame::SetDefaultFormat(const TextFormat& default_format) {
  impl_->default_format_ = default_format;
  impl_->ResetFont();
}

} // namespace ggadget
