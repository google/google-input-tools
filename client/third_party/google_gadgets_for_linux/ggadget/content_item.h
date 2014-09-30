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

#ifndef GGADGET_CONTENT_ITEM_H__
#define GGADGET_CONTENT_ITEM_H__

#include <ggadget/color.h>
#include <ggadget/gadget.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/slot.h>
#include <ggadget/variant.h>

namespace ggadget {

class CanvasInterface;
class ContentAreaElement;
class Connection;
class DetailsViewData;
class ScriptableCanvas;
class ScriptableImage;
class View;

/**
 * @ingroup ContentAreaElement
 *
 * Class to implement <a href=
 * "http://code.google.com/apis/desktop/docs/gadget-contentitem_apiref.html">
 * ContentItem Object</a>
 */
class ContentItem : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x062fc66bb03640ca, ScriptableInterface);

  enum Layout {
    /** Single line with just the heading and icon. */
    CONTENT_ITEM_LAYOUT_NOWRAP_ITEMS = 0,
    /** A layout displaying the heading, source, and time. */
    CONTENT_ITEM_LAYOUT_NEWS = 1,
    /** A layout displaying the heading, source, time, and snippet. */
    CONTENT_ITEM_LAYOUT_EMAIL = 2,
  };

  enum Flags {
    /** No flags passed. */
    CONTENT_ITEM_FLAG_NONE              = 0x0000,
    /** Item does not take user input. */
    CONTENT_ITEM_FLAG_STATIC            = 0x0001,
    /** Item is highlighted/shown as bold. */
    CONTENT_ITEM_FLAG_HIGHLIGHTED       = 0x0002,
    /** Item is pinned to the top of the list. */
    CONTENT_ITEM_FLAG_PINNED            = 0x0004,
    /** Item's time is shown as absolute time. */
    CONTENT_ITEM_FLAG_TIME_ABSOLUTE     = 0x0008,
    /** Item can take negative feedback from user. */
    CONTENT_ITEM_FLAG_NEGATIVE_FEEDBACK = 0x0010,
    /** Item's icon should be displayed on the left side. */
    CONTENT_ITEM_FLAG_LEFT_ICON         = 0x0020,
    /** Do not show a 'remove' option for this item in the context menu. */
    CONTENT_ITEM_FLAG_NO_REMOVE         = 0x0040,
    /**
     * Item may be shared with friends. This will enable the spedific menu
     * item in the context menu and the button in the details view.
     */
    CONTENT_ITEM_FLAG_SHAREABLE         = 0x0080,
    /** This item was received from another user. */
    CONTENT_ITEM_FLAG_SHARED            = 0x0100,
    /** The user has interacted (viewed details/opened etc.) with this item. */
    CONTENT_ITEM_FLAG_INTERACTED        = 0x0200,
    /**
     * The content item's text strings (heading, source, snippet) should not be
     * converted to plain text before displaying them on screen. Without this
     * flag, HTML tags and other markup are stripped out. You can use this flag
     * to display HTML code as-is.
     */
    CONTENT_ITEM_FLAG_DISPLAY_AS_IS     = 0x0400,
    /**
     * The @c snippet property of the content item contains HTML text that
     * should be interpreted. Use this flag to make the content in the details
     * view be formatted as HTML. (GDWin: Setting this flag implicitly sets
     * the @c CONTENT_ITEM_FLAG_DISPLAY_AS_IS flag.)
     */
    CONTENT_ITEM_FLAG_HTML              = 0x0800,
    /* Hide content items while still having them in the data structures. */
    CONTENT_ITEM_FLAG_HIDDEN            = 0x1000,
  };

 public:
  ContentItem(View *view);

  /** Used to register the global script class constructor of ContentItem. */
  static ContentItem *CreateInstance(View *view);

 protected:
  virtual ~ContentItem();
  virtual void DoClassRegister();

 public:
  /** Called when this content item is added into a content area. */
  void AttachContentArea(ContentAreaElement *content_area);
  /** Called when this content item is removed from the content area. */
  void DetachContentArea(ContentAreaElement *content_area);

 public:
  /** Gets and sets the image to show in the item. */
  ScriptableImage *GetImage() const;
  void SetImage(ScriptableImage *image);

  /** Gets and sets the image to show in the notifier. */
  ScriptableImage *GetNotifierImage() const;
  void SetNotifierImage(ScriptableImage *image);

  /** Gets and sets the time that this content item is created. */
  Date GetTimeCreated() const;
  void SetTimeCreated(const Date &time);

  /** Gets the display string of the created time. */
  std::string GetTimeDisplayString() const;

  /** Gets and sets the item's display title. */
  std::string GetHeading() const;
  void SetHeading(const char *heading);

  /** Gets and sets the item's displayed website/news source. */
  std::string GetSource() const;
  void SetSource(const char *source);

  /** Gets and sets the item's displayed snippet. */
  std::string GetSnippet() const;
  void SetSnippet(const char *snippet);

  /**
   * Gets the displayed heading, source and snippet. The displayed text
   * may be different from the original text when the
   * @c CONTENT_ITEM_FLAG_DISPLAY_AS_IS flag is not set.
   */
  std::string GetDisplayHeading() const;
  std::string GetDisplaySource() const;
  std::string GetDisplaySnippet() const;

  /**
   * Gets and sets the URL/file path opened when the user opens/double clicks
   * the item.
   */
  std::string GetOpenCommand() const;
  void SetOpenCommand(const char *open_command);

  /** Gets and sets the format in which the item should be displayed. */
  Layout GetLayout() const;
  void SetLayout(Layout layout);

  /**
   * Gets and sets the flags.
   * @param flags combination of Flags.
   */
  int GetFlags() const;
  void SetFlags(int flags);

  /** Gets and sets the tooltip text, such as full headlines, etc. */
  std::string GetTooltip() const;
  void SetTooltip(const std::string &tooltip);

  /** Gets and sets the item's required display position. */
  void GetRect(double *x, double *y, double *width, double *height,
               bool *x_relative, bool *y_relative,
               bool *width_relative, bool *height_relative);
  void SetRect(double x, double y, double width, double height,
               bool x_relative, bool y_relative,
               bool width_relative, bool height_relative);

  /** Gets and sets the item's actual display position. */
  void GetLayoutRect(double *x, double *y, double *width, double *height);
  void SetLayoutRect(double x, double y, double width, double height);

  /** Returns if this item can be opened by the user. */
  bool CanOpen() const;

  /** Draws the item. */
  void Draw(Gadget::DisplayTarget target, CanvasInterface *canvas,
            double x, double y, double width, double height);
  Connection *ConnectOnDrawItem(
      Slot7<void, ContentItem *, Gadget::DisplayTarget,
            ScriptableCanvas *, double, double, double, double> *handler);

  /** Gets the height in pixels of the item for the given width. */
  double GetHeight(Gadget::DisplayTarget target,
                CanvasInterface *canvas, double width);
  Connection *ConnectOnGetHeight(
      Slot4<double, ContentItem *, Gadget::DisplayTarget,
            ScriptableCanvas *, double> *handler);

  /** Called when the user opens/double clicks the item. */
  void OpenItem();
  Connection *ConnectOnOpenItem(Slot1<bool, ContentItem *> *handler);

  /** Called when the user clicks the 'pin' button of an item. */
  void ToggleItemPinnedState();
  Connection *ConnectOnToggleItemPinnedState(
      Slot1<bool, ContentItem *> *handler);

  /** Called to check if a tooltip is required for the item. */
  bool IsTooltipRequired(Gadget::DisplayTarget target,
                         CanvasInterface *canvas,
                         double x, double y, double width, double height);
  Connection *ConnectOnGetIsTooltipRequired(
      Slot7<bool, ContentItem *, Gadget::DisplayTarget,
            ScriptableCanvas *, double, double, double, double> *handler);

  /**
   * Called before showing the details view for the given item.
   * @return true to cancel the details view.
   */
  bool OnDetailsView(std::string *title, DetailsViewData **details_view_data,
                     int *flags);
  typedef Slot1<ScriptableInterface *, ContentItem *> OnDetailsViewHandler;
  Connection *ConnectOnDetailsView(OnDetailsViewHandler *handler);

  /**
   * Connects to the signal which will be fired to process the user action in
   * the details view.
   * @param flags is combination of @c ViewHostInterface::DetailsViewFlags.
   * @return @c true to cancel the default action of this feedback, @c false to
   *     continue the default action.
   */
  bool ProcessDetailsViewFeedback(int flags);
  Connection *ConnectOnProcessDetailsViewFeedback(
      Slot2<bool, ContentItem *, int> *handler);

  /**
   * Called when the user removes and item from the display.
   * @return @c true or nothing to cancel the remove and keep the item,
   *     @c false to continue and remove the item.
   */
  bool OnUserRemove();
  Connection *ConnectOnRemoveItem(Slot1<bool, ContentItem *> *handler);

 public:
  /**
   * Format a time into string for display in content item or details view.
   * @param time the time to display.
   * @param current_time the current time if relative format needed, or 0
   *     if absolute format needed.
   * @param short_form if short form of time string needed.
   * @return the formatted time string.
   */
  static std::string GetTimeDisplayString(uint64_t time,
                                          uint64_t current_time,
                                          bool short_form);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ContentItem);
  class Impl;
  Impl *impl_;
};

/**
 * @ingroup ContentAreaElement
 *
 * The graphics interface used to draw content items.
 */
class ScriptableCanvas : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xa4f94b8abd754d7d, ScriptableInterface);
  ScriptableCanvas(CanvasInterface *canvas, View *view);
  virtual ~ScriptableCanvas();

 protected:
  virtual void DoClassRegister();

 public:
  enum FontID {
    /** Normal text font. */
    FONT_NORMAL = -703,
    /** Bold item text. */
    FONT_BOLD = 577,
    /** Snippet text, could be slightly smaller than normal font. */
    FONT_SNIPPET = 575,
    /** For extra info about items (e.g. source and time). */
    FONT_EXTRA_INFO = 576,
  };

  /** Color for normal text. */
  static const Color kColorNormalText;
  /** Color for the Sidebar background. */
  static const Color kColorNormalBackground;
  /** Color for snippet text. */
  static const Color kColorSnippet;
  /** Color for extra info about items (e.g. source and time). */
  static const Color kColorExtraInfo;

  /**
   * Flag values used in @c DrawText(), @c GetTextWidth() and @c GetTextHeight()
   * methods. The flags parameter can be combination of zero to more following
   * values.
   */
  enum TextFlag {
    /** Center text horizontally. */
    TEXT_FLAG_CENTER = 1,
    /** Right align text. */
    TEXT_FLAG_RIGHT = 2,
    /** Center text vertically. */
    TEXT_FLAG_VCENTER = 4,
    /** Bottom align text. */
    TEXT_FLAG_BOTTOM = 8,
    /** Break text at word boundaries when wrapping multiple lines. */
    TEXT_FLAG_WORD_BREAK = 16,
    /** Display text in a single line without line wraps. */
    TEXT_FLAG_SINGLE_LINE = 32,
  };

  CanvasInterface *canvas() { return canvas_; }

  void DrawLine(double x1, double y1, double x2, double y2, const Color &color);
  void DrawRect(double x1, double y1, double width, double height,
                const Color *line_color, const Color *fill_color);
  void DrawImage(double x, double y, double width, double height,
                 ScriptableImage *image, int alpha_percent);
  void DrawText(double x, double y, double width, double height, const char *text,
                const Color &color, int flags, FontID font);
  double GetTextWidth(const char *text, int flags, FontID font);
  double GetTextHeight(const char *text, double width, int flags, FontID font);

  void DrawLineWithColorName(double x1, double y1, double x2, double y2,
                             const char *color);
  void DrawRectWithColorName(double x1, double y1, double width, double height,
                             const char *line_color, const char *fill_color);
  void DrawTextWithColorName(double x, double y, double width, double height,
                             const char *text, const char *color,
                             int flags, FontID font);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableCanvas);
  CanvasInterface *canvas_;
  View *view_;
};

} // namespace ggadget

#endif  // GGADGET_CONTENT_ITEM_H__
