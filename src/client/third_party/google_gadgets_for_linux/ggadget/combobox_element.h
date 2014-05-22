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

#ifndef GGADGET_COMBOBOX_ELEMENT_H__
#define GGADGET_COMBOBOX_ELEMENT_H__

#include "basic_element.h"

namespace ggadget {

class EditElementBase;
class ListBoxElement;

/**
 * @ingroup Elements
 * Class of <a href=
 * "http://code.google.com/apis/desktop/docs/gadget_apiref.html#combobox">
 * combobox element</a>.
 */
class ComboBoxElement : public BasicElement {
 public:
  DEFINE_CLASS_ID(0x848a2f5e84144915, BasicElement);

  ComboBoxElement(View *view, const char *name);
  virtual ~ComboBoxElement();

 protected:
  virtual void DoClassRegister();

 public:
  virtual void MarkRedraw();

  /** Type of a combobox element. */
  enum Type {
    /** The default, editable control. */
    COMBO_DROPDOWN,
    /** Uneditable. */
    COMBO_DROPLIST,
  };

  /** Gets if the dropdown list is visible. */
  bool IsDroplistVisible() const;
  /** Sets if the dropdown list is visible. */
  void SetDroplistVisible(bool visible);

  /** Gets the maximum # of items to show before scrollbar is displayed. */
  size_t GetMaxDroplistItems() const;
  /** Sets the maximum # of items to show before scrollbar is displayed. */
  void SetMaxDroplistItems(size_t max_droplist_items);

  /** Gets the type of this combobox. */
  Type GetType() const;
  /** Sets the type of this combobox. */
  void SetType(Type type);

  /** Gets the value of the edit area, only for dropdown mode. */
  std::string GetValue() const;
  /** Sets the value of the edit area, only for dropdown mode. */
  void SetValue(const char *value);

  /** Gets the background color or image of the element. */
  Variant GetBackground() const;
  /**
   * Sets the background color or image of the element.
   * The image is repeated if necessary, not stretched.
   */
  void SetBackground(const Variant &background);

  //@{
  /** Gets the element of edit area, only for dropdown mode. */
  EditElementBase *GetEdit();
  const EditElementBase *GetEdit() const;
  //@}

  //@{
  /** Gets the element of droplist. */
  ListBoxElement *GetDroplist();
  const ListBoxElement *GetDroplist() const;
  //@}

  /**
   * Gets the background texture of the item under the mouse cursor.
   */
  Variant GetItemOverColor() const;
  /**
   * Sets the background texture of the item under the mouse cursor.
   * Comboboxes have no itemSelectedColor. itemOverColor is for both mouse
   * over color and item selected with the keyboard or by program.
   */
  void SetItemOverColor(const Variant &color);

  virtual const Elements *GetChildren() const;
  virtual Elements *GetChildren();

  virtual EventResult OnMouseEvent(const MouseEvent &event, bool direct,
                                   BasicElement **fired_element,
                                   BasicElement **in_element,
                                   ViewInterface::HitTest *hittest);
  virtual EventResult OnDragEvent(const DragEvent &event, bool direct,
                                  BasicElement **fired_element);

  virtual void OnPopupOff();

  Connection *ConnectOnChangeEvent(Slot0<void> *slot);

  virtual double GetPixelHeight() const;

  virtual bool IsTabStopDefault() const;

 public:
  static BasicElement *CreateInstance(View *view, const char *name);

 protected:
  virtual void Layout();
  virtual void DoDraw(CanvasInterface *canvas);
  virtual EventResult HandleMouseEvent(const MouseEvent &event);
  virtual EventResult HandleKeyEvent(const KeyboardEvent &event);
  virtual EventResult HandleOtherEvent(const Event &event);
  virtual void AggregateMoreClipRegion(const Rectangle &boundary,
                                       ClipRegion *region);

 public:
  virtual bool IsChildInVisibleArea(const BasicElement *child) const;
  virtual bool HasOpaqueBackground() const;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ComboBoxElement);

  class Impl;
  Impl *impl_;
};

} // namespace ggadget

#endif // GGADGET_COMBOBOX_ELEMENT_H__
