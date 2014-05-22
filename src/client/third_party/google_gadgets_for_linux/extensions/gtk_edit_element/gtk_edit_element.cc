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
#include <ggadget/canvas_interface.h>
#include <ggadget/event.h>
#include <ggadget/logger.h>
#include <ggadget/scrolling_element.h>
#include <ggadget/slot.h>
#include <ggadget/texture.h>
#include <ggadget/view.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/element_factory.h>
#include <ggadget/scrollbar_element.h>
#include "gtk_edit_element.h"
#include "gtk_edit_impl.h"

#define Initialize gtk_edit_element_LTX_Initialize
#define Finalize gtk_edit_element_LTX_Finalize
#define RegisterElementExtension gtk_edit_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize gtk_edit_element extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize gtk_edit_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOGI("Register gtk_edit_element extension.");
    if (factory) {
      factory->RegisterElementClass(
          "edit", &ggadget::gtk::GtkEditElement::CreateInstance);
    }
    return true;
  }
}

namespace ggadget {
namespace gtk {

static const int kDefaultEditElementWidth = 60;
static const int kDefaultEditElementHeight = 16;
static const Color kDefaultBackgroundColor(1, 1, 1);

GtkEditElement::GtkEditElement(View *view, const char *name)
    : EditElementBase(view, name),
      impl_(new GtkEditImpl(this, ggadget::GetGlobalMainLoop(),
                            kDefaultEditElementWidth,
                            kDefaultEditElementHeight)) {
  impl_->SetBackground(new Texture(kDefaultBackgroundColor, 1.0));
  ConnectOnScrolledEvent(NewSlot(this, &GtkEditElement::OnScrolled));
}

GtkEditElement::~GtkEditElement() {
  delete impl_;
}

void GtkEditElement::Layout() {
  static int recurse_depth = 0;

  EditElementBase::Layout();
  int range, line_step, page_step, cur_pos;

  impl_->SetWidth(static_cast<int>(ceil(GetClientWidth())));
  impl_->SetHeight(static_cast<int>(ceil(GetClientHeight())));
  impl_->GetScrollBarInfo(&range, &line_step, &page_step, &cur_pos);

  bool changed = UpdateScrollBar(0, range);
  SetScrollYPosition(cur_pos);
  SetYLineStep(line_step);
  SetYPageStep(page_step);

  // See DivElement::Layout() impl for the reason of recurse_depth.
  if (changed && (range > 0 || recurse_depth < 2)) {
    recurse_depth++;
    // If the scrollbar display state was changed, then call Layout()
    // recursively to redo Layout.
    Layout();
    recurse_depth--;
  } else {
    // Call scrollbar's Layout() forcely, because scrollbar's position might be
    // updated afer calling UpdateScrollBar(), which is in charge of calling
    // scrollbar's Layout().
    ScrollBarElement *scrollbar = GetScrollBar();
    if (scrollbar)
      scrollbar->Layout();
  }
}

void GtkEditElement::MarkRedraw() {
  EditElementBase::MarkRedraw();
  impl_->MarkRedraw();
}

bool GtkEditElement::HasOpaqueBackground() const {
  const Texture *background = impl_->GetBackground();
  return background && background->IsFullyOpaque();
}

Variant GtkEditElement::GetBackground() const {
  return Variant(Texture::GetSrc(impl_->GetBackground()));
}

void GtkEditElement::SetBackground(const Variant &background) {
  impl_->SetBackground(GetView()->LoadTexture(background));
}

bool GtkEditElement::IsBold() const {
  return impl_->IsBold();
}

void GtkEditElement::SetBold(bool bold) {
  impl_->SetBold(bold);
}

std::string GtkEditElement::GetColor() const {
  return impl_->GetTextColor().ToString();
}

void GtkEditElement::SetColor(const char *color) {
  impl_->SetTextColor(Color(color));
}

std::string GtkEditElement::GetFont() const {
  return impl_->GetFontFamily();
}

void GtkEditElement::SetFont(const char *font) {
  impl_->SetFontFamily(font);
}

bool GtkEditElement::IsItalic() const {
  return impl_->IsItalic();
}

void GtkEditElement::SetItalic(bool italic) {
  impl_->SetItalic(italic);
}

bool GtkEditElement::IsMultiline() const {
  return impl_->IsMultiline();
}

void GtkEditElement::SetMultiline(bool multiline) {
  impl_->SetMultiline(multiline);
}

std::string GtkEditElement::GetPasswordChar() const {
  return impl_->GetPasswordChar();
}

void GtkEditElement::SetPasswordChar(const char *passwordChar) {
  impl_->SetPasswordChar(passwordChar);
}

bool GtkEditElement::IsStrikeout() const {
  return impl_->IsStrikeout();
}

void GtkEditElement::SetStrikeout(bool strikeout) {
  impl_->SetStrikeout(strikeout);
}

bool GtkEditElement::IsUnderline() const {
  return impl_->IsUnderline();
}

void GtkEditElement::SetUnderline(bool underline) {
  impl_->SetUnderline(underline);
}

std::string GtkEditElement::GetValue() const {
  return impl_->GetText();
}

void GtkEditElement::SetValue(const char *value) {
  impl_->SetText(value);
}

bool GtkEditElement::IsWordWrap() const {
  return impl_->IsWordWrap();
}

void GtkEditElement::SetWordWrap(bool wrap) {
  impl_->SetWordWrap(wrap);
}

bool GtkEditElement::IsReadOnly() const {
  return impl_->IsReadOnly();
}

void GtkEditElement::SetReadOnly(bool readonly) {
  impl_->SetReadOnly(readonly);
}

bool GtkEditElement::IsDetectUrls() const {
  // TODO
  return false;
}

void GtkEditElement::SetDetectUrls(bool /*detect_urls*/) {
  // TODO
}

void GtkEditElement::GetIdealBoundingRect(int *width, int *height) {
  int w, h;
  impl_->GetSizeRequest(&w, &h);
  if (width) *width = w;
  if (height) *height = h;
}

void GtkEditElement::Select(int start, int end) {
  impl_->Select(start, end);
  // If the edit box has no focus, then the selection will be reset when it
  // gains the focus by clicking on it. Then the selection will be useless.
  // FIXME: Is it the correct behaviour?
  GetView()->SetFocus(this);
}

void GtkEditElement::SelectAll() {
  impl_->SelectAll();
  // If the edit box has no focus, then the selection will be reset when it
  // gains the focus by clicking on it. Then the selection will be useless.
  // FIXME: Is it the correct behaviour?
  GetView()->SetFocus(this);
}

CanvasInterface::Alignment GtkEditElement::GetAlign() const {
  return impl_->GetAlign();
}

void GtkEditElement::SetAlign(CanvasInterface::Alignment align) {
  impl_->SetAlign(align);
}

CanvasInterface::VAlignment GtkEditElement::GetVAlign() const {
  return impl_->GetVAlign();
}

void GtkEditElement::SetVAlign(CanvasInterface::VAlignment valign) {
  impl_->SetVAlign(valign);
}

void GtkEditElement::DoDraw(CanvasInterface *canvas) {
  impl_->Draw(canvas);
  DrawScrollbar(canvas);
}

EventResult GtkEditElement::HandleMouseEvent(const MouseEvent &event) {
  if (EditElementBase::HandleMouseEvent(event) == EVENT_RESULT_HANDLED)
    return EVENT_RESULT_HANDLED;
  return impl_->OnMouseEvent(event);
}

EventResult GtkEditElement::HandleKeyEvent(const KeyboardEvent &event) {
  return impl_->OnKeyEvent(event);
}

EventResult GtkEditElement::HandleOtherEvent(const Event &event) {
  if (event.GetType() == Event::EVENT_FOCUS_IN) {
    impl_->FocusIn();
    return EVENT_RESULT_HANDLED;
  } else if(event.GetType() == Event::EVENT_FOCUS_OUT) {
    impl_->FocusOut();
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void GtkEditElement::GetDefaultSize(double *width, double *height) const {
  ASSERT(width && height);

  *width = kDefaultEditElementWidth;
  *height = kDefaultEditElementHeight;
}

void GtkEditElement::OnFontSizeChange() {
  impl_->OnFontSizeChange();
}

void GtkEditElement::OnScrolled() {
  DLOG("GtkEditElement::OnScrolled(%d)", GetScrollYPosition());
  impl_->ScrollTo(GetScrollYPosition());
}

BasicElement *GtkEditElement::CreateInstance(View *view, const char *name) {
  return new GtkEditElement(view, name);
}

} // namespace gtk
} // namespace ggadget
