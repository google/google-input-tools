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

#include "hotkey.h"
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/messages.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifndef XK_MISCELLANY
// On some platforms, keysym.h is not included by Xutil.h.
#include <X11/keysym.h>
#endif
#include <gdk/gdkx.h>
#endif

namespace ggadget {
namespace gtk {

enum KeyMask {
  KEY_NullMask       = 0,            // Key press event without modifier key
  KEY_ShiftMask      = 1 << 0,       // The Shift key is pressed down
  KEY_ControlMask    = 1 << 1,       // The Control key is pressed down
  KEY_AltMask        = 1 << 2,       // The Alt  key is pressed down
  KEY_MetaMask       = 1 << 3,       // The Meta key is pressed down
  KEY_SuperMask      = 1 << 4,       // The Super key is pressed down
  KEY_HyperMask      = 1 << 5,       // The Hyper key is pressed down
};

struct KeyMaskInfo {
  unsigned int mask;
  const char* name;
};

static const KeyMaskInfo kKeyMaskNames[] = {
  { KEY_ShiftMask,   "Shift" },
  { KEY_ControlMask, "Ctrl" },
  { KEY_AltMask,     "Alt" },
  { KEY_MetaMask,    "Meta" },
  { KEY_SuperMask,   "Super" },
  { KEY_HyperMask,   "Hyper" }
};

class KeyEvent {
 public:
  KeyEvent() : key_value_(0), key_mask_(0) {
#ifdef GDK_WINDOWING_X11
    if (!display_)
      InitializeX11KeyMasks();
#endif
  }

  KeyEvent(unsigned int key_value, unsigned int key_mask)
    : key_value_(key_value), key_mask_(key_mask) {
#ifdef GDK_WINDOWING_X11
    if (!display_)
      InitializeX11KeyMasks();
#endif
  }

  explicit KeyEvent(const std::string &key_string)
    : key_value_(0), key_mask_(0) {
#ifdef GDK_WINDOWING_X11
    if (!display_)
      InitializeX11KeyMasks();
#endif
    if (!key_string.length()) return;

    unsigned int key_value = 0;
    unsigned int key_mask = 0;

    std::string::size_type pos = 0;
    std::string::size_type current = 0;
    while(current != std::string::npos) {
      current = key_string.find("-", pos);
      std::string key;
      if (current == pos) {  // "-" is not the delimiter here
        key = "-";
        key_value = gdk_keyval_from_name(key.c_str());
        pos = current + 1;
        break;
      } else if (current == std::string::npos) {
        key = key_string.substr(pos);
      } else {
        key = key_string.substr(pos, static_cast<int>(current - pos));
      }

      pos = current + 1;
      bool normal_key = true;
      for (size_t i = 0; i < arraysize(kKeyMaskNames); ++i) {
        if (strcmp(key.c_str(), kKeyMaskNames[i].name) == 0) {
          key_mask |= kKeyMaskNames[i].mask;
          normal_key = false;
          break;
        }
      }
      if (normal_key) {
        if (key_value) return;  // already have a key, invalid
        key_value = gdk_keyval_from_name(key.c_str());
        if (key_value == GDK_VoidSymbol || !key_value)
          return;
      }
    }

    key_value_ = key_value;
    key_mask_ = key_mask;
  }

  explicit KeyEvent(const GdkEventKey *gdk_key)
    : key_value_(0), key_mask_(0) {
#ifdef GDK_WINDOWING_X11
    if (!display_)
      InitializeX11KeyMasks();
#endif
    if (!gdk_key) return;

    key_value_ = gdk_key->keyval;
    key_mask_ = 0;
    unsigned int state = gdk_key->state;
#ifdef GDK_WINDOWING_X11
    if (meta_mask_ && (state & meta_mask_) == meta_mask_) {
      key_mask_ |= KEY_MetaMask;
      state &= ~meta_mask_;
    }
    if (alt_mask_ && (state & alt_mask_) == alt_mask_) {
      key_mask_ |= KEY_AltMask;
      state &= ~alt_mask_;
    }
    if (super_mask_ && (state & super_mask_) == super_mask_) {
      key_mask_ |= KEY_SuperMask;
      state &= ~super_mask_;
    }
    if (hyper_mask_ && (state & hyper_mask_) == hyper_mask_) {
      key_mask_ |= KEY_HyperMask;
      state &= ~hyper_mask_;
    }
#endif
    if (state & GDK_SHIFT_MASK)
      key_mask_ |= KEY_ShiftMask;
    if (gdk_key->state & GDK_CONTROL_MASK)
      key_mask_ |= KEY_ControlMask;
  }

  bool IsValid() const {
    return key_value_ != 0 && key_value_ != GDK_VoidSymbol;
  }

  bool IsNormalKey() const {
    static const unsigned int kSpecialKeyValues[] = {
      GDK_Shift_L,
      GDK_Shift_R,
      GDK_Control_L,
      GDK_Control_R,
      GDK_Alt_L,
      GDK_Alt_R,
      GDK_Meta_L,
      GDK_Meta_R,
      GDK_Super_L,
      GDK_Super_R,
      GDK_Hyper_L,
      GDK_Hyper_R,
      GDK_Num_Lock,
      GDK_Caps_Lock // Num_Lock & Caps_Lock should be considered as special key.
    };

    if (!key_value_ || key_value_ == GDK_VoidSymbol)
      return false;
    for (size_t i = 0; i < arraysize(kSpecialKeyValues); ++i) {
      if (key_value_ == kSpecialKeyValues[i])
        return false;
    }
    return true;
  }

  void Reset() {
    key_value_ = 0;
    key_mask_ = 0;
  }

  // Used for getting the final hot key, usually a hotkey should be set by
  // several keyevents, for example, when user want to use a Ctrl-X as a hotkey,
  // four keyevents will come out: Ctrl-press, X-press, X-release, Ctrl-release,
  // note that the press and release order may not be the same as the example.
  void AppendKeyEvent(const KeyEvent &key, bool press) {
    key_mask_ |= key.key_mask_;
    if (press || key.IsNormalKey())
      key_value_ = key.key_value_;
  }

  unsigned int GetKeyValue() const {
    return key_value_;
  }

  void SetKeyValue(unsigned int key_value) {
    key_value_ = key_value;
  }

  unsigned int GetKeyMask() const {
    return key_mask_;
  }

  void SetKeyMask(unsigned int key_mask) {
    key_mask_ = key_mask;
  }

  std::string GetKeyString() const {
    std::string key_string;
    for (size_t i = 0; i < arraysize(kKeyMaskNames); ++i) {
      if (key_mask_ & kKeyMaskNames[i].mask)
        AppendKeyString(kKeyMaskNames[i].name, &key_string);
    }
    if (0 != key_value_ && GDK_VoidSymbol != key_value_) {
      char *name = gdk_keyval_name(key_value_);
      if (name && *name)
        AppendKeyString(name, &key_string);
    }
    return key_string;
  }

#ifdef GDK_WINDOWING_X11
  unsigned int GetX11KeyCode() const {
    return XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                            key_value_);
  }

  unsigned int GetX11KeyMask() const {
    unsigned int x11mask = 0;
    if (key_mask_ & KEY_ShiftMask)
      x11mask |= ShiftMask;
    if (key_mask_ & KEY_ControlMask)
      x11mask |= ControlMask;
    if (key_mask_ & KEY_AltMask)
      x11mask |= alt_mask_;
    if (key_mask_ & KEY_MetaMask)
      x11mask |= meta_mask_;
    if (key_mask_ & KEY_SuperMask)
      x11mask |= super_mask_;
    if (key_mask_ & KEY_HyperMask)
      x11mask |= hyper_mask_;
    return x11mask;
  }

  static unsigned int GetX11MaskForModifierKey(unsigned int keycode) {
    if (!display_)
      InitializeX11KeyMasks();
    switch (keycode) {
      case GDK_Shift_L:
      case GDK_Shift_R:
        return ShiftMask;
      case GDK_Control_L:
      case GDK_Control_R:
        return ControlMask;
      case GDK_Alt_L:
      case GDK_Alt_R:
        return alt_mask_;
      case GDK_Meta_L:
      case GDK_Meta_R:
        return meta_mask_;
      case GDK_Super_L:
      case GDK_Super_R:
        return super_mask_;
      case GDK_Hyper_L:
      case GDK_Hyper_R:
        return hyper_mask_;
      case GDK_Num_Lock:
        return numlock_mask_;
      case GDK_Caps_Lock:
        return LockMask;
      default:
        return 0;
    }
  }
#endif

 private:
  static void AppendKeyString(const char *new_key, std::string *key_string) {
    if (new_key && *new_key) {
      if (key_string->size())
        key_string->append("-");
      key_string->append(new_key);
    }
  }

 private:
  unsigned int key_value_;
  unsigned int key_mask_;

#ifdef GDK_WINDOWING_X11
  static void InitializeX11KeyMasks() {
    if (display_) return;
    display_ = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    if (!display_)
      return;

    XModifierKeymap *mods = XGetModifierMapping(display_);
    alt_mask_ = 0;
    meta_mask_ = 0;
    super_mask_ = 0;
    hyper_mask_ = 0;
    numlock_mask_ = 0;

    int ctrl_l  = XKeysymToKeycode(display_, XK_Control_L);
    int ctrl_r  = XKeysymToKeycode(display_, XK_Control_R);
    int meta_l  = XKeysymToKeycode(display_, XK_Meta_L);
    int meta_r  = XKeysymToKeycode(display_, XK_Meta_R);
    int alt_l   = XKeysymToKeycode(display_, XK_Alt_L);
    int alt_r   = XKeysymToKeycode(display_, XK_Alt_R);
    int super_l = XKeysymToKeycode(display_, XK_Super_L);
    int super_r = XKeysymToKeycode(display_, XK_Super_R);
    int hyper_l = XKeysymToKeycode(display_, XK_Hyper_L);
    int hyper_r = XKeysymToKeycode(display_, XK_Hyper_R);
    int numlock = XKeysymToKeycode(display_, XK_Num_Lock);

    // We skip the first three sets for Shift, Lock, and Control.
    // The remaining sets are for Mod1, Mod2, Mod3, Mod4, and Mod5.
    for (int i = 3; i < 8; i++) {
      for (int j = 0; j < mods->max_keypermod; j++) {
        int code = mods->modifiermap[i * mods->max_keypermod + j];
        if (!code) continue;
        if (code == alt_l || code == alt_r)
          alt_mask_ |= (1 << i);
        else if (code == meta_l || code == meta_r)
          meta_mask_ |= (1 << i);
        else if (code == super_l || code == super_r)
          super_mask_ |= (1 << i);
        else if (code == hyper_l || code == hyper_r)
          hyper_mask_ |= (1 << i);
        else if (code == numlock)
          numlock_mask_ |= (1 << i);
      }
    }

    /* Check whether there is a combine keys mapped to Meta */
    if (!meta_mask_) {
      char buf [32];
      XKeyEvent xkey;
      KeySym keysym_l, keysym_r;

      xkey.type = KeyPress;
      xkey.display = display_;
      xkey.serial = 0L;
      xkey.send_event = False;
      xkey.x = 0;
      xkey.y = 0;
      xkey.x_root = 0;
      xkey.y_root = 0;
      xkey.time = 0;
      xkey.same_screen = False;
      xkey.subwindow = None;
      xkey.window = None;
      xkey.root = DefaultRootWindow(display_);
      xkey.state = ShiftMask;

      xkey.keycode = meta_l;
      XLookupString(&xkey, buf, 32, &keysym_l, 0);
      xkey.keycode = meta_r;
      XLookupString(&xkey, buf, 32, &keysym_r, 0);

      if ((meta_l == alt_l && keysym_l == XK_Meta_L) ||
          (meta_r == alt_r && keysym_r == XK_Meta_R))
        meta_mask_ = ShiftMask + alt_mask_;
      else if ((meta_l == ctrl_l && keysym_l == XK_Meta_L) ||
               (meta_r == ctrl_r && keysym_r == XK_Meta_R))
        meta_mask_ = ShiftMask + ControlMask;
    }

    XFreeModifiermap (mods);
    DLOG("Modifier key masks: a:0x%x m:0x%x s:0x%x h:0x%x n:0x%x",
         alt_mask_, meta_mask_, super_mask_,
         hyper_mask_, numlock_mask_);
  }

  // used for XKeyEvent conversion
  static Display *display_;
  static unsigned int alt_mask_;
  static unsigned int meta_mask_;
  static unsigned int super_mask_;
  static unsigned int hyper_mask_;
  static unsigned int numlock_mask_;
#endif
};

#ifdef GDK_WINDOWING_X11
Display *KeyEvent::display_ = NULL;
unsigned int KeyEvent::alt_mask_ = Mod1Mask;
unsigned int KeyEvent::meta_mask_ = ShiftMask | Mod1Mask;
unsigned int KeyEvent::super_mask_ = 0;
unsigned int KeyEvent::hyper_mask_ = 0;
unsigned int KeyEvent::numlock_mask_ = 0;
#endif

// Records the complete sequence of a key event.
class KeyEventRecorder {
 public:
  KeyEventRecorder() : key_pressed_count_(0) { }

  void Reset() {
    key_pressed_count_ = 0;
    key_event_.Reset();
  }

  bool PushKeyEvent(const KeyEvent &key, bool press, KeyEvent *complete_key) {
    key_event_.AppendKeyEvent(key, press);
    if (press) {
      ++key_pressed_count_;
    } else {
      --key_pressed_count_;
      ASSERT(key_pressed_count_ >= 0);
      if (key_pressed_count_ <= 0) {
        if (complete_key) *complete_key = key_event_;
        key_event_.Reset();
        return true;
      }
    }
    return false;
  }

 private:
  int key_pressed_count_;
  KeyEvent key_event_;
};

class HotKeyGrabber::Impl {
 public:
  Impl(GdkScreen *screen)
    : root_window_(NULL),
#ifdef GDK_WINDOWING_X11
      x11_keycode_(0),
      x11_keymask_(0),
#endif
      is_grabbing_(false) {
    SetScreen(screen);
  }

  void SetScreen(GdkScreen *screen) {
    bool grabbing = is_grabbing_;
    if (grabbing)
      SetEnableGrabbing(false);
    if (screen)
      root_window_ = gdk_screen_get_root_window(screen);
    if (!root_window_)
      root_window_ = gdk_get_default_root_window();
    ASSERT(root_window_);
    if (root_window_) {
      gdk_window_set_events(root_window_, static_cast<GdkEventMask>(
          gdk_window_get_events(root_window_) |
          GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK));
    }
    if (grabbing)
      SetEnableGrabbing(true);
  }

  ~Impl() {
    SetEnableGrabbing(false);
  }

  bool SetHotKey(const std::string &hotkey) {
    bool grabbing = is_grabbing_;
    if (grabbing)
      SetEnableGrabbing(false);
    hotkey_ = KeyEvent(hotkey);
#ifdef GDK_WINDOWING_X11
    x11_keycode_ = hotkey_.GetX11KeyCode();
    x11_keymask_ = hotkey_.GetX11KeyMask();
    // For modifier key, we still grab key press, because grabbing key release
    // is not reliable.  So remove itself from its masks.
    if (!hotkey_.IsNormalKey())
      x11_keymask_ &=
          ~KeyEvent::GetX11MaskForModifierKey(hotkey_.GetKeyValue());
#endif
    if (grabbing && hotkey_.IsValid())
      SetEnableGrabbing(true);
    return hotkey_.IsValid() && grabbing == is_grabbing_;
  }

  std::string GetHotKey() const {
    return hotkey_.GetKeyString();
  }

  void SetEnableGrabbing(bool grabbing) {
    if (!root_window_ || is_grabbing_ == grabbing)
      return;

#ifdef GDK_WINDOWING_X11
    unsigned int masks[4];
    // Pressing the hotkey with capslock or numlock turned on should be
    // allowed. So the hotkey with all possible mask combinations must be
    // grabbed.
    masks[0] = x11_keymask_;
    masks[1] = masks[0] | KeyEvent::GetX11MaskForModifierKey(GDK_Num_Lock);
    masks[2] = masks[0] | KeyEvent::GetX11MaskForModifierKey(GDK_Caps_Lock);
    masks[3] = masks[1] | masks[2];

    if (grabbing && hotkey_.IsValid()) {
      for (int i = 0; i < 4; ++i) {
        gdk_error_trap_push();
        XGrabKey(GDK_WINDOW_XDISPLAY(root_window_), x11_keycode_, masks[i],
                 GDK_WINDOW_XWINDOW(root_window_), True, GrabModeAsync,
                 GrabModeAsync);
        XSync(GDK_WINDOW_XDISPLAY(root_window_), False);
        if (gdk_error_trap_pop() == 0)
          is_grabbing_ = true;
      }
      if (is_grabbing_)
        gdk_window_add_filter(root_window_, KeyEventFilter, this);
    } else if (is_grabbing_) {
      gdk_error_trap_push();
      for (int i = 0; i < 4; ++i) {
        XUngrabKey(GDK_WINDOW_XDISPLAY(root_window_), x11_keycode_, masks[i],
                   GDK_WINDOW_XWINDOW(root_window_));
      }
      gdk_error_trap_pop();
      gdk_window_remove_filter(root_window_, KeyEventFilter, this);
      is_grabbing_ = false;
    }
#endif
  }

  bool IsGrabbing() const {
    return is_grabbing_;
  }

  Connection* ConnectOnHotKeyPressed(Slot0<void> *slot) {
    return on_hotkey_pressed_signal_.Connect(slot);
  }

 private:
#ifdef GDK_WINDOWING_X11
  static GdkFilterReturn KeyEventFilter(GdkXEvent *gxe, GdkEvent *,
                                        gpointer data) {
    Impl *impl = reinterpret_cast<Impl*>(data);
    XKeyEvent *xkey_event = reinterpret_cast<XKeyEvent*>(gxe);

    if (!impl->is_grabbing_)
      return GDK_FILTER_CONTINUE;

    unsigned int mask = KeyEvent::GetX11MaskForModifierKey(XK_Num_Lock);
    mask |= KeyEvent::GetX11MaskForModifierKey(XK_Caps_Lock);
    if (xkey_event->type == KeyPress &&
        xkey_event->keycode == impl->x11_keycode_ &&
        (xkey_event->state & ~mask) == impl->x11_keymask_) {
      DLOG("Hotkey pressed: code:0x%x mask:0x%x",
           xkey_event->keycode, xkey_event->state);
      impl->on_hotkey_pressed_signal_();
    }
    return GDK_FILTER_CONTINUE;
  }
#endif

 private:
  GdkWindow *root_window_;
  Signal0<void> on_hotkey_pressed_signal_;
  KeyEvent hotkey_;
#ifdef GDK_WINDOWING_X11
  unsigned int x11_keycode_;
  unsigned int x11_keymask_;
#endif
  bool is_grabbing_;
};

HotKeyGrabber::HotKeyGrabber()
  : impl_(new Impl(NULL)) {
}

HotKeyGrabber::HotKeyGrabber(GdkScreen *screen)
  : impl_(new Impl(screen)) {
}

HotKeyGrabber::~HotKeyGrabber() {
  delete impl_;
  impl_ = NULL;
}

bool HotKeyGrabber::SetHotKey(const std::string &hotkey) {
  return impl_->SetHotKey(hotkey);
}

std::string HotKeyGrabber::GetHotKey() const {
  return impl_->GetHotKey();
}

void HotKeyGrabber::SetEnableGrabbing(bool grabbing) {
  impl_->SetEnableGrabbing(grabbing);
}

bool HotKeyGrabber::IsGrabbing() const {
  return impl_->IsGrabbing();
}

Connection* HotKeyGrabber::ConnectOnHotKeyPressed(Slot0<void> *slot) {
  return impl_->ConnectOnHotKeyPressed(slot);
}

class HotKeyDialog::Impl {
 public:
  Impl()
    : dialog_(NULL),
      entry_(NULL),
      label_(NULL) {
    dialog_ = gtk_dialog_new_with_buttons(GM_("DEFAULT_HOTKEY_DIALOG_TITLE"),
                                          NULL,
                                          GTK_DIALOG_MODAL,
                                          GTK_STOCK_CANCEL,
                                          GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_OK,
                                          GTK_RESPONSE_OK,
                                          NULL);
    GtkWidget *hbox = gtk_hbox_new(FALSE, 4);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);
    label_ = gtk_label_new(GM_("DEFAULT_HOTKEY_DIALOG_PROMPT"));
    gtk_box_pack_start(GTK_BOX(hbox), label_, false, false, 0);

    entry_ = gtk_entry_new();
    gtk_entry_set_editable(GTK_ENTRY(entry_), FALSE);
    // FIXME: Is it enough?
    gtk_widget_set_size_request(entry_, 200, -1);
    gtk_box_pack_start(GTK_BOX(hbox), entry_, true, true, 0);
    g_signal_connect(GTK_OBJECT(entry_), "button-press-event",
                     G_CALLBACK(OnEntryClicked), this);
    g_signal_connect(GTK_OBJECT(entry_), "key-press-event",
                     G_CALLBACK(OnEntryKeyPressed), this);
    g_signal_connect(GTK_OBJECT(entry_), "key-release-event",
                     G_CALLBACK(OnEntryKeyReleased), this);
    gtk_widget_show_all(hbox);

    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog_)->vbox), hbox);

    GtkWidget *button = gtk_button_new_with_label(GM_("CLEAR_HOTKEY"));
    g_signal_connect(GTK_OBJECT(button), "clicked",
                     G_CALLBACK(OnClearButtonClicked), this);
    gtk_widget_show(button);
    gtk_box_pack_end(GTK_BOX(GTK_DIALOG(dialog_)->action_area), button,
                     false, false, 0);
    gtk_button_box_set_child_secondary(
        GTK_BUTTON_BOX(GTK_DIALOG(dialog_)->action_area), button, TRUE);

    gtk_window_set_resizable(GTK_WINDOW(dialog_), FALSE);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog_), GTK_RESPONSE_OK);
  }
  ~Impl() {
    gtk_widget_destroy(dialog_);
  }

  void SetTitle(const char *title) {
    gtk_window_set_title(GTK_WINDOW(dialog_), title);
  }

  void SetPrompt(const char *prompt) {
    gtk_label_set_text(GTK_LABEL(label_), prompt);
  }

  bool Show() {
    UpdateEntryText(false);
    gtk_window_set_position(GTK_WINDOW(dialog_), GTK_WIN_POS_CENTER);
    gtk_widget_show_all(dialog_);
    gtk_window_set_focus(GTK_WINDOW(dialog_), NULL);
    gint result = gtk_dialog_run(GTK_DIALOG(dialog_));
    gtk_widget_hide(dialog_);
    return result == GTK_RESPONSE_OK;
  }

  void SetHotKey(const std::string &hotkey) {
    hotkey_ = KeyEvent(hotkey);
    recorder_.Reset();
    gtk_window_set_focus(GTK_WINDOW(dialog_), NULL);
    UpdateEntryText(false);
  }

  std::string GetHotKey() const {
    return hotkey_.GetKeyString();
  }

 private:
  void UpdateEntryText(bool capturing) {
    if (capturing)
      gtk_entry_set_text(GTK_ENTRY(entry_), GM_("HOTKEY_ENTRY_PROMPT"));
    else
      gtk_entry_set_text(GTK_ENTRY(entry_),
                         hotkey_.IsValid() ? hotkey_.GetKeyString().c_str() :
                         GM_("HOTKEY_DISABLED"));
  }

  static gboolean OnEntryClicked(GtkWidget *widget, GdkEventButton *event,
                                 gpointer data) {
    GGL_UNUSED(event);
    Impl *impl = reinterpret_cast<Impl *>(data);
    gtk_widget_grab_focus(widget);
    impl->UpdateEntryText(true);
    impl->recorder_.Reset();
    impl->hotkey_.Reset();
    return TRUE;
  }

  static gboolean OnEntryKeyPressed(GtkWidget *widget, GdkEventKey *event,
                                    gpointer data) {
    GGL_UNUSED(widget);
    Impl *impl = reinterpret_cast<Impl *>(data);
    KeyEvent key(event);
    if (key.IsValid())
      impl->recorder_.PushKeyEvent(key, true, NULL);
    return TRUE;
  }

  static gboolean OnEntryKeyReleased(GtkWidget *widget, GdkEventKey *event,
                                     gpointer data) {
    GGL_UNUSED(widget);
    Impl *impl = reinterpret_cast<Impl *>(data);
    KeyEvent key(event);
    if (key.IsValid() && impl->recorder_.PushKeyEvent(key, false, &key)) {
      gtk_window_set_focus(GTK_WINDOW(impl->dialog_), NULL);
      impl->hotkey_ = key;
      impl->UpdateEntryText(false);
    }
    return TRUE;
  }

  static void OnClearButtonClicked(GtkButton *button, gpointer data) {
    GGL_UNUSED(button);
    Impl *impl = reinterpret_cast<Impl *>(data);
    gtk_window_set_focus(GTK_WINDOW(impl->dialog_), NULL);
    impl->hotkey_.Reset();
    impl->UpdateEntryText(false);
  }

  GtkWidget *dialog_;
  GtkWidget *entry_;
  GtkWidget *label_;
  KeyEvent hotkey_;
  KeyEventRecorder recorder_;
};

HotKeyDialog::HotKeyDialog()
  : impl_(new Impl()) {
}

HotKeyDialog::~HotKeyDialog() {
  delete impl_;
  impl_ = NULL;
}

void HotKeyDialog::SetTitle(const char *title) {
  impl_->SetTitle(title);
}

void HotKeyDialog::SetPrompt(const char *prompt) {
  impl_->SetPrompt(prompt);
}

bool HotKeyDialog::Show() {
  return impl_->Show();
}

void HotKeyDialog::SetHotKey(const std::string &hotkey) {
  impl_->SetHotKey(hotkey);
}

std::string HotKeyDialog::GetHotKey() const {
  return impl_->GetHotKey();
}

} // namespace gtk
} // namespace ggadget
