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

#ifndef GGADGET_QT_EDIT_ELEMENT_H__
#define GGADGET_QT_EDIT_ELEMENT_H__

#include <string>
#include <QtCore/QString>
#include <QtGui/QTextDocument>
#include <QtGui/QTextCursor>
#include <QtGui/QInputMethodEvent>
#include <ggadget/canvas_interface.h>
#include <ggadget/basic_element.h>
#include <ggadget/edit_element_base.h>

namespace ggadget {
namespace qt {

class QtEditElement : public EditElementBase {
 public:
  DEFINE_CLASS_ID(0xea8a5426cb544282, EditElementBase);

  QtEditElement(View *view, const char *name);
  virtual ~QtEditElement();

  virtual void Layout();
  virtual void MarkRedraw();

 public:
  virtual Variant GetBackground() const;
  virtual void SetBackground(const Variant &background);
  virtual bool IsBold() const;
  virtual void SetBold(bool bold);
  virtual std::string GetColor() const;
  virtual void SetColor(const char *color);
  virtual std::string GetFont() const;
  virtual void SetFont(const char *font);
  virtual bool IsItalic() const;
  virtual void SetItalic(bool italic);
  virtual bool IsMultiline() const;
  virtual void SetMultiline(bool multiline);
  virtual std::string GetPasswordChar() const;
  virtual void SetPasswordChar(const char *passwordChar);
  virtual bool IsStrikeout() const;
  virtual void SetStrikeout(bool strikeout);
  virtual bool IsUnderline() const;
  virtual void SetUnderline(bool underline);
  virtual std::string GetValue() const;
  virtual void SetValue(const char *value);
  virtual bool IsWordWrap() const;
  virtual void SetWordWrap(bool wrap);
  virtual bool IsReadOnly() const;
  virtual void SetReadOnly(bool readonly);
  virtual bool IsDetectUrls() const;
  virtual void SetDetectUrls(bool detect_urls);
  virtual void GetIdealBoundingRect(int *width, int *height);
  virtual void Select(int start, int end);
  virtual void SelectAll();
  virtual CanvasInterface::Alignment GetAlign() const;
  virtual void SetAlign(CanvasInterface::Alignment align);
  virtual CanvasInterface::VAlignment GetVAlign() const;
  virtual void SetVAlign(CanvasInterface::VAlignment valign);

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);
  virtual EventResult HandleOtherEvent(const Event &event);
  virtual void GetDefaultSize(double *width, double *height) const;
  virtual void OnFontSizeChange();

 private:
  void GetScrollBarInfo(int *x_range, int *y_range,
                        int *line_step, int *page_step,
                        int *cur_pos);
  void OnScrolled();
  void MoveCursor(QTextCursor::MoveOperation, int count,
                  bool extend_selection);
  void CutClipboard();
  void CopyClipboard();
  void PasteClipboard();
  void EnterText(QString str);
  void SetWidth(int width);
  void SetHeight(int height);
  void FocusIn();
  void FocusOut();
  int RealHeight();
  int RequestWidth();
  int RequestHeight();
  int RealWidth();
  void ScrollToCursor();
  EventResult HandleInputMethodEvent(QInputMethodEvent *e);

  QTextDocument doc_;
  QTextCursor *cursor_;
  bool multiline_;
  bool bold_;
  bool italic_;
  bool strikeout_;
  bool underline_;
  bool overwrite_;
  bool wrap_;
  bool readonly_;
  bool focused_;
  int page_line_;
  int width_, height_;
  int scroll_offset_x_;
  int scroll_offset_y_;
  Texture *background_;
  Color text_color_;
  std::string font_family_;
  QString password_char_;
  QAbstractTextDocumentLayout::PaintContext paint_ctx_;
  DISALLOW_EVIL_CONSTRUCTORS(QtEditElement);
};

} // namespace qt
} // namespace ggadget

#endif // GGADGET_QT_EDIT_ELEMENT_H__
