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

#include <algorithm>
#include <cstdlib>
#include <string>
#include <cairo.h>
#include <pango/pango.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <ggadget/common.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/math_utils.h>
#include <ggadget/logger.h>
#include <ggadget/texture.h>
#include <ggadget/string_utils.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/view.h>
#include <ggadget/gtk/cairo_graphics.h>
#include <ggadget/gtk/cairo_canvas.h>
#include <ggadget/gtk/cairo_font.h>
#include "gtk_edit_impl.h"
#include "gtk_edit_element.h"

#ifndef PANGO_VERSION_CHECK
#define PANGO_VERSION_CHECK(a,b,c) 0
#endif

namespace ggadget {
namespace gtk {

static const int kInnerBorderX = 2;
static const int kInnerBorderY = 1;
static const int kCursorBlinkTimeout = 400;
static const double kStrongCursorBarWidth = 2;
static const double kStrongCursorBarHeight = 1;
static const double kWeakCursorBarWidth = 2;
static const double kWeakCursorBarHeight = 1;
static const Color kStrongCursorColor(0, 0, 0);
static const Color kWeakCursorColor(0.5, 0.5, 0.5);
static const Color kTextUnderCursorColor(1, 1, 1);
static const Color kDefaultTextColor(0, 0, 0);
static const Color kDefaultBackgroundColor(1, 1, 1);
static const Color kDefaultSelectionBackgroundColor(0.5, 0.5, 0.5);
static const Color kDefaultSelectionTextColor(1, 1, 1);
static const uint64_t kTripleClickTimeout = 500;

GtkEditImpl::GtkEditImpl(GtkEditElement *owner,
                         MainLoopInterface *main_loop,
                         int width, int height)
    : owner_(owner),
      main_loop_(main_loop),
      graphics_(owner->GetView()->GetGraphics()),
      im_context_(NULL),
      cached_layout_(NULL),
      preedit_attrs_(NULL),
      last_dblclick_time_(0),
      width_(width),
      height_(height),
      cursor_(0),
      preedit_cursor_(0),
      selection_bound_(0),
      scroll_offset_x_(0),
      scroll_offset_y_(0),
      cursor_blink_timer_(0),
      cursor_blink_status_(0),
      visible_(true),
      focused_(false),
      need_im_reset_(false),
      overwrite_(false),
      select_words_(false),
      select_lines_(false),
      button_(false),
      bold_(false),
      underline_(false),
      strikeout_(false),
      italic_(false),
      multiline_(false),
      wrap_(false),
      cursor_visible_(true),
      readonly_(false),
      content_modified_(false),
      selection_changed_(false),
      cursor_moved_(false),
      background_(new Texture(kDefaultBackgroundColor, 1)),
      text_color_(kDefaultTextColor),
      align_(CanvasInterface::ALIGN_LEFT),
      valign_(CanvasInterface::VALIGN_TOP),
      cursor_index_in_layout_(-1),
      content_region_(0.9) {
  ASSERT(main_loop_);
  ASSERT(graphics_);
  InitImContext();
}

GtkEditImpl::~GtkEditImpl() {
  if (im_context_)
    g_object_unref(im_context_);
  delete background_;

  if (cursor_blink_timer_)
    main_loop_->RemoveWatch(cursor_blink_timer_);

  ResetPreedit();
  ResetLayout();
}

void GtkEditImpl::Draw(CanvasInterface *canvas) {
  if (background_) {
    background_->Draw(canvas, 0, 0, width_, height_);
  }

  canvas->PushState();
  canvas->IntersectRectClipRegion(kInnerBorderX,
                                  kInnerBorderY,
                                  width_- kInnerBorderX,
                                  height_ - kInnerBorderY);
  DrawText(canvas);
  canvas->PopState();
  DrawCursor(canvas);

  last_selection_region_ = selection_region_;
  last_cursor_region_ = cursor_region_;
  last_content_region_ = content_region_;
}

void GtkEditImpl::FocusIn() {
  if (!focused_) {
    focused_ = true;
    if (!readonly_ && im_context_) {
      need_im_reset_ = true;
      gtk_im_context_focus_in(im_context_);
      UpdateIMCursorLocation();
    }
    selection_changed_ = true;
    cursor_moved_ = true;
    // Don't adjust scroll.
    QueueRefresh(false, NO_SCROLL);
  }
}

void GtkEditImpl::FocusOut() {
  if (focused_) {
    focused_ = false;
    if (!readonly_ && im_context_) {
      need_im_reset_ = true;
      gtk_im_context_focus_out(im_context_);
    }
    selection_changed_ = true;
    cursor_moved_ = true;
    // Don't adjust scroll.
    QueueRefresh(false, NO_SCROLL);
  }
}

void GtkEditImpl::SetWidth(int width) {
  if (width_ != width) {
    width_ = width;
    if (width_ <= kInnerBorderX * 2)
      width_ = kInnerBorderX * 2 + 1;
    QueueRefresh(true, MINIMAL_ADJUST);
  }
}

int GtkEditImpl::GetWidth() {
  return width_;
}

void GtkEditImpl::SetHeight(int height) {
  if (height_ != height) {
    height_ = height;
    if (height_ <= kInnerBorderY * 2)
      height_ = kInnerBorderY * 2 + 1;
    QueueRefresh(true, MINIMAL_ADJUST);
  }
}

int GtkEditImpl::GetHeight() {
  return height_;
}

void GtkEditImpl::GetSizeRequest(int *width, int *height) {
  int layout_width, layout_height;
  PangoLayout *layout = EnsureLayout();

  pango_layout_get_pixel_size(layout, &layout_width, &layout_height);

  layout_width += kInnerBorderX * 2;
  layout_height += kInnerBorderY * 2;

  if (wrap_ && layout_width < width_)
    layout_width = width_;

  if (width)
    *width = layout_width;
  if (height)
    *height = layout_height;
}

void GtkEditImpl::SetBold(bool bold) {
  if (bold_ != bold) {
    bold_ = bold;
    QueueRefresh(true, MINIMAL_ADJUST);
  }
}

bool GtkEditImpl::IsBold() {
  return bold_;
}

void GtkEditImpl::SetItalic(bool italic) {
  if (italic_ != italic) {
    italic_ = italic;
    QueueRefresh(true, MINIMAL_ADJUST);
  }
}

bool GtkEditImpl::IsItalic() {
  return italic_;
}

void GtkEditImpl::SetStrikeout(bool strikeout) {
  if (strikeout_ != strikeout) {
    strikeout_ = strikeout;
    QueueRefresh(true, MINIMAL_ADJUST);
  }
}

bool GtkEditImpl::IsStrikeout() {
  return strikeout_;
}

void GtkEditImpl::SetUnderline(bool underline) {
  if (underline_ != underline) {
    underline_ = underline;
    QueueRefresh(true, MINIMAL_ADJUST);
  }
}

bool GtkEditImpl::IsUnderline() {
  return underline_;
}

void GtkEditImpl::SetMultiline(bool multiline) {
  if (multiline_ != multiline) {
    multiline_ = multiline;
    if (!multiline_)
      SetText(CleanupLineBreaks(text_.c_str()).c_str());
    QueueRefresh(true, CENTER_CURSOR);
  }
}

bool GtkEditImpl::IsMultiline() {
  return multiline_;
}

void GtkEditImpl::SetWordWrap(bool wrap) {
  if (wrap_ != wrap) {
    wrap_ = wrap;
    QueueRefresh(true, CENTER_CURSOR);
  }
}

bool GtkEditImpl::IsWordWrap() {
  return wrap_;
}

void GtkEditImpl::SetReadOnly(bool readonly) {
  if (readonly_ != readonly) {
    readonly_ = readonly;
    if (readonly_) {
      if (im_context_) {
        if (focused_)
          gtk_im_context_focus_out(im_context_);
        g_object_unref(im_context_);
        im_context_ = NULL;
      }
      ResetPreedit();
    } else {
      ResetPreedit();
      InitImContext();
      if (focused_)
        gtk_im_context_focus_in(im_context_);
    }
  }
  QueueRefresh(false, NO_SCROLL);
}

bool GtkEditImpl::IsReadOnly() {
  return readonly_;
}

void GtkEditImpl::SetText(const char* text) {
  const char *end = NULL;
  g_utf8_validate(text, -1, &end);

  std::string txt((text && *text && end > text) ? std::string(text, end) : "");
  if (txt == text_)
    return; // prevent some redraws

  text_ = multiline_ ? txt : CleanupLineBreaks(txt.c_str());
  cursor_ = 0;
  selection_bound_ = 0;
  need_im_reset_ = true;
  ResetImContext();
  QueueRefresh(true, MINIMAL_ADJUST);
  owner_->FireOnChangeEvent();
}

std::string GtkEditImpl::GetText() {
  return text_;
}

void GtkEditImpl::SetBackground(Texture *background) {
  if (background_)
    delete background_;
  background_ = background;
  QueueRefresh(false, NO_SCROLL);
}

const Texture *GtkEditImpl::GetBackground() {
  return background_;
}

void GtkEditImpl::SetTextColor(const Color &color) {
  text_color_ = color;
  content_modified_ = true;
  QueueRefresh(false, NO_SCROLL);
}

Color GtkEditImpl::GetTextColor() {
  return text_color_;
}

void GtkEditImpl::SetFontFamily(const char *font) {
  if (AssignIfDiffer(font, &font_family_))
    QueueRefresh(true, MINIMAL_ADJUST);
}

std::string GtkEditImpl::GetFontFamily() {
  return font_family_;
}

void GtkEditImpl::OnFontSizeChange() {
  QueueRefresh(true, MINIMAL_ADJUST);
}

void GtkEditImpl::SetPasswordChar(const char *c) {
  if (c == NULL || *c == 0 || !IsLegalUTF8Char(c, GetUTF8CharLength(c))) {
    SetVisibility(true);
    password_char_.clear();
  } else {
    SetVisibility(false);
    password_char_.assign(c, GetUTF8CharLength(c));
  }
  QueueRefresh(true, CENTER_CURSOR);
}

std::string GtkEditImpl::GetPasswordChar() {
  return password_char_;
}

bool GtkEditImpl::IsScrollBarRequired() {
  int request_height;
  GetSizeRequest(NULL, &request_height);
  return height_ >= request_height;
}

void GtkEditImpl::GetScrollBarInfo(int *range, int *line_step,
                               int *page_step, int *cur_pos) {
  PangoLayout *layout = EnsureLayout();
  int nlines = pango_layout_get_line_count(layout);

  // Only enable scrolling when there are more than one lines.
  if (nlines > 1) {
    int request_height;
    int real_height = height_ - kInnerBorderY * 2;
    pango_layout_get_pixel_size(layout, NULL, &request_height);
    if (range)
      *range = (request_height > real_height? (request_height - real_height) : 0);
    if (line_step) {
      *line_step = request_height / nlines;
      if (*line_step == 0) *line_step = 1;
    }
    if (page_step)
      *page_step = real_height;
    if (cur_pos)
      *cur_pos = - scroll_offset_y_;
  } else {
    if (range) *range = 0;
    if (line_step) *line_step = 0;
    if (page_step) *page_step = 0;
    if (cur_pos) *cur_pos = 0;
  }
}

void GtkEditImpl::ScrollTo(int position) {
  int request_height;
  int real_height = height_ - kInnerBorderY * 2;
  PangoLayout *layout = EnsureLayout();
  pango_layout_get_pixel_size(layout, NULL, &request_height);

  if (request_height > real_height) {
    if (position < 0)
      position = 0;
    else if (position > request_height - real_height)
      position = request_height - real_height;

    scroll_offset_y_ = -position;
    content_modified_ = true;
    QueueRefresh(false, NO_SCROLL);
  }
}

void GtkEditImpl::MarkRedraw() {
  content_modified_ = true;
  QueueRefresh(false, NO_SCROLL);
}

EventResult GtkEditImpl::OnMouseEvent(const MouseEvent &event) {
  // Only handle mouse events with left button down.
  if (event.GetButton() != MouseEvent::BUTTON_LEFT)
    return EVENT_RESULT_UNHANDLED;

  ResetImContext();
  Event::Type type = event.GetType();

  int x = static_cast<int>(round(event.GetX())) -
            kInnerBorderX - scroll_offset_x_;
  int y = static_cast<int>(round(event.GetY())) -
            kInnerBorderY - scroll_offset_y_;
  int index = XYToTextIndex(x, y);
  int sel_start, sel_end;
  GetSelectionBounds(&sel_start, &sel_end);

  uint64_t current_time = main_loop_->GetCurrentTime();
  if (type == Event::EVENT_MOUSE_DOWN &&
      current_time - last_dblclick_time_ <= kTripleClickTimeout) {
    SelectLine();
  } else if (type == Event::EVENT_MOUSE_DBLCLICK) {
    SelectWord();
    last_dblclick_time_ = current_time;
  } else if (type == Event::EVENT_MOUSE_DOWN) {
    if (event.GetModifier() & Event::MODIFIER_SHIFT) {
      // If current click position is inside the selection range, then just
      // cancel the selection.
      if (index > sel_start && index < sel_end)
        SetCursor(index);
      else if (index <= sel_start)
        SetSelectionBounds(sel_end, index);
      else if (index >= sel_end)
        SetSelectionBounds(sel_start, index);
    } else {
      SetCursor(index);
    }
  } else if (type == Event::EVENT_MOUSE_MOVE) {
    SetSelectionBounds(selection_bound_, index);
  }
  QueueRefresh(false, MINIMAL_ADJUST);
  return EVENT_RESULT_HANDLED;
}

EventResult GtkEditImpl::OnKeyEvent(const KeyboardEvent &event) {
  GdkEventKey *gdk_event = static_cast<GdkEventKey *>(event.GetOriginalEvent());
  ASSERT(gdk_event);

  Event::Type type = event.GetType();
  // Cause the cursor to stop blinking for a while.
  cursor_blink_status_ = 4;

  if (!readonly_ && im_context_ && type != Event::EVENT_KEY_PRESS &&
      gtk_im_context_filter_keypress(im_context_, gdk_event)) {
    need_im_reset_ = true;
    QueueRefresh(false, MINIMAL_ADJUST);
    return EVENT_RESULT_HANDLED;
  }

  if (type == Event::EVENT_KEY_UP)
    return EVENT_RESULT_UNHANDLED;

  unsigned int keyval = gdk_event->keyval;
  bool shift = (gdk_event->state & GDK_SHIFT_MASK);
  bool ctrl = (gdk_event->state & GDK_CONTROL_MASK);

  // DLOG("GtkEditImpl::OnKeyEvent(%d, shift:%d ctrl:%d)", keyval, shift, ctrl);

  if (type == Event::EVENT_KEY_DOWN) {
    if (keyval == GDK_Left || keyval == GDK_KP_Left) {
      if (!ctrl) MoveCursor(VISUALLY, -1, shift);
      else MoveCursor(WORDS, -1, shift);
    } else if (keyval == GDK_Right || keyval == GDK_KP_Right) {
      if (!ctrl) MoveCursor(VISUALLY, 1, shift);
      else MoveCursor(WORDS, 1, shift);
    } else if (keyval == GDK_Up || keyval == GDK_KP_Up) {
      MoveCursor(DISPLAY_LINES, -1, shift);
    } else if (keyval == GDK_Down || keyval == GDK_KP_Down) {
      MoveCursor(DISPLAY_LINES, 1, shift);
    } else if (keyval == GDK_Home || keyval == GDK_KP_Home) {
      if (!ctrl) MoveCursor(DISPLAY_LINE_ENDS, -1, shift);
      else MoveCursor(BUFFER, -1, shift);
    } else if (keyval == GDK_End || keyval == GDK_KP_End) {
      if (!ctrl) MoveCursor(DISPLAY_LINE_ENDS, 1, shift);
      else MoveCursor(BUFFER, 1, shift);
    } else if (keyval == GDK_Page_Up || keyval == GDK_KP_Page_Up) {
      if (!ctrl) MoveCursor(PAGES, -1, shift);
      else MoveCursor(BUFFER, -1, shift);
    } else if (keyval == GDK_Page_Down || keyval == GDK_KP_Page_Down) {
      if (!ctrl) MoveCursor(PAGES, 1, shift);
      else MoveCursor(BUFFER, 1, shift);
    } else if ((keyval == GDK_x && ctrl && !shift) ||
               (keyval == GDK_Delete && shift && !ctrl)) {
      CutClipboard();
    } else if ((keyval == GDK_c && ctrl && !shift) ||
               (keyval == GDK_Insert && ctrl && !shift)) {
      CopyClipboard();
    } else if ((keyval == GDK_v && ctrl && !shift) ||
               (keyval == GDK_Insert && shift && !ctrl)) {
      PasteClipboard();
    } else if (keyval == GDK_BackSpace) {
      BackSpace();
    } else if (keyval == GDK_Delete && !shift) {
      Delete();
    } else if (keyval == GDK_Insert && !shift && !ctrl) {
      ToggleOverwrite();
    } else {
      return EVENT_RESULT_UNHANDLED;
    }
  } else { // EVENT_KEY_PRESS
    if (keyval == GDK_Return || keyval == GDK_KP_Enter) {
      // If multiline_ is unset, just ignore new_line.
      if (multiline_)
        EnterText("\n");
      else
        return EVENT_RESULT_UNHANDLED;
    } else {
      return EVENT_RESULT_UNHANDLED;
    }
  }

  QueueRefresh(false, CENTER_CURSOR);
  return EVENT_RESULT_HANDLED;
}

//private =================================================================

void GtkEditImpl::QueueDraw() {
  if (content_modified_) {
    UpdateContentRegion();
    if (!last_content_region_.IsEmpty())
      owner_->QueueDrawRegion(last_content_region_);
    if (!content_region_.IsEmpty())
      owner_->QueueDrawRegion(content_region_);
    content_modified_ = false;
    selection_changed_ = true;
    cursor_moved_ = true;
  }

  if (selection_changed_) {
    UpdateSelectionRegion();
    if (!last_selection_region_.IsEmpty())
      owner_->QueueDrawRegion(last_selection_region_);
    if (!selection_region_.IsEmpty())
      owner_->QueueDrawRegion(selection_region_);
    selection_changed_ = false;
  }

  if (cursor_moved_) {
    UpdateCursorRegion();
    if (!last_cursor_region_.IsEmpty())
      owner_->QueueDrawRegion(last_cursor_region_);
    if (!cursor_region_.IsEmpty())
      owner_->QueueDrawRegion(cursor_region_);
    cursor_moved_ = false;
  }
}

void GtkEditImpl::ResetLayout() {
  if (cached_layout_) {
    g_object_unref(cached_layout_);
    cached_layout_ = NULL;
    content_modified_ = true;
    cursor_index_in_layout_ = -1;
  }
}

PangoLayout* GtkEditImpl::EnsureLayout() {
  if (!cached_layout_) {
    cached_layout_ = CreateLayout();
  }
  return cached_layout_;
}

PangoLayout* GtkEditImpl::CreateLayout() {
  // Creates the pango layout with a temporary canvas that is not zoomed.
  CairoCanvas *canvas = new CairoCanvas(1.0, 1, 1, CAIRO_FORMAT_ARGB32);
  PangoLayout *layout = pango_cairo_create_layout(canvas->GetContext());
  canvas->Destroy();
  PangoAttrList *tmp_attrs = pango_attr_list_new();
  std::string tmp_string;

  /* Set necessary parameters */
  if (wrap_) {
    pango_layout_set_width(layout, (width_ - kInnerBorderX * 2) * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
  } else {
    pango_layout_set_width(layout, -1);
  }

  pango_layout_set_single_paragraph_mode(layout, !multiline_);

  if (visible_) {
    size_t cursor_index = static_cast<size_t>(cursor_);
    size_t preedit_length = preedit_.length();
    tmp_string = text_;

    if (preedit_length) {
      tmp_string.insert(cursor_index, preedit_);
      if (preedit_attrs_) {
        pango_attr_list_splice(tmp_attrs, preedit_attrs_,
                               static_cast<int>(cursor_index),
                               static_cast<int>(preedit_length));
      }
    }
  } else {
    // Invisible mode doesn't support preedit string.
    ASSERT(preedit_.length() == 0);
    size_t nchars = g_utf8_strlen(text_.c_str(), text_.length());
    tmp_string.reserve(password_char_.length() * nchars);
    for (size_t i = 0; i < nchars; ++i) {
      tmp_string.append(password_char_);
    }
  }

  pango_layout_set_text(layout, tmp_string.c_str(),
                        static_cast<int>(tmp_string.length()));

  /* Set necessary attributes */
  PangoAttribute *attr;
  if (underline_) {
    attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
    attr->start_index = 0;
    attr->end_index = static_cast<guint>(tmp_string.length());
    pango_attr_list_insert(tmp_attrs, attr);
  }
  if (strikeout_) {
    attr = pango_attr_strikethrough_new(TRUE);
    attr->start_index = 0;
    attr->end_index = static_cast<guint>(tmp_string.length());
    pango_attr_list_insert(tmp_attrs, attr);
  }
  /* Set font desc */
  {
    /* safe to down_cast here, because we know the actual implementation. */
    CairoFont *font = down_cast<CairoFont*>(
        graphics_->NewFont(
            font_family_.empty() ? kDefaultFontName : font_family_.c_str(),
            owner_->GetCurrentSize(),
            italic_ ? FontInterface::STYLE_ITALIC : FontInterface::STYLE_NORMAL,
            bold_ ? FontInterface::WEIGHT_BOLD : FontInterface::WEIGHT_NORMAL));
    ASSERT(font);
    attr = pango_attr_font_desc_new(font->GetFontDescription());
    attr->start_index = 0;
    attr->end_index = static_cast<guint>(tmp_string.length());
    pango_attr_list_insert(tmp_attrs, attr);
    font->Destroy();
  }
  pango_layout_set_attributes(layout, tmp_attrs);
  pango_attr_list_unref(tmp_attrs);

  /* Set alignment according to text direction. Only set layout's alignment
   * when it's not wrapped and in single line mode.
   */
  if (!wrap_ && pango_layout_get_line_count(layout) <= 1 &&
      align_ != CanvasInterface::ALIGN_CENTER) {
    PangoDirection dir;
    if (visible_) {
      dir = pango_find_base_dir(tmp_string.c_str(),
                                static_cast<int>(tmp_string.length()));
    } else {
      dir = PANGO_DIRECTION_NEUTRAL;
    }

    if (dir == PANGO_DIRECTION_NEUTRAL) {
      GtkWidget *widget = GetWidgetAndCursorLocation(NULL);
      if (widget && gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL)
        dir = PANGO_DIRECTION_RTL;
      else
        dir = PANGO_DIRECTION_LTR;
    }

    // If wordWrap is false then "justify" alignment has no effect.
    PangoAlignment pango_align = (align_ == CanvasInterface::ALIGN_RIGHT ?
                                  PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT);

    // Invert the alignment if text direction is right to left.
    if (dir == PANGO_DIRECTION_RTL)
      pango_align = (align_ == CanvasInterface::ALIGN_RIGHT ?
                     PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT);

    pango_layout_set_alignment(layout, pango_align);
    pango_layout_set_justify(layout, FALSE);
  } else if (align_ == CanvasInterface::ALIGN_JUSTIFY) {
    pango_layout_set_justify(layout, TRUE);
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
  } else if (align_ == CanvasInterface::ALIGN_RIGHT) {
    pango_layout_set_justify(layout, FALSE);
    pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
  } else if (align_ == CanvasInterface::ALIGN_CENTER) {
    pango_layout_set_justify(layout, FALSE);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
  } else {
    pango_layout_set_justify(layout, FALSE);
    pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
  }

  return layout;
}

void GtkEditImpl::AdjustScroll(AdjustScrollPolicy policy) {
  if (policy == NO_SCROLL) return;

  int old_offset_x = scroll_offset_x_;
  int old_offset_y = scroll_offset_y_;
  int display_width = width_ - kInnerBorderX * 2;
  int display_height = height_ - kInnerBorderY * 2;

  PangoLayout *layout = EnsureLayout();
  int text_width, text_height;
  pango_layout_get_pixel_size(layout, &text_width, &text_height);

  PangoRectangle strong;
  PangoRectangle weak;
  GetCursorLocationInLayout(&strong, &weak);

  if (!wrap_ && display_width >= text_width) {
    PangoAlignment align = pango_layout_get_alignment(layout);
    if (align == PANGO_ALIGN_RIGHT)
      scroll_offset_x_ = display_width - text_width;
    else if (align == PANGO_ALIGN_CENTER)
      scroll_offset_x_ = (display_width - text_width) / 2;
    else
      scroll_offset_x_ = 0;
  } else {
    if (scroll_offset_x_ + strong.x > display_width) {
      if (policy == CENTER_CURSOR) {
        scroll_offset_x_ = std::max(display_width - text_width,
                                    display_width / 2 - strong.x);
      } else {
        scroll_offset_x_ = display_width - strong.x;
      }
    }
    if (!wrap_ && scroll_offset_x_ + text_width < display_width) {
      scroll_offset_x_ = display_width - text_width;
    }
    if (scroll_offset_x_ + strong.x < 0) {
      if (policy == CENTER_CURSOR) {
        scroll_offset_x_ = std::min(0, display_width / 2 - strong.x);
      } else {
        scroll_offset_x_ = -strong.x;
      }
    }

    if (std::abs(weak.x - strong.x) < display_width) {
      if (scroll_offset_x_ + weak.x < 0)
        scroll_offset_x_ = - weak.x;
      else if (scroll_offset_x_ + weak.x > display_width)
        scroll_offset_x_ = display_width - weak.x;
    }
  }

  if (display_height >= text_height) {
    if (valign_ == CanvasInterface::VALIGN_TOP)
      scroll_offset_y_ = 0;
    else if (valign_ == CanvasInterface::VALIGN_MIDDLE)
      scroll_offset_y_ = (display_height - text_height) / 2;
    else
      scroll_offset_y_ = display_height - text_height;
  } else {
    if (scroll_offset_y_ + strong.y + strong.height > display_height)
      scroll_offset_y_ = display_height - strong.y - strong.height;

    if (scroll_offset_y_ + text_height < display_height)
      scroll_offset_y_ = display_height - text_height;

    if (scroll_offset_y_ + strong.y < 0)
      scroll_offset_y_ = -strong.y;
  }

  if (old_offset_x != scroll_offset_x_ || old_offset_y != scroll_offset_y_)
    content_modified_ = true;
}

void GtkEditImpl::QueueRefresh(bool relayout, AdjustScrollPolicy policy) {
  // DLOG("GtkEditImpl::QueueRefresh(%d,%d)", relayout, policy);
  if (relayout)
    ResetLayout();

  if (policy != NO_SCROLL)
    AdjustScroll(policy);

  QueueDraw();
  QueueCursorBlink();
}

void GtkEditImpl::ResetImContext() {
  if (need_im_reset_) {
    need_im_reset_ = false;
    if (im_context_)
      gtk_im_context_reset(im_context_);
    ResetPreedit();
  }
}

void GtkEditImpl::ResetPreedit() {
  // Reset layout if there were some content in preedit string
  if (preedit_.length())
    ResetLayout();

  preedit_.clear();
  preedit_cursor_ = 0;
  if (preedit_attrs_) {
    pango_attr_list_unref(preedit_attrs_);
    preedit_attrs_ = NULL;
  }
}

void GtkEditImpl::InitImContext() {
  if (im_context_)
    g_object_unref(im_context_);

  im_context_ = gtk_im_multicontext_new();

  g_signal_connect(im_context_, "commit",
      G_CALLBACK(CommitCallback), this);

  if (visible_) {
    gtk_im_context_set_use_preedit(im_context_, TRUE);

    g_signal_connect(im_context_, "retrieve-surrounding",
        G_CALLBACK(RetrieveSurroundingCallback), this);
    g_signal_connect(im_context_, "delete-surrounding",
        G_CALLBACK(DeleteSurroundingCallback), this);
    g_signal_connect(im_context_, "preedit-start",
        G_CALLBACK(PreeditStartCallback), this);
    g_signal_connect(im_context_, "preedit-changed",
        G_CALLBACK(PreeditChangedCallback), this);
    g_signal_connect(im_context_, "preedit-end",
        G_CALLBACK(PreeditEndCallback), this);
  } else {
    gtk_im_context_set_use_preedit(im_context_, FALSE);
  }
}

void GtkEditImpl::SetVisibility(bool visible) {
  if (visible_ != visible) {
    visible_ = visible;

    if (!readonly_) {
      if (focused_)
        gtk_im_context_focus_out(im_context_);

      InitImContext();
      ResetPreedit();

      if (focused_)
        gtk_im_context_focus_in(im_context_);
    }

    ResetLayout();
  }
}

bool GtkEditImpl::IsCursorBlinking() {
  return (focused_ && !readonly_ && selection_bound_ == cursor_);
}

void GtkEditImpl::QueueCursorBlink() {
  if (IsCursorBlinking()) {
    if (!cursor_blink_timer_)
      cursor_blink_timer_ = main_loop_->AddTimeoutWatch(kCursorBlinkTimeout,
          new WatchCallbackSlot(NewSlot(this, &GtkEditImpl::CursorBlinkCallback)));
  } else {
    if (cursor_blink_timer_) {
      main_loop_->RemoveWatch(cursor_blink_timer_);
      cursor_blink_timer_ = 0;
    }

    cursor_visible_ = true;
  }
}

bool GtkEditImpl::CursorBlinkCallback(int timer_id) {
  GGL_UNUSED(timer_id);
  -- cursor_blink_status_;
  if (cursor_blink_status_ < 0)
    cursor_blink_status_ = 2;

  if (cursor_blink_status_ > 0)
    ShowCursor();
  else
    HideCursor();

  return true;
}

void GtkEditImpl::ShowCursor() {
  if (!cursor_visible_) {
    cursor_visible_ = true;
    if (focused_ && !readonly_) {
      cursor_moved_ = true;
      QueueDraw();
    }
  }
}

void GtkEditImpl::HideCursor() {
  if (cursor_visible_) {
    cursor_visible_ = false;
    if (focused_ && !readonly_) {
      cursor_moved_ = true;
      QueueDraw();
    }
  }
}

void GtkEditImpl::DrawCursor(CanvasInterface *canvas) {
  if (!cursor_visible_ || !focused_) return;

  PangoRectangle strong;
  PangoRectangle weak;
  GetCursorLocationInLayout(&strong, &weak);
  canvas->PushState();
  canvas->TranslateCoordinates(kInnerBorderX + scroll_offset_x_,
                               kInnerBorderY + scroll_offset_y_);

  // Draw strong cursor.
  // TODO: Is the color ok?
  canvas->DrawFilledRect(strong.x, strong.y, strong.width, strong.height,
                         kStrongCursorColor);

  if (strong.width > 1) {
    // Block cursor, ignore weak cursor.
    PangoLayout *layout = EnsureLayout();
    CairoCanvas *cairo_canvas = down_cast<CairoCanvas *>(canvas);
    cairo_t *cr = cairo_canvas->GetContext();
    cairo_rectangle(cr, strong.x, strong.y, strong.width, strong.height);
    cairo_clip(cr);
    cairo_set_source_rgb(cr,
                         kTextUnderCursorColor.red,
                         kTextUnderCursorColor.green,
                         kTextUnderCursorColor.blue);
    pango_cairo_show_layout(cr, layout);
  } else {
    // Draw a small arror towards weak cursor
    if (strong.x > weak.x) {
      canvas->DrawFilledRect(strong.x - kStrongCursorBarWidth, strong.y,
                             kStrongCursorBarWidth, kStrongCursorBarHeight,
                             kStrongCursorColor);
    } else if (strong.x < weak.x) {
      canvas->DrawFilledRect(strong.x + strong.width, strong.y,
                             kStrongCursorBarWidth, kStrongCursorBarHeight,
                             kStrongCursorColor);
    }

    if (strong.x != weak.x ) {
      // Draw weak cursor.
      // TODO: Is the color ok?
      canvas->DrawFilledRect(weak.x, weak.y, weak.width, weak.height,
                             kWeakCursorColor);
      // Draw a small arror towards strong cursor
      if (weak.x > strong.x) {
        canvas->DrawFilledRect(weak.x - kWeakCursorBarWidth, weak.y,
                               kWeakCursorBarWidth, kWeakCursorBarHeight,
                               kWeakCursorColor);
      } else {
        canvas->DrawFilledRect(weak.x + weak.width, weak.y,
                               kWeakCursorBarWidth, kWeakCursorBarHeight,
                               kWeakCursorColor);
      }
    }
  }

  canvas->PopState();
}

void GtkEditImpl::GetCursorRects(Rectangle *strong, Rectangle *weak) {
  PangoRectangle strong_pos;
  PangoRectangle weak_pos;
  GetCursorLocationInLayout(&strong_pos, &weak_pos);

  strong->x =
      strong_pos.x + kInnerBorderX + scroll_offset_x_ - kStrongCursorBarWidth;
  strong->w = kStrongCursorBarWidth * 2 + strong_pos.width;
  strong->y = strong_pos.y + kInnerBorderY + scroll_offset_y_ - 1;
  strong->h = strong_pos.height + 2;

  if (weak_pos.x != strong_pos.x) {
    weak->x =
        weak_pos.x + kInnerBorderX + scroll_offset_x_ - kWeakCursorBarWidth;
    weak->w = kWeakCursorBarWidth * 2 + weak_pos.width;
    weak->y = weak_pos.y+ kInnerBorderY + scroll_offset_y_ - 1;
    weak->h = weak_pos.height + 2;
  } else {
    *weak = *strong;
  }
}

void GtkEditImpl::UpdateCursorRegion() {
  cursor_region_.Clear();

  Rectangle strong, weak;
  GetCursorRects(&strong, &weak);

  cursor_region_.AddRectangle(strong);
  cursor_region_.AddRectangle(weak);
}

void GtkEditImpl::UpdateSelectionRegion() {
  selection_region_.Clear();

  // Selection in a single line may be not continual, so we use pango to
  // get the x-ranges of each selection range in one line, and draw them
  // separately.
  int start_index, end_index;
  if (GetSelectionBounds(&start_index, &end_index)) {
    PangoLayout *layout = EnsureLayout();
    PangoRectangle line_extents, pos;
    int draw_start, draw_end;
    int *ranges;
    int n_ranges;
    int n_lines = pango_layout_get_line_count(layout);
    double x, y, w, h;

    start_index = TextIndexToLayoutIndex(start_index, false);
    end_index = TextIndexToLayoutIndex(end_index, false);

    for(int line_index = 0; line_index < n_lines; ++line_index) {
#if PANGO_VERSION_CHECK(1,16,0)
      PangoLayoutLine *line = pango_layout_get_line_readonly(layout, line_index);
#else
      PangoLayoutLine *line = pango_layout_get_line(layout, line_index);
#endif
      if (line->start_index + line->length < start_index)
        continue;
      if (end_index < line->start_index)
        break;
      draw_start = std::max(start_index, line->start_index);
      draw_end = std::min(end_index, line->start_index + line->length);
      pango_layout_line_get_x_ranges(line, draw_start, draw_end,
                                     &ranges, &n_ranges);
      pango_layout_line_get_pixel_extents(line, NULL, &line_extents);
      pango_layout_index_to_pos(layout, line->start_index,  &pos);
      for(int i = 0; i < n_ranges; ++i) {
        x = kInnerBorderX + scroll_offset_x_ + PANGO_PIXELS(ranges[i * 2]);
        y = kInnerBorderY + scroll_offset_y_ + PANGO_PIXELS(pos.y);
        w = PANGO_PIXELS(ranges[i * 2 + 1] - ranges[i * 2]);
        h = line_extents.height;
        if (x < width_ && x + w > 0 && y < height_ && y + h > 0) {
          selection_region_.AddRectangle(Rectangle(x, y, w, h));
        }
      }
      g_free(ranges);
    }
  }
}

void GtkEditImpl::UpdateContentRegion() {
  content_region_.Clear();

  PangoLayout *layout = EnsureLayout();
  PangoRectangle extents;
  double x, y, w, h;

  PangoLayoutIter *iter = pango_layout_get_iter(layout);
  do {
    pango_layout_iter_get_line_extents(iter, NULL, &extents);

#if PANGO_VERSION_CHECK(1,16,0)
    pango_extents_to_pixels(&extents, NULL);
#else
    extents.x = PANGO_PIXELS_FLOOR(extents.x);
    extents.y = PANGO_PIXELS_FLOOR(extents.y);
    extents.width = PANGO_PIXELS_CEIL(extents.width);
    extents.height = PANGO_PIXELS_CEIL(extents.height);
#endif

    x = kInnerBorderX + scroll_offset_x_ + extents.x;
    y = kInnerBorderY + scroll_offset_y_ + extents.y;
    w = extents.width;
    h = extents.height;

    if (x < width_ && x + w > 0 && y < height_ && y + h > 0) {
      content_region_.AddRectangle(Rectangle(x, y, w, h));
    }
  } while(pango_layout_iter_next_line(iter));

  pango_layout_iter_free(iter);
}

void GtkEditImpl::DrawText(CanvasInterface *canvas) {
  PangoLayout *layout = EnsureLayout();

  CairoCanvas *cairo_canvas = down_cast<CairoCanvas *>(canvas);
  cairo_canvas->PushState();

  cairo_set_source_rgb(cairo_canvas->GetContext(),
                       text_color_.red,
                       text_color_.green,
                       text_color_.blue);
  cairo_move_to(cairo_canvas->GetContext(),
                scroll_offset_x_ + kInnerBorderX,
                scroll_offset_y_ + kInnerBorderY);
  pango_cairo_show_layout(cairo_canvas->GetContext(), layout);
  cairo_canvas->PopState();

  // Draw selection background.
  // Selection in a single line may be not continual, so we use pango to
  // get the x-ranges of each selection range in one line, and draw them
  // separately.
  if (!selection_region_.IsEmpty()) {
    canvas->PushState();
    selection_region_.Integerize();
    cairo_canvas->IntersectGeneralClipRegion(selection_region_);

    Color selection_color = GetSelectionBackgroundColor();
    Color text_color = GetSelectionTextColor();

    cairo_set_source_rgb(cairo_canvas->GetContext(),
                         selection_color.red,
                         selection_color.green,
                         selection_color.blue);
    cairo_paint(cairo_canvas->GetContext());

    cairo_move_to(cairo_canvas->GetContext(),
                  scroll_offset_x_ + kInnerBorderX,
                  scroll_offset_y_ + kInnerBorderY);
    cairo_set_source_rgb(cairo_canvas->GetContext(),
                         text_color.red,
                         text_color.green,
                         text_color.blue);
    pango_cairo_show_layout(cairo_canvas->GetContext(), layout);
    canvas->PopState();
  }
}

void GtkEditImpl::MoveCursor(MovementStep step, int count, bool extend_selection) {
  ResetImContext();
  int new_cursor = 0;
  // Clear selection first if not extend it.
  if (cursor_ != selection_bound_ && !extend_selection)
    SetCursor(cursor_);

  // Calculate the new offset after motion.
  switch(step) {
    case VISUALLY:
      new_cursor = MoveVisually(cursor_, count);
      break;
    case WORDS:
      new_cursor = MoveWords(cursor_, count);
      break;
    case DISPLAY_LINES:
      new_cursor = MoveDisplayLines(cursor_, count);
      break;
    case DISPLAY_LINE_ENDS:
      new_cursor = MoveLineEnds(cursor_, count);
      break;
    case PAGES:
      new_cursor = MovePages(cursor_, count);
      break;
    case BUFFER:
      ASSERT(count == -1 || count == 1);
      new_cursor = static_cast<int>(count == -1 ? 0 : text_.length());
      break;
  }

  if (extend_selection)
    SetSelectionBounds(selection_bound_, new_cursor);
  else
    SetCursor(new_cursor);
}

int GtkEditImpl::MoveVisually(int current_index, int count) {
  ASSERT(current_index >= 0 &&
         current_index <= static_cast<int>(text_.length()));
  ASSERT(count);

  PangoLayout *layout = EnsureLayout();
  const char *text = pango_layout_get_text(layout);
  int index = TextIndexToLayoutIndex(current_index, false);
  int new_index = 0;
  int new_trailing = 0;
  while (count != 0) {
    if (count > 0) {
      --count;
      pango_layout_move_cursor_visually(layout, true, index, 0, 1,
                                        &new_index, &new_trailing);
    } else if (count < 0) {
      ++count;
      pango_layout_move_cursor_visually(layout, true, index, 0, -1,
                                        &new_index, &new_trailing);
    }

    if (new_index < 0 || new_index == G_MAXINT)
      break;

    index = static_cast<int>(
        g_utf8_offset_to_pointer(text + new_index, new_trailing) - text);
  }
  return LayoutIndexToTextIndex(index);
}

int GtkEditImpl::MoveLogically(int current_index, int count) {
  ASSERT(current_index >= 0 &&
         current_index <= static_cast<int>(text_.length()));
  ASSERT(count);

  PangoLayout *layout = EnsureLayout();
  const char *text = pango_layout_get_text(layout);
  int index = TextIndexToLayoutIndex(current_index, false);

  if (visible_) {
    PangoLogAttr *log_attrs;
    gint n_attrs;
    pango_layout_get_log_attrs(layout, &log_attrs, &n_attrs);
    const char *ptr = text + index;
    const char *end = text + text_.length() + preedit_.length();
    int offset = static_cast<int>(g_utf8_pointer_to_offset(text, ptr));

    while (count > 0 && ptr < end) {
      do {
        ptr = g_utf8_find_next_char(ptr, NULL);
        ++offset;
      } while (ptr && *ptr && !log_attrs[offset].is_cursor_position);
      --count;
      if (!ptr) ptr = end;
    }
    while(count < 0 && ptr > text) {
      do {
        ptr = g_utf8_find_prev_char(text, ptr);
        --offset;
      } while (ptr && *ptr && !log_attrs[offset].is_cursor_position);
      ++count;
      if (!ptr) ptr = text;
    }
    index = static_cast<int>(ptr - text);
    g_free(log_attrs);
  } else {
    int password_char_length = static_cast<int>(password_char_.length());
    int text_len = static_cast<int>(strlen(text));
    index = Clamp(index + count * password_char_length, 0, text_len);
  }

  return LayoutIndexToTextIndex(index);
}

int GtkEditImpl::MoveWords(int current_index, int count) {
  ASSERT(current_index >= 0 &&
         current_index <= static_cast<int>(text_.length()));
  ASSERT(count);

  if (!visible_) {
    return (count > 0 ? static_cast<int>(text_.length()) : 0);
  }

  PangoLayout *layout = EnsureLayout();
  const char *text = pango_layout_get_text(layout);
  int index = TextIndexToLayoutIndex(current_index, false);

  int line_index;
  pango_layout_index_to_line_x(layout, index, FALSE, &line_index, NULL);

  // Weird bug: line_index here may be >= than line count?
  int line_count = pango_layout_get_line_count(layout);
  if (line_index >= line_count) {
    line_index = line_count - 1;
  }

#if PANGO_VERSION_CHECK(1,16,0)
  PangoLayoutLine *line = pango_layout_get_line_readonly(layout, line_index);
#else
  PangoLayoutLine *line = pango_layout_get_line(layout, line_index);
#endif
  // The cursor movement direction shall be determined by the direction of
  // current text line.
  if (line->resolved_dir == PANGO_DIRECTION_RTL) {
    count = -count;
  }

  const char *ptr = text + index;
  const char *end = text + text_.length() + preedit_.length();
  int offset = static_cast<int>(g_utf8_pointer_to_offset(text, ptr));

  PangoLogAttr *log_attrs;
  gint n_attrs;
  pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);
  while (count > 0 && ptr < end) {
    do {
      ptr = g_utf8_find_next_char(ptr, NULL);
      ++offset;
    } while (ptr && *ptr && !log_attrs[offset].is_word_start &&
             !log_attrs[offset].is_word_end &&
             !log_attrs[offset].is_sentence_boundary);
    --count;
    if (!ptr) ptr = end;
  }
  while(count < 0 && ptr > text) {
    do {
      ptr = g_utf8_find_prev_char(text, ptr);
      --offset;
    } while (ptr && *ptr && !log_attrs[offset].is_word_start &&
             !log_attrs[offset].is_word_end &&
             !log_attrs[offset].is_sentence_boundary);
    ++count;
    if (!ptr) ptr = text;
  }
  index = static_cast<int>(ptr - text);
  g_free(log_attrs);

  return LayoutIndexToTextIndex(index);
}

int GtkEditImpl::MoveDisplayLines(int current_index, int count) {
  ASSERT(current_index >= 0 &&
         current_index <= static_cast<int>(text_.length()));
  ASSERT(count);
  ASSERT(preedit_.length() == 0);

  PangoLayout *layout = EnsureLayout();
  const char *text = pango_layout_get_text(layout);
  int index = TextIndexToLayoutIndex(current_index, false);
  int n_lines = pango_layout_get_line_count(layout);
  int line_index = 0;
  int x_off = 0;
  PangoRectangle rect;

  // Find the current cursor X position in layout
  pango_layout_index_to_line_x(layout, index, FALSE, &line_index, &x_off);

  // Weird bug: line_index here may be >= than line count?
  if (line_index >= n_lines) {
    line_index = n_lines - 1;
  }

  pango_layout_get_cursor_pos(layout, index, &rect, NULL);
  x_off = rect.x;

  line_index += count;

  if (line_index < 0) {
    return 0;
  } else if (line_index >= n_lines) {
    return static_cast<int>(text_.length());
  }

#if PANGO_VERSION_CHECK(1,16,0)
  PangoLayoutLine *line = pango_layout_get_line_readonly(layout, line_index);
#else
  PangoLayoutLine *line = pango_layout_get_line(layout, line_index);
#endif

  // Find out the cursor x offset related to the new line position.
  pango_layout_index_to_pos(layout, line->start_index, &rect);

  if (line->resolved_dir == PANGO_DIRECTION_RTL) {
    PangoRectangle extents;
    pango_layout_line_get_extents(line, NULL, &extents);
    rect.x -= extents.width;
  }

  // rect.x is the left edge position of the line in the layout
  x_off -= rect.x;
  if (x_off < 0) x_off = 0;

  int trailing;
  pango_layout_line_x_to_index(line, x_off, &index, &trailing);

  index = static_cast<int>(
      g_utf8_offset_to_pointer(text + index, trailing) - text);
  return LayoutIndexToTextIndex(index);
}

int GtkEditImpl::MovePages(int current_index, int count) {
  ASSERT(current_index >= 0 &&
         current_index <= static_cast<int>(text_.length()));
  ASSERT(count);
  ASSERT(preedit_.length() == 0);

  // Transfer pages to display lines.
  PangoLayout *layout = EnsureLayout();
  int layout_height;
  pango_layout_get_pixel_size(layout, NULL, &layout_height);
  int n_lines = pango_layout_get_line_count(layout);
  int line_height = layout_height / n_lines;
  int page_lines = (height_ - kInnerBorderY * 2) / line_height;
  return MoveDisplayLines(current_index, count * page_lines);
}

int GtkEditImpl::MoveLineEnds(int current_index, int count) {
  ASSERT(current_index >= 0 &&
         current_index <= static_cast<int>(text_.length()));
  ASSERT(count);

  if (!visible_) {
    return (count > 0 ? static_cast<int>(text_.length()) : 0);
  }

  PangoLayout *layout = EnsureLayout();
  const char *text = pango_layout_get_text(layout);
  int index = TextIndexToLayoutIndex(current_index, false);
  int line_index = 0;

  // Find current line
  pango_layout_index_to_line_x(layout, index, FALSE, &line_index, NULL);

  // Weird bug: line_index here may be >= than line count?
  int line_count = pango_layout_get_line_count(layout);
  if (line_index >= line_count) {
    line_index = line_count - 1;
  }

#if PANGO_VERSION_CHECK(1,16,0)
  PangoLayoutLine *line = pango_layout_get_line_readonly(layout, line_index);
#else
  PangoLayoutLine *line = pango_layout_get_line(layout, line_index);
#endif

  if (line->length == 0)
    return current_index;

  if (line->resolved_dir == PANGO_DIRECTION_RTL) {
    count = -count;
  }

  if (count > 0) {
    const char *start = text + line->start_index;
    const char *end = start + line->length;
    const char *ptr = end;
    PangoLogAttr *log_attrs;
    gint n_attrs;
    pango_layout_get_log_attrs (layout, &log_attrs, &n_attrs);
    int offset = static_cast<int>(g_utf8_pointer_to_offset(text, ptr));

    if (line_index == line_count - 1 || *ptr == 0 ||
        log_attrs[offset].is_mandatory_break ||
        log_attrs[offset].is_sentence_boundary ||
        log_attrs[offset].is_sentence_end) {
      // Real line break.
      index = line->start_index + line->length;
    } else {
      // Line wrap,
      do {
        ptr = g_utf8_find_prev_char(start, ptr);
        --offset;
      } while(ptr && *ptr && !log_attrs[offset].is_cursor_position);
      index = static_cast<int>(ptr ? ptr - text : end - text);
    }
    g_free(log_attrs);
  } else {
    index = line->start_index;
  }

  return LayoutIndexToTextIndex(index);
}

void GtkEditImpl::SetCursor(int cursor) {
  if (cursor != cursor_) {
    ResetImContext();
    // If there was a selection range, then the selection range will be cleared.
    // Then content_modified_ shall be set to true to force redrawing the text.
    if (cursor_ != selection_bound_)
      selection_changed_ = true;
    cursor_ = cursor;
    selection_bound_ = cursor;
    cursor_moved_ = true;

    // Force recalculate the cursor position.
    cursor_index_in_layout_ = -1;
  }
}

int GtkEditImpl::XYToTextIndex(int x, int y) {
  int width, height;
  PangoLayout *layout = EnsureLayout();
  const char *text = pango_layout_get_text(layout);
  pango_layout_get_pixel_size(layout, &width, &height);

  if (y < 0) {
    return 0;
  } else if (y >= height) {
    return static_cast<int>(text_.length());
  }

  int trailing;
  int index;
  pango_layout_xy_to_index(layout, x * PANGO_SCALE, y * PANGO_SCALE,
                           &index, &trailing);
  index = static_cast<int>(
      g_utf8_offset_to_pointer(text + index, trailing) - text);

  index = LayoutIndexToTextIndex(index);

  // Adjust the offset if preedit is not empty and if the offset is after
  // current cursor.
  int preedit_length = static_cast<int>(preedit_.length());
  if (preedit_length && index > cursor_) {
    if (index >= cursor_ + preedit_length)
      index -= preedit_length;
    else
      index = cursor_;
  }
  return Clamp(index, 0, static_cast<int>(text_.length()));
}

bool GtkEditImpl::GetSelectionBounds(int *start, int *end) {
  if (start)
    *start = std::min(selection_bound_, cursor_);
  if (end)
    *end = std::max(selection_bound_, cursor_);

  return(selection_bound_ != cursor_);
}

void GtkEditImpl::SetSelectionBounds(int selection_bound, int cursor) {
  if (selection_bound_ != selection_bound || cursor_ != cursor) {
    selection_changed_ = true;
    selection_bound_ = selection_bound;
    if (cursor_ != cursor) {
      cursor_ = cursor;
      cursor_moved_ = true;

      // Force recalculate the cursor position.
      cursor_index_in_layout_ = -1;
    }
    ResetImContext();
  }
}

int GtkEditImpl::TextIndexToLayoutIndex(int text_index,
                                        bool consider_preedit_cursor) {
  if (visible_) {
    if (text_index < cursor_)
      return text_index;

    if (text_index == cursor_ && consider_preedit_cursor)
      return text_index + preedit_cursor_;

    return text_index + static_cast<int>(preedit_.length());
  }

  // Invisibile mode doesn't support preedit.
  const char *text = text_.c_str();
  int offset = static_cast<int>(
      g_utf8_pointer_to_offset(text, text + text_index));
  int password_char_length = static_cast<int>(password_char_.length());

  return offset * password_char_length;
}

int GtkEditImpl::LayoutIndexToTextIndex(int layout_index) {
  if (visible_) {
    if (layout_index < cursor_)
      return layout_index;

    int preedit_length = static_cast<int>(preedit_.length());
    if (layout_index >= cursor_ + preedit_length)
      return layout_index - preedit_length;

    return cursor_;
  }

  // Invisibile mode doesn't support preedit.
  int password_char_length = static_cast<int>(password_char_.length());
  ASSERT(layout_index % password_char_length == 0);

  int offset = layout_index / password_char_length;
  const char *text = text_.c_str();

  return static_cast<int>(g_utf8_offset_to_pointer(text, offset) - text);
}

int GtkEditImpl::GetCharLength(int index) {
  const char *text = text_.c_str();
  const char *ptr = text + index;
  const char *end = text + text_.length();
  const char *next = g_utf8_find_next_char(ptr, end);
  return static_cast<int>(next ? static_cast<int>(next - ptr) : end - ptr);
}

int GtkEditImpl::GetPrevCharLength(int index) {
  const char *text = text_.c_str();
  const char *ptr = text + index;
  const char *prev = g_utf8_find_prev_char(text, ptr);
  return static_cast<int>(prev ? static_cast<int>(ptr - prev) : ptr - text);
}

void GtkEditImpl::EnterText(const char *str) {
  if (readonly_ || !str || !*str) return;

  if (GetSelectionBounds(NULL, NULL)) {
    DeleteSelection();
  } else if (overwrite_ && cursor_ != static_cast<int>(text_.length())) {
    DeleteText(cursor_, MoveLogically(cursor_, 1));
  }

  std::string tmp_text;
  if (!multiline_) {
    tmp_text = CleanupLineBreaks(str);
    str = tmp_text.c_str();
  }

  const char *end = NULL;
  g_utf8_validate(str, -1, &end);
  if (end > str) {
    size_t len = end - str;
    text_.insert(cursor_, str, len);
    cursor_ += static_cast<int>(len);
    selection_bound_ += static_cast<int>(len);
  }

  ResetLayout();
  owner_->FireOnChangeEvent();
}

void GtkEditImpl::DeleteText(int start, int end) {
  if (readonly_) return;

  int text_length = static_cast<int>(text_.length());
  if (start < 0)
    start = 0;
  else if (start > text_length)
    start = text_length;

  if (end < 0)
    end = 0;
  else if (end > text_length)
    end = text_length;

  if (start > end)
    std::swap(start, end);
  else if (start == end)
    return;

  text_.erase(start, end - start);

  if (cursor_ >= end)
    cursor_ -= (end - start);
  if (selection_bound_ >= end)
    selection_bound_ -= (end - start);

  ResetLayout();
  owner_->FireOnChangeEvent();
}

void GtkEditImpl::SelectWord() {
  int selection_bound = MoveWords(cursor_, -1);
  int cursor = MoveWords(selection_bound, 1);
  SetSelectionBounds(selection_bound, cursor);
}

void GtkEditImpl::SelectLine() {
  int selection_bound = MoveLineEnds(cursor_, -1);
  int cursor = MoveLineEnds(selection_bound, 1);
  SetSelectionBounds(selection_bound, cursor);
}

void GtkEditImpl::Select(int start, int end) {
  int text_length = static_cast<int>(text_.length());
  if (start == -1)
    start = text_length;
  if (end == -1)
    end = text_length;

  start = Clamp(start, 0, text_length);
  end = Clamp(end, 0, text_length);
  SetSelectionBounds(start, end);
  QueueRefresh(false, MINIMAL_ADJUST);
}

void GtkEditImpl::SelectAll() {
  SetSelectionBounds(0, static_cast<int>(text_.length()));
  QueueRefresh(false, MINIMAL_ADJUST);
}

CanvasInterface::Alignment GtkEditImpl::GetAlign() const {
  return align_;
}

void GtkEditImpl::SetAlign(CanvasInterface::Alignment align) {
  align_ = align;
  QueueRefresh(true, CENTER_CURSOR);
}

CanvasInterface::VAlignment GtkEditImpl::GetVAlign() const {
  return valign_;
}

void GtkEditImpl::SetVAlign(CanvasInterface::VAlignment valign) {
  valign_ = valign;
  QueueRefresh(true, CENTER_CURSOR);
}

void GtkEditImpl::DeleteSelection() {
  int start, end;
  if (GetSelectionBounds(&start, &end))
    DeleteText(start, end);
}

void GtkEditImpl::CopyClipboard() {
  int start, end;
  if (GetSelectionBounds(&start, &end)) {
    GtkWidget *widget = GetWidgetAndCursorLocation(NULL);
    if (widget) {
      if (visible_) {
        gtk_clipboard_set_text(
            gtk_widget_get_clipboard(widget, GDK_SELECTION_CLIPBOARD),
            text_.c_str() + start, end - start);
      } else {
        // Don't copy real content if it's in invisible.
        std::string content;
        int nchars = static_cast<int>(
            g_utf8_strlen(text_.c_str() + start, end - start));
        for (int i = 0; i < nchars; ++i)
          content.append(password_char_);
        gtk_clipboard_set_text(
            gtk_widget_get_clipboard(widget, GDK_SELECTION_CLIPBOARD),
            content.c_str(), static_cast<int>(content.length()));
      }
    }
  }
}

void GtkEditImpl::CutClipboard() {
  CopyClipboard();
  DeleteSelection();
}

void GtkEditImpl::PasteClipboard() {
  GtkWidget *widget = GetWidgetAndCursorLocation(NULL);
  if (widget) {
    gtk_clipboard_request_text(
        gtk_widget_get_clipboard(widget, GDK_SELECTION_CLIPBOARD),
        PasteCallback, this);
  }
}

void GtkEditImpl::BackSpace() {
  if (GetSelectionBounds(NULL, NULL)) {
    DeleteSelection();
  } else {
    if (cursor_ == 0) return;
    DeleteText(MoveLogically(cursor_, -1), cursor_);
  }
}

void GtkEditImpl::Delete() {
  if (GetSelectionBounds(NULL, NULL)) {
    DeleteSelection();
  } else {
    if (cursor_ == static_cast<int>(text_.length())) return;
    DeleteText(cursor_, MoveLogically(cursor_, 1));
  }
}

void GtkEditImpl::ToggleOverwrite() {
  overwrite_ = !overwrite_;

  // Force recalculate the cursor position.
  cursor_index_in_layout_ = -1;
  cursor_moved_ = true;
  QueueRefresh(false, NO_SCROLL);
}

Color GtkEditImpl::GetSelectionBackgroundColor() {
  GtkWidget *widget = GetWidgetAndCursorLocation(NULL);
  if (widget) {
    GtkStyle *style = gtk_widget_get_style(widget);
    if (style) {
      GdkColor *color =
          &style->base[focused_ ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE];
      return Color(static_cast<double>(color->red) / 65535.0,
                   static_cast<double>(color->green) / 65535.0,
                   static_cast<double>(color->blue) / 65535.0);
    }
  }
  return kDefaultSelectionBackgroundColor;
}

Color GtkEditImpl::GetSelectionTextColor() {
  GtkWidget *widget = GetWidgetAndCursorLocation(NULL);
  if (widget) {
    GtkStyle *style = gtk_widget_get_style(widget);
    if (style) {
      GdkColor *color =
          &style->text[focused_ ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE];
      return Color(static_cast<double>(color->red) / 65535.0,
                   static_cast<double>(color->green) / 65535.0,
                   static_cast<double>(color->blue) / 65535.0);
    }
  }
  return kDefaultSelectionTextColor;
}

GtkWidget *GtkEditImpl::GetWidgetAndCursorLocation(GdkRectangle *cur) {
  GtkWidget *widget = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
  if (widget && cur) {
    PangoRectangle strong;
    int display_width = width_ - kInnerBorderX * 2;
    int display_height = height_ - kInnerBorderY * 2;
    GetCursorLocationInLayout(&strong, NULL);
    strong.x = Clamp(strong.x + scroll_offset_x_, 0, display_width);
    strong.y = Clamp(strong.y + scroll_offset_y_, 0, display_height);
    double x, y, height;
    owner_->GetView()->ViewCoordToNativeWidgetCoord(0, strong.height,
                                                    &x, &height);
    owner_->SelfCoordToViewCoord(strong.x, strong.y, &x, &y);
    owner_->GetView()->ViewCoordToNativeWidgetCoord(x, y, &x, &y);
    cur->x = int(x);
    cur->y = int(y);
    cur->width = 0;
    cur->height = int(ceil(height));
  }
  return widget;
}

void GtkEditImpl::GetCursorLocationInLayout(PangoRectangle *strong,
                                            PangoRectangle *weak) {
  if (cursor_index_in_layout_ < 0) {
    // Recalculate cursor position.
    PangoLayout *layout = EnsureLayout();
    int index = TextIndexToLayoutIndex(cursor_, true);
    cursor_index_in_layout_ = index;

    pango_layout_get_cursor_pos(layout, index, &strong_cursor_pos_,
                                &weak_cursor_pos_);
    strong_cursor_pos_.width = PANGO_SCALE;
    weak_cursor_pos_.width = PANGO_SCALE;

    if (overwrite_) {
      PangoRectangle pos;
      pango_layout_index_to_pos(layout, index, &pos);
      if (pos.width != 0) {
        if (pos.width < 0) {
          pos.x += pos.width;
          pos.width = -pos.width;
        }
        strong_cursor_pos_ = pos;
      }
      weak_cursor_pos_ = strong_cursor_pos_;
    }
  }

  if (strong) {
    strong->x = PANGO_PIXELS(strong_cursor_pos_.x);
    strong->y = PANGO_PIXELS(strong_cursor_pos_.y);
    strong->width = PANGO_PIXELS(strong_cursor_pos_.width);
    strong->height = PANGO_PIXELS(strong_cursor_pos_.height);
  }
  if (weak) {
    weak->x = PANGO_PIXELS(weak_cursor_pos_.x);
    weak->y = PANGO_PIXELS(weak_cursor_pos_.y);
    weak->width = PANGO_PIXELS(weak_cursor_pos_.width);
    weak->height = PANGO_PIXELS(weak_cursor_pos_.height);
  }
}

void GtkEditImpl::UpdateIMCursorLocation() {
  if (im_context_) {
    GdkRectangle cur;
    GtkWidget *widget = GetWidgetAndCursorLocation(&cur);
    if (widget && widget->window) {
      gtk_im_context_set_client_window(im_context_, widget->window);
      gtk_im_context_set_cursor_location(im_context_, &cur);
      DLOG("Update IM cursor location: x=%d, y=%d, h=%d",
           cur.x, cur.y, cur.height);
    }
  }
}

void GtkEditImpl::CommitCallback(GtkIMContext *context, const char *str,
                                 void *gg) {
  GGL_UNUSED(context);
  reinterpret_cast<GtkEditImpl*>(gg)->EnterText(str);
  reinterpret_cast<GtkEditImpl*>(gg)->QueueRefresh(false, MINIMAL_ADJUST);
}

gboolean GtkEditImpl::RetrieveSurroundingCallback(GtkIMContext *context,
                                                  void *gg) {
  GtkEditImpl *edit = reinterpret_cast<GtkEditImpl*>(gg);
  gtk_im_context_set_surrounding(context, edit->text_.c_str(),
                                 static_cast<int>(edit->text_.length()),
                                 static_cast<int>(edit->cursor_));
  return TRUE;
}

gboolean GtkEditImpl::DeleteSurroundingCallback(GtkIMContext *context, int offset,
                                            int n_chars, void *gg) {
  GGL_UNUSED(context);
  GtkEditImpl *edit = reinterpret_cast<GtkEditImpl*>(gg);
  const char *text = edit->text_.c_str();
  int text_length = static_cast<int>(edit->text_.length());
  int cursor = edit->cursor_;

  const char *start_ptr = g_utf8_offset_to_pointer(text + cursor, offset);
  if (start_ptr < text) start_ptr = text;
  int start_index = static_cast<int>(start_ptr - text);

  if (start_index < text_length) {
    const char *end_ptr = g_utf8_offset_to_pointer(text + start_index, n_chars);
    if (end_ptr < text) end_ptr = text;
    int end_index = static_cast<int>(end_ptr - text);
    edit->DeleteText(start_index, end_index);
    edit->QueueRefresh(false, CENTER_CURSOR);
  }
  return TRUE;
}

void GtkEditImpl::PreeditStartCallback(GtkIMContext *context, void *gg) {
  GGL_UNUSED(context);
  reinterpret_cast<GtkEditImpl*>(gg)->ResetPreedit();
  reinterpret_cast<GtkEditImpl*>(gg)->QueueRefresh(false, MINIMAL_ADJUST);
  reinterpret_cast<GtkEditImpl*>(gg)->UpdateIMCursorLocation();
}

void GtkEditImpl::PreeditChangedCallback(GtkIMContext *context, void *gg) {
  GtkEditImpl *edit = reinterpret_cast<GtkEditImpl*>(gg);
  char *str;
  edit->ResetPreedit();
  int cursor_pos;
  gtk_im_context_get_preedit_string(context, &str,
                                    &edit->preedit_attrs_,
                                    &cursor_pos);
  edit->preedit_cursor_ =
      static_cast<int>(g_utf8_offset_to_pointer(str, cursor_pos) - str);
  edit->preedit_.assign(str);
  g_free(str);
  edit->QueueRefresh(true, MINIMAL_ADJUST);
  edit->need_im_reset_ = true;
  edit->content_modified_ = true;

  // Force recalculate the cursor position.
  edit->cursor_index_in_layout_ = -1;
}

void GtkEditImpl::PreeditEndCallback(GtkIMContext *context, void *gg) {
  GGL_UNUSED(context);
  reinterpret_cast<GtkEditImpl*>(gg)->ResetPreedit();
  reinterpret_cast<GtkEditImpl*>(gg)->QueueRefresh(false, MINIMAL_ADJUST);
}

void GtkEditImpl::PasteCallback(GtkClipboard *clipboard,
                            const gchar *str, void *gg) {
  GGL_UNUSED(clipboard);
  reinterpret_cast<GtkEditImpl*>(gg)->EnterText(str);
  reinterpret_cast<GtkEditImpl*>(gg)->QueueRefresh(false, MINIMAL_ADJUST);
}

} // namespace gtk
} // namespace ggadget
