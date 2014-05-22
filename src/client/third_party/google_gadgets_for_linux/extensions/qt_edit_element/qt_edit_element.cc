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
#include <QtGui/QPainter>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QTextLine>
#include <QtGui/QTextDocumentFragment>
#include <QtGui/QTextBlock>
#include <ggadget/canvas_interface.h>
#include <ggadget/event.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/scrolling_element.h>
#include <ggadget/slot.h>
#include <ggadget/texture.h>
#include <ggadget/view.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/element_factory.h>
#include <ggadget/qt/qt_canvas.h>
#include "qt_edit_element.h"

#define Initialize qt_edit_element_LTX_Initialize
#define Finalize qt_edit_element_LTX_Finalize
#define RegisterElementExtension qt_edit_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize qt_edit_element extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize qt_edit_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOGI("Register qt_edit_element extension.");
    if (factory) {
      factory->RegisterElementClass(
          "edit", &ggadget::qt::QtEditElement::CreateInstance);
    }
    return true;
  }
}

namespace ggadget {
namespace qt {

static const int kDefaultEditElementWidth = 60;
static const int kDefaultEditElementHeight = 16;

static const int kInnerBorderX = 2;
static const int kInnerBorderY = 1;

static const Color kDefaultTextColor(0, 0, 0);
static const Color kDefaultBackgroundColor(1, 1, 1);
static const Color kDefaultSelectionBackgroundColor(0.5, 0.5, 0.5);
static const Color kDefaultSelectionTextColor(1, 1, 1);

static void SetCursorSelection(QTextCursor *cur, int start, int end) {
  cur->setPosition(start);
  cur->setPosition(end, QTextCursor::KeepAnchor);
}

QtEditElement::QtEditElement(View *view, const char *name)
    : EditElementBase(view, name),
      cursor_(NULL),
      multiline_(false),
      bold_(false),
      italic_(false),
      strikeout_(false),
      underline_(false),
      overwrite_(false),
      wrap_(false),
      readonly_(false),
      focused_(false),
      page_line_(0),
      width_(kDefaultEditElementWidth),
      height_(kDefaultEditElementHeight),
      scroll_offset_x_(0),
      scroll_offset_y_(0),
      background_(NULL) {
  ConnectOnScrolledEvent(NewSlot(this, &QtEditElement::OnScrolled));
  cursor_ = new QTextCursor(&doc_);
  SetFont(kDefaultFontName);
}

QtEditElement::~QtEditElement() {
  delete cursor_;
  delete background_;
}

void QtEditElement::GetScrollBarInfo(int *x_range, int *y_range,
                                     int *line_step, int *page_step,
                                     int *cur_pos) {
  SetWidth(static_cast<int>(ceil(GetClientWidth())));
  SetHeight(static_cast<int>(ceil(GetClientHeight())));

  if (RequestHeight() > height_ && multiline_) {
    *y_range = RequestHeight() - height_;
    *x_range = 0;
    *line_step = 10;
    *page_step = height_;
    *cur_pos = scroll_offset_y_;
  } else {
    *y_range = 0;
    *x_range = 0;
    *line_step = 0;
    *page_step = 0;
    *cur_pos = 0;
  }
}

void QtEditElement::Layout() {
  static int recurse_depth = 0;
  EditElementBase::Layout();

  int x_range, y_range, line_step, page_step, cur_pos;
  GetScrollBarInfo(&x_range, &y_range, &line_step, &page_step, &cur_pos);
  SetScrollYPosition(cur_pos);
  SetYLineStep(line_step);
  SetYPageStep(page_step);

  // See DivElement::Layout() impl for the reason of recurse_depth.
  if (UpdateScrollBar(x_range, y_range) && (y_range > 0 || recurse_depth < 2)) {
    recurse_depth++;
    // If the scrollbar display state was changed, then call Layout()
    // recursively to redo Layout.
    Layout();
    recurse_depth--;
  }
}

void QtEditElement::MarkRedraw() {
  EditElementBase::MarkRedraw();
}

Variant QtEditElement::GetBackground() const {
  return Variant(Texture::GetSrc(background_));
}

void QtEditElement::SetBackground(const Variant &background) {
  delete background_;
  background_ = GetView()->LoadTexture(background);
}

bool QtEditElement::IsBold() const {
  return bold_;
}

void QtEditElement::SetBold(bool bold) {
  if (bold_ != bold) {
    bold_ = bold;
    QFont f = doc_.defaultFont();
    f.setBold(bold);
    doc_.setDefaultFont(f);
    QueueDraw();
  }
}

std::string QtEditElement::GetColor() const {
  return text_color_.ToString();
}

void QtEditElement::SetColor(const char *color) {
  Color::FromString(color, &text_color_, NULL);
  QColor c(text_color_.RedInt(),
           text_color_.GreenInt(),
           text_color_.BlueInt());
  paint_ctx_.palette.setBrush(QPalette::Text, c);
  QueueDraw();
}

std::string QtEditElement::GetFont() const {
  return doc_.defaultFont().family().toUtf8().data();
}

void QtEditElement::SetFont(const char *font) {
  if (AssignIfDiffer(font, &font_family_)) {
    QFont qfont(font_family_.empty()
                ? kDefaultFontName
                : QString::fromUtf8(font));
    double s = GetCurrentSize();
    if (s > 0) qfont.setPointSizeF(s);
    doc_.setDefaultFont(qfont);
    QueueDraw();
  }
}

bool QtEditElement::IsItalic() const {
  return italic_;
}

void QtEditElement::SetItalic(bool italic) {
  if (italic_ != italic) {
    italic_ = italic;
    QFont f = doc_.defaultFont();
    f.setItalic(italic);
    doc_.setDefaultFont(f);
    QueueDraw();
  }
}

bool QtEditElement::IsMultiline() const {
  return multiline_;
}

void QtEditElement::SetMultiline(bool multiline) {
  if (multiline_ != multiline) {
    multiline_ = multiline;
    if (!multiline_)
      SetValue(GetValue().c_str());
    QueueDraw();
  }
}

std::string QtEditElement::GetPasswordChar() const {
  return GetValue();
}

void QtEditElement::SetPasswordChar(const char *c) {
  if (c == NULL || *c == 0 || !IsLegalUTF8Char(c, GetUTF8CharLength(c))) {
    password_char_ = "*";
  } else {
    password_char_ = QString::fromUtf8(c);
  }
}

void QtEditElement::OnFontSizeChange() {
  QFont font = doc_.defaultFont();
  double s = GetCurrentSize();
  if (s > 0) font.setPointSizeF(s);
  doc_.setDefaultFont(font);
}

bool QtEditElement::IsStrikeout() const {
  return strikeout_;
}

void QtEditElement::SetStrikeout(bool strikeout) {
  if (strikeout_ != strikeout) {
    strikeout_ = strikeout;
    QFont f = doc_.defaultFont();
    f.setStrikeOut(strikeout);
    doc_.setDefaultFont(f);
    QueueDraw();
  }
}

bool QtEditElement::IsUnderline() const {
  return underline_;
}

void QtEditElement::SetUnderline(bool underline) {
  if (underline_ != underline) {
    underline_ = underline;
    QFont f = doc_.defaultFont();
    f.setUnderline(underline);
    doc_.setDefaultFont(f);
    QueueDraw();
  }
}

std::string QtEditElement::GetValue() const {
  return doc_.toPlainText().toUtf8().data();
}

void QtEditElement::SetValue(const char *value) {
  QString qval;
  if (!multiline_) {
    std::string v = CleanupLineBreaks(value);
    qval = QString::fromUtf8(v.c_str());
  } else {
    qval = QString::fromUtf8(value);
  }

  if (qval != doc_.toPlainText()) {
    doc_.setPlainText(qval);
    QueueDraw();
    FireOnChangeEvent();
  }
}

bool QtEditElement::IsWordWrap() const {
  return wrap_;
}

void QtEditElement::SetWordWrap(bool wrap) {
  if (wrap_ != wrap) {
    wrap_ = wrap;
    QTextOption opt = doc_.defaultTextOption();
    if (wrap)
      opt.setWrapMode(QTextOption::WordWrap);
    else
      opt.setWrapMode(QTextOption::NoWrap);
    doc_.setDefaultTextOption(opt);
    QueueDraw();
  }
}

bool QtEditElement::IsReadOnly() const {
  return readonly_;
}

void QtEditElement::SetReadOnly(bool readonly) {
  if (readonly != readonly_) {
    readonly_ = readonly;
    QueueDraw();
  }
}

bool QtEditElement::IsDetectUrls() const {
  // TODO
  return false;
}

void QtEditElement::SetDetectUrls(bool /*detect_urls*/) {
  // TODO
}

void QtEditElement::GetIdealBoundingRect(int *width, int *height) {
  const QSize s = doc_.documentLayout()->documentSize().toSize();

  if (width) *width = s.width();
  if (height) *height = s.height();
}

void QtEditElement::Select(int start, int end) {
  SetCursorSelection(cursor_, start, end);
}

void QtEditElement::SelectAll() {
  cursor_->setPosition(0);
  cursor_->movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
}

CanvasInterface::Alignment QtEditElement::GetAlign() const {
  // TODO
  return CanvasInterface::ALIGN_LEFT;
}

void QtEditElement::SetAlign(CanvasInterface::Alignment) {
  // TODO
}

CanvasInterface::VAlignment QtEditElement::GetVAlign() const {
  // TODO
  return CanvasInterface::VALIGN_TOP;
}

void QtEditElement::SetVAlign(CanvasInterface::VAlignment) {
  // TODO
}

static QRectF GetRectForPosition(QTextDocument *doc, int position) {
  const QTextBlock block = doc->findBlock(position);
  if (!block.isValid()) return QRectF();
  const QAbstractTextDocumentLayout *docLayout = doc->documentLayout();
  const QTextLayout *layout = block.layout();
  const QPointF layoutPos = docLayout->blockBoundingRect(block).topLeft();
  int relativePos = position - block.position();
/*  if (preeditCursor != 0) {
    int preeditPos = layout->preeditAreaPosition();
    if (relativePos == preeditPos)
      relativePos += preeditCursor;
    else if (relativePos > preeditPos)
      relativePos += layout->preeditAreaText().length();
  }*/
  QTextLine line = layout->lineForTextPosition(relativePos);

  int cursorWidth;
  {
    bool ok = false;
    cursorWidth = docLayout->property("cursorWidth").toInt(&ok);
    if (!ok)
      cursorWidth = 1;
  }

  QRectF r;

  if (line.isValid())
    r = QRectF(layoutPos.x() + line.cursorToX(relativePos) - 5 - cursorWidth, layoutPos.y() + line.y(),
        2 * cursorWidth + 10, line.ascent() + line.descent()+1.);
  else
    r = QRectF(layoutPos.x() - 5 - cursorWidth, layoutPos.y(), 2 * cursorWidth + 10, 10); // #### correct height

  return r;
}

void QtEditElement::DoDraw(CanvasInterface *canvas) {
  canvas->PushState();
  QtCanvas *c = down_cast<QtCanvas*>(canvas);
  if (background_) {
    background_->Draw(canvas, 0, 0, width_, height_);
  } else {
    canvas->DrawFilledRect(0, 0, width_, height_, kDefaultBackgroundColor);
  }
  QPainter *p = c->GetQPainter();

  QTextDocument *doc = &doc_;
  QTextCursor *cursor = cursor_;

  // These document and cursor are used to draw when password_ is set
  QTextDocument tmp_doc;
  QTextCursor tmp_cursor(&tmp_doc);
  if (!password_char_.isEmpty()) {
    QString shadow;
    size_t len = GetValue().length();
    for (size_t i = 0; i < len; i++)
      shadow.append(password_char_);
    tmp_doc.setPlainText(shadow);

    // Setup cursor's position and selection
    int start = cursor_->selectionStart();
    int end = cursor_->selectionEnd();
    int pos = cursor_->position();
    tmp_cursor.setPosition(pos);
    if (end > start) {
      if (pos == start) {
        tmp_cursor.movePosition(QTextCursor::NextCharacter,
                                QTextCursor::KeepAnchor,
                                end - start);
      } else {
        tmp_cursor.movePosition(QTextCursor::PreviousCharacter,
                                QTextCursor::KeepAnchor,
                                end - start);
      }
    }

    doc = &tmp_doc;
    cursor = &tmp_cursor;
    DLOG("passwd is: %s", GetValue().c_str());
    DLOG("Selection is from %d to %d", start, end);
  }

  QAbstractTextDocumentLayout::Selection selection;
  selection.cursor = *cursor;
  selection.format.setForeground(QColor(0xff, 0xff, 0xff));
  selection.format.setBackground(QColor(0x00, 0x00, 0xff));
  paint_ctx_.selections.clear();
  paint_ctx_.selections.append(selection);

  QRectF rect(0, scroll_offset_y_, c->GetWidth(),
              c->GetHeight());
  paint_ctx_.clip = rect;

  canvas->TranslateCoordinates(0, -scroll_offset_y_);
  doc->documentLayout()->draw(p, paint_ctx_);
  paint_ctx_.selections.clear();

  // Draw the cursor
  if (focused_) {
    QRectF r = GetRectForPosition(doc, cursor->position());
    double x = (r.left() + r.right())/2;
    c->DrawLine(x, r.top(), x, r.bottom(), 1, Color::kBlack);
  }
  canvas->PopState();
  DrawScrollbar(canvas);
}

EventResult QtEditElement::HandleMouseEvent(const MouseEvent &event) {
  if (EditElementBase::HandleMouseEvent(event) == EVENT_RESULT_HANDLED)
    return EVENT_RESULT_HANDLED;
  if (event.GetButton() != MouseEvent::BUTTON_LEFT)
    return EVENT_RESULT_UNHANDLED;

  Event::Type type = event.GetType();
  double x = event.GetX() - kInnerBorderX - scroll_offset_x_;
  double y = event.GetY() - kInnerBorderY - scroll_offset_y_;
  int offset = doc_.documentLayout()->hitTest(QPointF(x, y), Qt::FuzzyHit);
  int sel_start = cursor_->selectionStart();
  int sel_end = cursor_->selectionEnd();

  if (type == Event::EVENT_MOUSE_DOWN) {
    if (event.GetModifier() & Event::MODIFIER_SHIFT) {
      // If current click position is inside the selection range, then just
      // cancel the selection.
      if (offset > sel_start && offset < sel_end)
        cursor_->setPosition(offset);
      else if (offset <= sel_start)
        SetCursorSelection(cursor_, sel_end, offset);
      else if (offset >= sel_end)
        SetCursorSelection(cursor_, sel_start, offset);
    } else {
      cursor_->setPosition(offset);
    }
  } else if (type == Event::EVENT_MOUSE_DBLCLICK) {
    if (event.GetModifier() & Event::MODIFIER_SHIFT)
      cursor_->select(QTextCursor::LineUnderCursor);
    else
      cursor_->select(QTextCursor::WordUnderCursor);
  } else if (type == Event::EVENT_MOUSE_MOVE) {
    cursor_->setPosition(offset, QTextCursor::KeepAnchor);
  }

  QueueDraw();
  return EVENT_RESULT_HANDLED;
}

EventResult QtEditElement::HandleInputMethodEvent(QInputMethodEvent *e) {
  if (readonly_) return EVENT_RESULT_UNHANDLED;

  cursor_->removeSelectedText();

  // insert commit string
  if (!e->commitString().isEmpty() || e->replacementLength()) {
    QTextCursor c = *cursor_;
    c.setPosition(c.position() + e->replacementStart());
    c.setPosition(c.position() + e->replacementLength(), QTextCursor::KeepAnchor);
    c.insertText(e->commitString());
    ScrollToCursor();
    FireOnChangeEvent();
    QueueDraw();
  }

  return EVENT_RESULT_HANDLED;
}

EventResult QtEditElement::HandleKeyEvent(const KeyboardEvent &event) {
  QEvent *qevent = static_cast<QEvent*>(event.GetOriginalEvent());
  if (qevent->type() == QEvent::InputMethod) {
    return HandleInputMethodEvent(static_cast<QInputMethodEvent*>(qevent));
  }
  Event::Type type = event.GetType();
  if (type == Event::EVENT_KEY_UP)
    return EVENT_RESULT_UNHANDLED;

  QKeyEvent *key_event = static_cast<QKeyEvent*>(qevent);
  int mod = event.GetModifier();
  bool shift = (mod & Event::MODIFIER_SHIFT);
  bool ctrl = (mod & Event::MODIFIER_CONTROL);
  int keyval = key_event->key();

  if (type == Event::EVENT_KEY_DOWN) {
    if (keyval == Qt::Key_Left) {
      if (!ctrl) MoveCursor(QTextCursor::Left, 1, shift);
      else MoveCursor(QTextCursor::WordLeft, 1, shift);
    } else if (keyval == Qt::Key_Right) {
      if (!ctrl) MoveCursor(QTextCursor::Right, 1, shift);
      else MoveCursor(QTextCursor::WordRight, 1, shift);
    } else if (keyval == Qt::Key_Up) {
      MoveCursor(QTextCursor::Up, 1, shift);
    } else if (keyval == Qt::Key_Down) {
      MoveCursor(QTextCursor::Down, 1, shift);
    } else if (keyval == Qt::Key_Home) {
      if (!ctrl) MoveCursor(QTextCursor::StartOfLine, 1, shift);
      else MoveCursor(QTextCursor::Start, 1, shift);
    } else if (keyval == Qt::Key_End) {
      if (!ctrl) MoveCursor(QTextCursor::EndOfLine, 1, shift);
      else MoveCursor(QTextCursor::End, 1, shift);
    } else if (keyval == Qt::Key_PageUp) {
      if (!ctrl) MoveCursor(QTextCursor::Up, page_line_, shift);
    } else if (keyval == Qt::Key_PageDown) {
      if (!ctrl) MoveCursor(QTextCursor::Down, page_line_, shift);
    } else if ((keyval == Qt::Key_X && ctrl && !shift) ||
               (keyval == Qt::Key_Delete && shift && !ctrl)) {
      CutClipboard();
    } else if ((keyval == Qt::Key_C && ctrl && !shift) ||
               (keyval == Qt::Key_Insert && ctrl && !shift)) {
      CopyClipboard();
    } else if ((keyval == Qt::Key_V && ctrl && !shift) ||
               (keyval == Qt::Key_Insert && shift && !ctrl)) {
      PasteClipboard();
    } else if (keyval == Qt::Key_A && ctrl) {
      SelectAll();
    } else if (keyval == Qt::Key_Backspace) {
      cursor_->deletePreviousChar();
      ScrollToCursor();
      FireOnChangeEvent();
    } else if (keyval == Qt::Key_Delete && !shift) {
      cursor_->deleteChar();
      ScrollToCursor();
      FireOnChangeEvent();
    } else if (keyval == Qt::Key_Insert && !shift && !ctrl) {
      overwrite_ = !overwrite_;
    } else if (!key_event->text().isEmpty()
               && keyval != Qt::Key_Escape
               && keyval != Qt::Key_Return
               && keyval != Qt::Key_Tab) {
      EnterText(key_event->text());
    } else {
      return EVENT_RESULT_UNHANDLED;
    }
  } else { // EVENT_KEY_PRESS
    if (keyval == Qt::Key_Return) {
      // If multiline_ is unset, just ignore new_line.
      if (multiline_)
        EnterText("\n");
      else
        return EVENT_RESULT_UNHANDLED;
    } else {
      return EVENT_RESULT_UNHANDLED;
    }
  }
  QueueDraw();
  return EVENT_RESULT_HANDLED;
}

void QtEditElement::ScrollToCursor() {
  if (!multiline_) return;
  QRectF r = GetRectForPosition(&doc_, cursor_->position());
  if (r.top() < scroll_offset_y_) {
    scroll_offset_y_ = static_cast<int>(r.top());
  } else if (r.bottom() > scroll_offset_y_ + RealHeight()) {
    scroll_offset_y_ = static_cast<int>(r.bottom() - RealHeight());
  }
}

void QtEditElement::EnterText(QString str) {
  if (readonly_) return;

  if (cursor_->hasSelection() || overwrite_) {
    cursor_->deleteChar();
  }

  cursor_->insertText(str);

  // Scroll to the position of cursor_ if necessary
  ScrollToCursor();

  FireOnChangeEvent();
}

void QtEditElement::SetWidth(int width) {
  if (width_ != width) {
    width_ = width;
    if (width_ <= kInnerBorderX * 2)
      width_ = kInnerBorderX * 2 + 1;
  }
}

void QtEditElement::SetHeight(int height) {
  if (height_ != height) {
    height_ = height;
    if (height_ <= kInnerBorderY * 2)
      height_ = kInnerBorderY * 2 + 1;
  }
}

EventResult QtEditElement::HandleOtherEvent(const Event &event) {
  if (event.GetType() == Event::EVENT_FOCUS_IN) {
    FocusIn();
    return EVENT_RESULT_HANDLED;
  } else if(event.GetType() == Event::EVENT_FOCUS_OUT) {
    FocusOut();
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void QtEditElement::GetDefaultSize(double *width, double *height) const {
  ASSERT(width && height);

  *width = kDefaultEditElementWidth;
  *height = kDefaultEditElementHeight;
}

void QtEditElement::OnScrolled() {
  DLOG("QtEditElement::OnScrolled(%d)", GetScrollYPosition());
  int position = GetScrollYPosition();

  if ( RequestHeight() > RealHeight()) {
    if (position < 0)
      position = 0;
    else if (position >= RequestHeight() - RealHeight())
      position = RequestHeight() - RealHeight() - 1;

    scroll_offset_y_ = position;
    QueueDraw();
  }
}

BasicElement *QtEditElement::CreateInstance(View *view, const char *name) {
  return new QtEditElement(view, name);
}

void QtEditElement::MoveCursor(QTextCursor::MoveOperation op, int count,
                               bool extend_selection) {
  QTextCursor::MoveMode mode =
      extend_selection ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;
  cursor_->movePosition(op, mode, count);
  ScrollToCursor();
}

void QtEditElement::FocusIn() {
  if (!focused_) {
    focused_ = true;
    QueueDraw();
  }
}

void QtEditElement::FocusOut() {
  if (focused_) {
    focused_ = false;
    QueueDraw();
  }
}

void QtEditElement::PasteClipboard() {
  QClipboard *clipboard = QApplication::clipboard();
  if (!multiline_) {
    std::string content = clipboard->text().toUtf8().data();
    content = CleanupLineBreaks(content.c_str());
    EnterText(QString::fromUtf8(content.c_str()));
  } else {
    EnterText(clipboard->text());
  }
}

void QtEditElement::CopyClipboard() {
  if (cursor_->hasSelection() && password_char_.isEmpty()) {
    const QTextDocumentFragment fragment(*cursor_);
    QApplication::clipboard()->setText(fragment.toPlainText());
  }
}

void QtEditElement::CutClipboard() {
  if (readonly_ || !password_char_.isEmpty() || !cursor_->hasSelection())
    return;
  CopyClipboard();
  cursor_->deleteChar();
  FireOnChangeEvent();
}

int QtEditElement::RealHeight() {
  return height_ - kInnerBorderY * 2;
}

int QtEditElement::RealWidth() {
  return width_ - kInnerBorderX * 2;
}

int QtEditElement::RequestHeight() {
  return static_cast<int>(ceil(doc_.documentLayout()->documentSize().height()));
}

int QtEditElement::RequestWidth() {
  return static_cast<int>(ceil(doc_.documentLayout()->documentSize().width()));
}

} // namespace qt
} // namespace ggadget
