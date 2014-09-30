/*
  Copyright 2011 Google Inc.

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

#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cstdlib>
#include <cctype>
#include <vector>
#include <cmath>
#include <algorithm>
#include <gtk/gtk.h>
#include <ggadget/element_factory.h>
#include <ggadget/elements.h>
#include <ggadget/gadget_interface.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/scriptable_function.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/scriptable_holder.h>
#include <ggadget/string_utils.h>
#include <ggadget/unicode_utils.h>
#include <ggadget/view.h>
#include <ggadget/script_context_interface.h>

#include "browser_element.h"
#include "browser_child.h"

#define Initialize gtkmoz_browser_element_LTX_Initialize
#define Finalize gtkmoz_browser_element_LTX_Finalize
#define RegisterElementExtension \
    gtkmoz_browser_element_LTX_RegisterElementExtension

namespace ggadget {
namespace gtkmoz {
static MainLoopInterface *ggl_main_loop = NULL;
}
}

extern "C" {
  static void OnSigChild(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
  }

  bool Initialize() {
    LOGI("Initialize gtkmoz_browser_element extension.");
    ggadget::gtkmoz::ggl_main_loop = ggadget::GetGlobalMainLoop();
    ASSERT(ggadget::gtkmoz::ggl_main_loop);
    signal(SIGCHLD, OnSigChild);
    return true;
  }

  void Finalize() {
    LOGI("Finalize gtkmoz_browser_element extension.");
    ggadget::gtkmoz::ggl_main_loop = NULL;
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    LOGI("Register gtkmoz_browser_element extension, using name \"_browser\".");
    if (factory) {
      factory->RegisterElementClass(
          "_browser", &ggadget::gtkmoz::BrowserElement::CreateInstance);
    }
    return true;
  }
}

namespace ggadget {
namespace gtkmoz {

static const char *kBrowserChildNames[] = {
#if _DEBUG
  "gtkmoz-browser-child",
#endif
  GGL_LIBEXEC_DIR "/gtkmoz-browser-child",
  NULL
};

static const std::string kUndefinedStr("undefined");
static const std::string kNullStr("null");
static const std::string kTrueStr("true");
static const std::string kFalseStr("false");

class BrowserElementImpl;

// Manages RPC (though pipes) to the browser-child process.
// There is only one BrowserController and one browser-child process.
class BrowserController {
 public:
  BrowserController()
      : child_pid_(0),
        down_fd_(0), up_fd_(0),
        up_fd_watch_(0),
        ping_timer_watch_(0),
        ping_flag_(false),
        browser_seq_(0),
        recursion_depth_(0),
        command_start_time_(0),
        first_command_(false) {
  }

  ~BrowserController() {
    StopChild(false);
    instance_ = NULL;
  }

  bool PingTimerCallback(int watch) {
    if (!ping_flag_ || browser_elements_.empty()) {
      LOG("Browser child ping timeout or there is no browser element.");
      StopChild(true);
    }
    ping_flag_ = false;
    return true;
  }

  static BrowserController *get() {
    if (!instance_)
      instance_ = new BrowserController();
    return instance_;
  }

  void StartChild() {
    int down_fds[2], up_fds[2];
    if (pipe(down_fds) == -1) {
      LOG("Failed to create downwards pipe to browser child");
      return;
    }
    if (pipe(up_fds) == -1) {
      LOG("Failed to create upwards pipe to browser child");
      close(down_fds[0]); close(down_fds[1]);
      return;
    }

    child_pid_ = fork();
    if (child_pid_ == -1) {
      LOG("Failed to fork browser child");
      close(down_fds[0]); close(down_fds[1]);
      close(up_fds[0]); close(up_fds[1]);
      return;
    }

    if (child_pid_ == 0) {
      // This is the child process.
      close(down_fds[1]);
      close(up_fds[0]);
      std::string down_fd_str = StringPrintf("%d", down_fds[0]);
      std::string up_fd_str = StringPrintf("%d", up_fds[1]);
      for (size_t i = 0; kBrowserChildNames[i]; ++i) {
        execl(kBrowserChildNames[i], kBrowserChildNames[i],
              down_fd_str.c_str(), up_fd_str.c_str(), NULL);
      }
      LOG("Failed to execute browser child");
      _exit(-1);
    } else {
      close(down_fds[0]);
      close(up_fds[1]);
      down_fd_ = down_fds[1];
      up_fd_ = up_fds[0];
      int up_fd_flags = fcntl(up_fd_, F_GETFL);
      up_fd_flags |= O_NONBLOCK;
      fcntl(up_fd_, F_SETFL, up_fd_flags);
      up_fd_watch_ = ggl_main_loop->AddIOReadWatch(up_fd_,
          new WatchCallbackSlot(
              NewSlot(this, &BrowserController::OnUpFDReady)));
      ping_timer_watch_ = ggl_main_loop->AddTimeoutWatch(kPingInterval * 3 / 2,
          new WatchCallbackSlot(
              NewSlot(this, &BrowserController::PingTimerCallback)));
      first_command_ = true;
    }
  }

  void StopChild(bool on_error) {
    up_buffer_.clear();
    if (child_pid_) {
      ggl_main_loop->RemoveWatch(up_fd_watch_);
      up_fd_watch_ = 0;
      ggl_main_loop->RemoveWatch(ping_timer_watch_);
      ping_timer_watch_ = 0;

      // Don't send QUIT command on error to prevent error loops.
      if (!on_error) {
        std::string quit_command(kQuitCommand);
        quit_command += kEndOfMessageFull;
        Write(down_fd_, quit_command.c_str(), quit_command.size());
      }

      up_fd_watch_ = 0;
      close(down_fd_);
      down_fd_ = 0;
      close(up_fd_);
      up_fd_ = 0;
      kill(child_pid_, SIGTERM);
      child_pid_ = 0;
      DestroyAllBrowsers();
    }
  }

  void DestroyAllBrowsers();

  size_t AddBrowserElement(BrowserElementImpl *impl) {
    if (!child_pid_)
      StartChild();
    browser_elements_[++browser_seq_] = impl;
    DLOG("Added browser %zu. Total %zu browsers open", browser_seq_,
         browser_elements_.size());
    return browser_seq_;
  }

  void CloseBrowser(size_t id, bool send_command) {
    if (browser_elements_.erase(id)) {
      if (send_command)
        SendCommand(kCloseBrowserCommand, id, NULL);
      DLOG("Closed browser %zu. %zu browsers left", id,
           browser_elements_.size());
    }
  }

  bool OnUpFDReady(int) {
    ReadUpPipe();
    return true;
  }

  // Read the up pipe.
  // If any request received, processes it and returns a blank string.
  // If any reply received, returns it.
  // If only partial request received, saves into up_buffer_ and returns a
  // blank string.
  std::string ReadUpPipe() {
    char buffer[4096];
    ssize_t read_bytes;
    while ((read_bytes = read(up_fd_, buffer, sizeof(buffer))) > 0) {
      up_buffer_.append(buffer, read_bytes);
      if (read_bytes < static_cast<ssize_t>(sizeof(buffer)))
        break;
    }
    if (read_bytes <= 0) {
      // Because we ensure up_fd_ has data before calling ReadUpPipe(),
      // read() should not return 0.
      LOG("Failed to read up pipe");
      StopChild(true);
    }

    std::string reply;
    // In rare cases that up_buffer_ can contain more than one messages.
    // For example, child sends a ping feedback immediately after a reply.
    while (true) {
      if (strncmp(up_buffer_.c_str(), kReplyPrefix, kReplyPrefixLength) == 0) {
        // This message is a reply message.
        size_t eom_pos = up_buffer_.find('\n');
        if (eom_pos == up_buffer_.npos)
          break;
        reply = up_buffer_.substr(0, eom_pos + 1);
        up_buffer_.erase(0, eom_pos + 1);
      } else {
        size_t eom_pos = up_buffer_.find(kEndOfMessageFull);
        if (eom_pos == up_buffer_.npos)
          break;

        std::string message(up_buffer_, 0, eom_pos + kEOMFullLength);
        up_buffer_.erase(0, eom_pos + kEOMFullLength);

        static const size_t kMaxParams = 20;
        size_t curr_pos = 0;
        size_t param_count = 0;
        const char *params[kMaxParams];
        while (curr_pos <= eom_pos) {
          size_t end_of_line_pos = message.find('\n', curr_pos);
          ASSERT(end_of_line_pos != message.npos);
          message[end_of_line_pos] = '\0';
          if (param_count < kMaxParams) {
            params[param_count] = message.c_str() + curr_pos;
            param_count++;
          } else {
            LOG("Too many up message parameter");
            // Don't exit to recover from the error status.
          }
          curr_pos = end_of_line_pos + 1;
        }
        ASSERT(curr_pos = eom_pos + 1);

        ProcessFeedback(param_count, params);
      }
    }
    return reply;
  }

  // Defined later because it depends on BrowserElementImpl class.
  void ProcessFeedback(size_t param_count, const char **params);

  std::string SendCommandBuffer(const std::string &command) {
    if (down_fd_ == 0) {
      LOG("No browser-child available");
      return "";
    }
    Write(down_fd_, command.c_str(), command.size());
    DLOG("[%d] ==> SendCommand: %.80s...", recursion_depth_, command.c_str());

    static const uint64_t kWholeTimeout = 5000;
    static const int kSingleTimeout = 1500;
    static const int kMaxRecursion = 500;
    if (recursion_depth_ == 0)
      command_start_time_ = ggl_main_loop->GetCurrentTime();
    if (recursion_depth_ >= kMaxRecursion) {
      LOG("Too much recursion");
      // Force all recursions to break;
      command_start_time_ = 0;
      return "";
    }

    ++recursion_depth_;
    std::string reply;
    do {
      struct pollfd poll_fd = { up_fd_, POLLIN, 0 };
      int ret = poll(&poll_fd, 1,
                     first_command_ ? kWholeTimeout : kSingleTimeout);
      if (ret > 0) {
        reply = ReadUpPipe();
        if (!reply.empty())
          break;
      } else {
        break;
      }
    } while (ggl_main_loop->GetCurrentTime() - command_start_time_ <
             kWholeTimeout);

    --recursion_depth_;
    if (reply.empty()) {
      LOG("Failed to read command reply: current_buffer='%s'",
          up_buffer_.c_str());
      // Force all recursions to break;
      command_start_time_ = 0;
      if (recursion_depth_ == 0)
        StopChild(true);
      return reply;
    }

    first_command_ = false;
    // Remove the reply prefix and ending '\n'.
    reply.erase(0, kReplyPrefixLength);
    reply.erase(reply.size() - 1, 1);
    DLOG("[%d] <== SendCommand reply: %.40s...",
         recursion_depth_, reply.c_str());
    return reply;
  }

  std::string SendCommand(const char *type, size_t browser_id, ...) {
    if (down_fd_ == 0) {
      LOG("No browser-child available");
      return "";
    }
    std::string buffer(StringPrintf("%s\n%zu", type, browser_id));

    va_list ap;
    va_start(ap, browser_id);
    const char *param;
    while ((param = va_arg(ap, const char *)) != NULL) {
      buffer += '\n';
      buffer += param;
    }
    buffer += kEndOfMessageFull;
    return SendCommandBuffer(buffer);
  }

  static void OnSigPipe(int sig) {
    LOG("SIGPIPE Signal");
    instance_->StopChild(true);
  }

  void Write(int fd, const char *data, size_t size) {
    sig_t old_handler = signal(SIGPIPE, OnSigPipe);
    if (write(fd, data, size) < 0) {
      LOG("Failed to write to pipe");
      StopChild(true);
    }
    signal(SIGPIPE, old_handler);
  }

  static BrowserController *instance_;
  int child_pid_;
  int down_fd_, up_fd_;
  int up_fd_watch_;
  int ping_timer_watch_;
  bool ping_flag_;
  std::string up_buffer_;
  typedef LightMap<size_t, BrowserElementImpl *> BrowserElements;
  BrowserElements browser_elements_;
  size_t browser_seq_;
  bool removing_watch_;
  int recursion_depth_;
  uint64_t command_start_time_;
  bool first_command_;
};

// Manages the objects of the host side and the browser side for a browser.
class BrowserElementImpl {
 public:
  class BrowserObjectWrapper : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0x1d89f790355540ab, ScriptableInterface);

    BrowserObjectWrapper(BrowserElementImpl *owner,
                         BrowserObjectWrapper *parent, size_t object_id)
        : owner_(owner),
          parent_(parent),
          object_id_(object_id),
          object_id_str_(StringPrintf("%zu", object_id)),
          call_self_(this),
          to_string_(NewSlot(this, &BrowserObjectWrapper::ToString)) {
      if (parent_)
        parent_->Ref();
    }

    virtual ~BrowserObjectWrapper() {
      delete to_string_;
      if (owner_) {
        // Don't send Unref command after the browser is closed.
        owner_->browser_objects_.erase(object_id_);
        owner_->controller_->SendCommand(kUnrefCommand, owner_->browser_id_,
                                         object_id_str_.c_str(), NULL);
      }
      if (parent_)
        parent_->Unref();
    }

    void OnOwnerDestroy() {
      owner_ = NULL;
    }

    virtual PropertyType GetPropertyInfo(const char *name, Variant *prototype) {
      if (!*name) {
        *prototype = Variant(&call_self_);
        return ScriptableInterface::PROPERTY_METHOD;
      }
      if (strcmp(name, "toString") == 0) {
        *prototype = Variant(to_string_);
        return ScriptableInterface::PROPERTY_METHOD;
      }
      *prototype = Variant(Variant::TYPE_VARIANT);
      return ScriptableInterface::PROPERTY_DYNAMIC;
    }

    virtual ResultVariant GetProperty(const char *name) {
      if (!owner_)
        return ResultVariant();
      if (!*name) {
        // Get the default method used to call this object as a function.
        return ResultVariant(Variant(&call_self_));
      }
      if (strcmp(name, "toString") == 0) {
        // Handle toString() locally.
        return ResultVariant(Variant(to_string_));
      }
      if (strcmp(name, "valueOf") == 0) {
        // Use default valueOf().
        return ResultVariant();
      }
      std::string result = owner_->controller_->SendCommand(
          kGetPropertyCommand, owner_->browser_id_, object_id_str_.c_str(),
          EncodeJavaScriptString(name, '"').c_str(), NULL);
      if (!owner_)
        return ResultVariant();
      return owner_->DecodeValue(this, result.c_str(), Variant::TYPE_VARIANT);
    }

    virtual bool SetProperty(const char *name, const Variant &value) {
      if (!owner_)
        return false;
      owner_->controller_->SendCommand(
          kSetPropertyCommand, owner_->browser_id_, object_id_str_.c_str(),
          EncodeJavaScriptString(name, '"').c_str(),
          owner_->EncodeValue(value).c_str(), NULL);
      return owner_ != NULL;
    }

    virtual ResultVariant GetPropertyByIndex(int index) {
      if (!owner_)
        return ResultVariant();
      std::string result = owner_->controller_->SendCommand(
          kGetPropertyCommand, owner_->browser_id_, object_id_str_.c_str(),
          StringPrintf("%d", index).c_str(), NULL);
      if (!owner_)
        return ResultVariant();
      return owner_->DecodeValue(this, result.c_str(), Variant::TYPE_VARIANT);
    }

    virtual bool SetPropertyByIndex(int index, const Variant &value) {
      if (!owner_)
        return false;
      owner_->controller_->SendCommand(
          kSetPropertyCommand, owner_->browser_id_, object_id_str_.c_str(),
          StringPrintf("%d", index).c_str(),
          owner_->EncodeValue(value).c_str(), NULL);
      return owner_ != NULL;
    }

    std::string ToString() {
      return StringPrintf("browser %zu object %zu", owner_->browser_id_,
                          object_id_);
    }

    class CallSelfSlot : public Slot {
     public:
      CallSelfSlot(BrowserObjectWrapper *wrapper)
          : wrapper_(wrapper) {
      }

      virtual bool HasMetadata() const {
        return false;
      }

      virtual ResultVariant Call(ScriptableInterface *object,
                                 int argc, const Variant argv[]) const {
        if (!wrapper_->owner_)
          return ResultVariant();
        std::string buffer(kCallCommand);
        buffer += StringPrintf("\n%zu", wrapper_->owner_->browser_id_);
        buffer += '\n';
        buffer += wrapper_->object_id_str_;
        buffer += '\n';
        if (wrapper_->parent_)
          buffer += wrapper_->parent_->object_id_str_;
        for (int i = 0; i < argc; i++) {
          buffer += '\n';
          buffer += wrapper_->owner_->EncodeValue(argv[i]);
        }
        buffer += kEndOfMessageFull;
        std::string result = wrapper_->owner_->controller_->SendCommandBuffer(
            buffer);
        if (!wrapper_->owner_)
          return ResultVariant();
        return wrapper_->owner_->DecodeValue(NULL, result.c_str(),
                                             Variant::TYPE_VARIANT);
      }

      virtual bool operator==(const Slot &another) const {
        return this == &another;
      }

      BrowserObjectWrapper *wrapper_;
    };

    BrowserElementImpl *owner_;
    BrowserObjectWrapper *parent_;
    size_t object_id_;
    std::string object_id_str_;
    CallSelfSlot call_self_;
    Slot *to_string_;
  };

  // Wraps a host method into a ScriptableInterface object, while doesn't
  // take the ownership of the method slot.
  class HostSlotWrapper : public ScriptableHelperDefault {
   public:
    DEFINE_CLASS_ID(0xc12afe14b57d4e0b, ScriptableInterface);
    HostSlotWrapper(ScriptableInterface *parent, const std::string &name)
        : parent_(parent), name_(name) {
    }

    virtual PropertyType GetPropertyInfo(const char *name,
                                         Variant *prototype) {
      if (!*name) {
        if (prototype)
          *prototype = Variant(GetSlot());
        return PROPERTY_METHOD;
      }
      return PROPERTY_NOT_EXIST;
    }

    virtual ResultVariant GetProperty(const char *name) {
      return ResultVariant(!*name ? Variant(GetSlot()) : Variant());
    }

    Slot *GetSlot() {
      ScriptableInterface *parent_obj = parent_.Get();
      if (parent_obj) {
        ResultVariant method = parent_obj->GetProperty(name_.c_str());
        if (method.v().type() == Variant::TYPE_SLOT)
          return VariantValue<Slot *>()(method.v());
      }
      return NULL;
    }

    ScriptableHolder<ScriptableInterface> parent_;
    std::string name_;
  };

  BrowserElementImpl(BrowserElement *owner)
      : owner_(owner),
        object_seq_(0),
        controller_(BrowserController::get()),
        browser_id_(0),
        content_type_("text/html"),
        socket_(NULL),
        x_(0), y_(0), width_(0), height_(0),
        content_updated_(false),
        minimized_(false), popped_out_(false),
        always_open_new_window_(true),
        minimized_connection_(owner->GetView()->ConnectOnMinimizeEvent(
            NewSlot(this, &BrowserElementImpl::OnViewMinimized))),
        restored_connection_(owner->GetView()->ConnectOnRestoreEvent(
            NewSlot(this, &BrowserElementImpl::OnViewRestored))),
        popout_connection_(owner->GetView()->ConnectOnPopOutEvent(
            NewSlot(this, &BrowserElementImpl::OnViewPoppedOut))),
        popin_connection_(owner->GetView()->ConnectOnPopInEvent(
            NewSlot(this, &BrowserElementImpl::OnViewPoppedIn))),
        dock_connection_(owner->GetView()->ConnectOnDockEvent(
            NewSlot(this, &BrowserElementImpl::OnViewDockUndock))),
        undock_connection_(owner->GetView()->ConnectOnUndockEvent(
            NewSlot(this, &BrowserElementImpl::OnViewDockUndock))) {
  }

  ~BrowserElementImpl() {
    Deactivate();
    minimized_connection_->Disconnect();
    restored_connection_->Disconnect();
    popout_connection_->Disconnect();
    popin_connection_->Disconnect();
    dock_connection_->Disconnect();
    undock_connection_->Disconnect();
  }

  void Deactivate() {
    if (browser_id_) {
      // If socket_ is not a valid socket, the child may have closed the
      // browser by itself, so no need to send the close command.
      controller_->CloseBrowser(browser_id_, GTK_IS_SOCKET(socket_));
      browser_id_ = 0;
    }
    for (BrowserObjectMap::iterator it = browser_objects_.begin();
         it != browser_objects_.end(); ++it) {
      // The browser_objects may still be referenced by host script engine.
      it->second->OnOwnerDestroy();
    }

    if (GTK_IS_WIDGET(socket_)) {
      gtk_widget_destroy(socket_);
      socket_ = NULL;
    }
  }

  void GetWidgetExtents(gint *x, gint *y, gint *width, gint *height) {
    double widget_x0, widget_y0;
    double widget_x1, widget_y1;
    owner_->SelfCoordToViewCoord(0, 0, &widget_x0, &widget_y0);
    owner_->SelfCoordToViewCoord(owner_->GetPixelWidth(),
                                 owner_->GetPixelHeight(),
                                 &widget_x1, &widget_y1);

    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x0, widget_y0,
                                                    &widget_x0, &widget_y0);
    owner_->GetView()->ViewCoordToNativeWidgetCoord(widget_x1, widget_y1,
                                                    &widget_x1, &widget_y1);

    *x = static_cast<gint>(round(widget_x0));
    *y = static_cast<gint>(round(widget_y0));
    *width = static_cast<gint>(ceil(widget_x1 - widget_x0));
    *height = static_cast<gint>(ceil(widget_y1 - widget_y0));
  }

  void EnsureBrowser() {
    if (!browser_id_)
      browser_id_ = controller_->AddBrowserElement(this);
    if (!browser_id_ || GTK_IS_SOCKET(socket_))
      return;

    content_updated_ = content_.empty();
    GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    if (!GTK_IS_FIXED(container)) {
      LOG("BrowserElement needs a GTK_FIXED parent. Actual type: %s",
          G_OBJECT_TYPE_NAME(container));
      return;
    }

    socket_ = gtk_socket_new();
    g_signal_connect_after(socket_, "realize",
                           G_CALLBACK(OnSocketRealize), this);
    g_signal_connect(socket_, "destroy",
                     GTK_SIGNAL_FUNC(gtk_widget_destroyed), &socket_);

    GetWidgetExtents(&x_, &y_, &width_, &height_);
    gtk_fixed_put(GTK_FIXED(container), socket_, x_, y_);
    gtk_widget_set_size_request(socket_, width_, height_);
    gtk_widget_show(socket_);
    gtk_widget_realize(socket_);
  }

  static void OnSocketRealize(GtkWidget *widget, gpointer user_data) {
    BrowserElementImpl *impl = static_cast<BrowserElementImpl *>(user_data);
    if (impl->browser_id_) {
      std::string browser_id_str = StringPrintf("%zu", impl->browser_id_);
      // Convert GdkNativeWindow to intmax_t to ensure the printf format
      // to match the data type and not to loose accuracy.
      std::string socket_id_str = StringPrintf("0x%jx",
          static_cast<intmax_t>(gtk_socket_get_id(GTK_SOCKET(impl->socket_))));
      impl->controller_->SendCommand(kNewBrowserCommand, impl->browser_id_,
                                     socket_id_str.c_str(), NULL);
      impl->UpdateChildContent();
      impl->controller_->SendCommand(kSetAlwaysOpenNewWindowCommand,
                                     impl->browser_id_,
                                     impl->always_open_new_window_ ? "1" : "0",
                                     NULL);
    }
  }

  void UpdateChildContent() {
    if (browser_id_ && !content_updated_) {
      std::string content = EncodeJavaScriptString(content_.c_str(), '"');
      controller_->SendCommand(kSetContentCommand, browser_id_,
                               content_type_.c_str(), content.c_str(), NULL);
      content_updated_ = true;
    }
  }

  void Layout() {
    EnsureBrowser();
    GtkWidget *container = GTK_WIDGET(owner_->GetView()->GetNativeWidget());
    if (GTK_IS_FIXED(container) && GTK_IS_SOCKET(socket_)) {
      bool force_layout = false;
      // check if the contain has changed.
      if (gtk_widget_get_parent(socket_) != container) {
        gtk_widget_reparent(socket_, container);
        force_layout = true;
      }

      gint x, y, width, height;
      GetWidgetExtents(&x, &y, &width, &height);

      if (x != x_ || y != y_ || force_layout) {
        x_ = x;
        y_ = y;
        gtk_fixed_move(GTK_FIXED(container), socket_, x, y);
      }
      if (width != width_ || height != height_ || force_layout) {
        width_ = width;
        height_ = height;
        gtk_widget_set_size_request(socket_, width, height);
      }
      if (owner_->IsReallyVisible() && (!minimized_ || popped_out_))
        gtk_widget_show(socket_);
      else
        gtk_widget_hide(socket_);
    }
  }

  void SetContent(const std::string &content) {
    content_ = content;
    content_updated_ = false;
    if (browser_id_ && GTK_IS_SOCKET(socket_))
      UpdateChildContent();
    // Otherwise the content will be set when the socket is created.
  }

  void OnViewMinimized() {
    // The browser widget must be hidden when the view is minimized.
    if (GTK_IS_SOCKET(socket_) && !popped_out_) {
      gtk_widget_hide(socket_);
    }
    minimized_ = true;
  }

  void OnViewRestored() {
    if (GTK_IS_SOCKET(socket_) && owner_->IsReallyVisible() && !popped_out_) {
      gtk_widget_show(socket_);
    }
    minimized_ = false;
  }

  void OnViewPoppedOut() {
    popped_out_ = true;
    Layout();
  }

  void OnViewPoppedIn() {
    popped_out_ = false;
    Layout();
  }

  void OnViewDockUndock() {
    // The toplevel window might be changed, so it's necessary to reparent the
    // browser widget.
    Layout();
  }

  std::string EncodeValue(const Variant &value) {
    switch (value.type()) {
      case Variant::TYPE_VOID:
        return kUndefinedStr;
      case Variant::TYPE_BOOL:
        return VariantValue<bool>()(value) ? kTrueStr : kFalseStr;
      case Variant::TYPE_INT64:
        return StringPrintf("%jd", VariantValue<int64_t>()(value));
      case Variant::TYPE_DOUBLE:
        return StringPrintf("%g", VariantValue<double>()(value));
      case Variant::TYPE_STRING: {
        const char *str = VariantValue<const char *>()(value);
        if (!str) return kNullStr;
        return EncodeJavaScriptString(str, '"');
      }
      case Variant::TYPE_UTF16STRING: {
        const UTF16Char *str = VariantValue<const UTF16Char *>()(value);
        if (!str) return kNullStr;
        return EncodeJavaScriptString(str, '"');
      }
      case Variant::TYPE_SCRIPTABLE: {
        ScriptableInterface *obj =
            VariantValue<ScriptableInterface *>()(value);
        if (!obj) return kNullStr;
        return StringPrintf("hobj %zu", AddHostObject(obj));
      }
      case Variant::TYPE_SLOT: {
        // Note: this case is not for the result of GetProperty() of a host
        // object which is specially handled in GetHostObjectProperty().
        Slot *slot = VariantValue<Slot *>()(value);
        if (!slot) return kNullStr;
        ScriptableFunction *obj = new ScriptableFunction(slot);
        return StringPrintf("hobj %zu", AddHostObject(obj));
      }
      default:
        return StringPrintf("exception: this value can't be passed to "
            "browser_child: %s", value.Print().c_str());
    }
  }

  // 'parent' is only useful when getting a property of a browser object and
  // the type of the property is still an object.
  ResultVariant DecodeValue(BrowserObjectWrapper *parent,
                            const char *str, Variant::Type expected_type) {
    char first_char = str[0];
    Variant value;
    if (isdigit(first_char) || first_char == '-') {
      value = Variant(strtod(str, NULL));
    } else if (first_char == '"' || first_char == '\'') {
      UTF16String s;
      value = DecodeJavaScriptString(str, &s) ? Variant(s) : Variant();
    } else if (strncmp(str, "wobj ", 5) == 0) {
      size_t object_id = static_cast<size_t>(strtol(str + 5, NULL, 10));
      BrowserObjectWrapper *wrapper = AddOrGetBrowserObject(parent, object_id);
      value = Variant(implicit_cast<ScriptableInterface *>(wrapper));
    } else if (kTrueStr == str) {
      value = Variant(true);
    } else if (kFalseStr == str) {
      value = Variant(false);
    } else if (kNullStr == str) {
      value = Variant(static_cast<ScriptableInterface *>(NULL));
    }

    ResultVariant result(value);
    Variant::Type actual_type = value.type();
    if (expected_type == Variant::TYPE_VARIANT || expected_type == actual_type)
      return result;

    value = Variant();
    // Type mismatch, try to convert.
    switch (expected_type) {
      case Variant::TYPE_VOID:
        // This type is not checked before value parsing, because if the
        // browser returns an object while the host expects nothing, the
        // above 'ResultVariant result' can cleanup the object.
        break;
      case Variant::TYPE_BOOL:
        // Value convert to bool true if the value is a non-empty string,
        // non-zero number or a non-null object.
        value = Variant((actual_type == Variant::TYPE_UTF16STRING &&
                         *VariantValue<const UTF16Char *>()(value)) ||
                        (actual_type == Variant::TYPE_DOUBLE &&
                         VariantValue<double>()(value) != 0) ||
                        (actual_type == Variant::TYPE_SCRIPTABLE &&
                         VariantValue<ScriptableInterface *>()(value) != NULL));
        break;
      case Variant::TYPE_INT64:
        value = Variant(
            actual_type == Variant::TYPE_DOUBLE ?
                static_cast<int64_t>(VariantValue<double>()(value)) : 0);
        break;
      case Variant::TYPE_STRING:
        if (actual_type == Variant::TYPE_UTF16STRING) {
          std::string s;
          ConvertStringUTF16ToUTF8(VariantValue<UTF16String>()(value), &s);
          value = Variant(s);
        } else if (actual_type == Variant::TYPE_SCRIPTABLE &&
                   VariantValue<ScriptableInterface *>()(value) == NULL) {
          value = Variant(static_cast<const char *>(NULL));
        } else {
          // The value is not a string, use its string representation directly.
          value = Variant(str);
        }
        break;
      case Variant::TYPE_UTF16STRING:
        if (actual_type == Variant::TYPE_SCRIPTABLE &&
            VariantValue<ScriptableInterface *>()(value) == NULL) {
          value = Variant(static_cast<const UTF16Char *>(NULL));
        } else {
          // The value is not a string, use its string representation directly.
          UTF16String s;
          ConvertStringUTF8ToUTF16(str, &s);
          value = Variant(s);
        }
        break;
      default:
        LOG("Unsupported conversion from value %s to type %d",
            str, expected_type);
        break;
    }
    return ResultVariant(value);
  }

  size_t AddHostObject(ScriptableInterface *object) {
    ASSERT(object);
    object_seq_++;
    host_objects_[object_seq_].Reset(object);
    return object_seq_;
  }

  void UnrefHostObject(size_t object_id) {
    if (object_id != 0) {
      HostObjectMap::iterator it = host_objects_.find(object_id);
      if (it != host_objects_.end())
        host_objects_.erase(it);
    }
  }

  ScriptableInterface *GetHostObject(const char *object_id_str) {
    size_t object_id = static_cast<size_t>(strtol(object_id_str, NULL, 10));
    if (object_id == 0)
      return external_object_.Get();
    HostObjectMap::iterator it = host_objects_.find(object_id);
    if (it == host_objects_.end())
      return NULL;
    return it->second.Get();
  }

  BrowserObjectWrapper *AddOrGetBrowserObject(BrowserObjectWrapper *parent,
                                              size_t object_id) {
    BrowserObjectMap::iterator it = browser_objects_.find(object_id);
    if (it != browser_objects_.end())
      return it->second;

    BrowserObjectWrapper *wrapper = new BrowserObjectWrapper(this, parent,
                                                             object_id);
    browser_objects_[object_id] = wrapper;
    return wrapper;
  }

  std::string GetHostObjectProperty(const char *object_id_str,
                                    const char *property) {
    ScriptableInterface *object = GetHostObject(object_id_str);
    if (!object)
      return StringPrintf("exception: host object %s not found", object_id_str);
    std::string property_name;
    ResultVariant result;
    if (DecodeJavaScriptString(property, &property_name))
      result = object->GetProperty(property_name.c_str());
    else
      result = object->GetPropertyByIndex(atoi(property));

    if (result.v().type() == Variant::TYPE_SLOT) {
      // Specially handle Slot property, don't wrap it into a
      // ScriptableFunction because it is owned by the object.
      return EncodeValue(Variant(new HostSlotWrapper(object, property_name)));
    } else {
      return EncodeValue(result.v());
    }
  }

  std::string SetHostObjectProperty(const char *object_id_str,
                                    const char *property, const char *value) {
    ScriptableInterface *object = GetHostObject(object_id_str);
    if (!object)
      return StringPrintf("exception: host object %s not found", object_id_str);
    std::string property_name;
    if (DecodeJavaScriptString(property, &property_name)) {
      Variant prototype(Variant::TYPE_VARIANT);
      object->GetPropertyInfo(property_name.c_str(), &prototype);
      ResultVariant v = DecodeValue(NULL, value, prototype.type());
      object->SetProperty(property_name.c_str(), v.v());
    } else {
      ResultVariant v = DecodeValue(NULL, value, Variant::TYPE_VARIANT);
      object->SetPropertyByIndex(atoi(property), v.v());
    }
    return "";
  }

  std::string CallHostObject(size_t param_count, const char **params) {
    ScriptableInterface *object = GetHostObject(params[2]);
    if (!object)
      return StringPrintf("exception: host object %s not found", params[2]);
    ScriptableInterface *this_object = GetHostObject(params[3]);

    Variant method = object->GetProperty("").v();
    if (method.type() != Variant::TYPE_SLOT) {
      return StringPrintf(
          "exception: host object %s can't be called as a function", params[2]);
    }

    Slot *slot = VariantValue<Slot *>()(method);
    if (!slot)
      return "";

    size_t argc = param_count - 4;
    int expected_argc = static_cast<int>(argc);
    const Variant::Type *arg_types = NULL;
    if (slot->HasMetadata()) {
      // TODO: for now we don't support vararg slots (GetArgCount() returns
      // MAX_INT), and slots with default args.
      expected_argc = slot->GetArgCount();
      arg_types = slot->GetArgTypes();
    }
    if (expected_argc != static_cast<int>(argc))
      return "exception: Wrong number of arguments";

    Variant *argv = new Variant[argc];
    ResultVariant *holders = new ResultVariant[argc];
    for (size_t i = 0; i < argc; ++i) {
      holders[i] = DecodeValue(NULL, params[i + 4],
          arg_types ? arg_types[i] : Variant::TYPE_VARIANT);
      argv[i] = holders[i].v();
    }

    ResultVariant result = slot->Call(this_object, expected_argc, argv);
    delete [] argv;
    delete [] holders;
    return EncodeValue(result.v());
  }

  bool OpenURL(const char *url) {
    GadgetInterface *gadget = owner_->GetView()->GetGadget();
    bool result = false;
    if (gadget) {
      // Let the gadget allow this OpenURL gracefully.
      bool old_interaction = gadget->SetInUserInteraction(true);
      result = gadget->OpenURL(url);
      gadget->SetInUserInteraction(old_interaction);
    }
    return result;
  }

  std::string ProcessFeedback(size_t param_count, const char **params) {
    std::string result;
    const char *type = params[0];
    if (strcmp(type, kGetPropertyFeedback) == 0) {
      if (param_count != 4) {
        LOG("%s feedback needs 4 parameters, but only %zu is given",
            kGetPropertyFeedback, param_count);
      } else {
        result = GetHostObjectProperty(params[2], params[3]);
      }
    } else if (strcmp(type, kSetPropertyFeedback) == 0) {
      if (param_count != 5) {
        LOG("%s feedback needs 5 parameters, but only %zu is given",
            kSetPropertyFeedback, param_count);
      } else {
        SetHostObjectProperty(params[2], params[3], params[4]);
      }
    } else if (strcmp(type, kCallFeedback) == 0) {
      if (param_count < 4) {
        LOG("%s feedback needs at least 4 parameters, but only %zu is given",
            kCallFeedback, param_count);
      } else {
        result = CallHostObject(param_count, params);
      }
    } else if (strcmp(type, kUnrefFeedback) == 0) {
      if (param_count != 3) {
        LOG("%s feedback needs 3 parameters, but only %zu is given",
            kUnrefFeedback, param_count);
      } else {
        UnrefHostObject(static_cast<size_t>(strtol(params[2], NULL, 10)));
      }
    } else if (strcmp(type, kOpenURLFeedback) == 0) {
      if (param_count != 3) {
        LOG("%s feedback needs 3 parameters, but only %zu is given",
            kOpenURLFeedback, param_count);
      } else {
        result = ongotourl_signal_(params[2], true) || OpenURL(params[2]) ?
            '1' : '0';
      }
    } else if (strcmp(type, kGoToURLFeedback) == 0) {
      if (param_count != 3) {
        LOG("%s feedback needs 3 parameters, but only %zu is given",
            kGoToURLFeedback, param_count);
      } else {
        result = ongotourl_signal_(params[2], false) ? '1' : '0';
      }
    } else if (strcmp(type, kNetErrorFeedback) == 0) {
      if (param_count != 3) {
        LOG("%s feedback needs 3 parameters, but only %zu is given",
            kNetErrorFeedback, param_count);
      } else {
        result = onerror_signal_(params[2]) ? '1' : '0';
      }
    } else {
      LOG("Unknown feedback: %s", type);
    }
    return result;
  }

  void SetAlwaysOpenNewWindow(bool always_open_new_window) {
    if (always_open_new_window_ != always_open_new_window) {
      always_open_new_window_ = always_open_new_window;
      if (browser_id_) {
        controller_->SendCommand(kSetAlwaysOpenNewWindowCommand, browser_id_,
                                 always_open_new_window ? "1" : "0", NULL);
      }
    }
  }

  typedef LightMap<size_t, ScriptableHolder<ScriptableInterface> >
      HostObjectMap;
  typedef LightMap<size_t, BrowserObjectWrapper *> BrowserObjectMap;
  HostObjectMap host_objects_;
  BrowserObjectMap browser_objects_;

  BrowserElement *owner_;
  size_t object_seq_;
  BrowserController *controller_;
  size_t browser_id_;

  std::string content_type_;
  std::string content_;
  GtkWidget *socket_;
  gint x_, y_, width_, height_;
  bool content_updated_ : 1;
  bool minimized_ : 1;
  bool popped_out_ : 1;
  bool always_open_new_window_ : 1;
  ScriptableHolder<ScriptableInterface> external_object_;
  Connection *minimized_connection_, *restored_connection_,
             *popout_connection_, *popin_connection_,
             *dock_connection_, *undock_connection_;
  Signal2<bool, const char *, bool> ongotourl_signal_;
  Signal1<bool, const char *> onerror_signal_;
};

BrowserController *BrowserController::instance_ = NULL;

void BrowserController::DestroyAllBrowsers() {
  while (!browser_elements_.empty())
    browser_elements_.begin()->second->Deactivate();
}

void BrowserController::ProcessFeedback(size_t param_count,
                                        const char **params) {
  if (param_count == 1 && strcmp(params[0], kPingFeedback) == 0) {
    Write(down_fd_, kPingAckFull, kPingAckFullLength);
    ping_flag_ = true;
  } else if (param_count < 2) {
    LOG("No enough feedback parameters");
  } else {
    size_t id = static_cast<size_t>(strtol(params[1], NULL, 0));
    BrowserElements::const_iterator it = browser_elements_.find(id);
    if (it != browser_elements_.end()) {
      std::string result(kReplyPrefix);
      result += it->second->ProcessFeedback(param_count, params);
      DLOG("ProcessFeedback: %zu %s(%.20s,%.20s,%.20s,%.20s,%.20s,%.20s) "
           "result: %.40s...", param_count, params[0],
           param_count > 1 ? params[1] : "", param_count > 2 ? params[2] : "",
           param_count > 3 ? params[3] : "", param_count > 4 ? params[4] : "",
           param_count > 5 ? params[5] : "", param_count > 6 ? params[6] : "",
           result.c_str());
      result += '\n';
      Write(down_fd_, result.c_str(), result.size());
    } else {
      LOG("Invalid browser id: %s", params[1]);
    }
  }
}

BrowserElement::BrowserElement(View *view, const char *name)
    : BasicElement(view, "browser", name, true),
      impl_(new BrowserElementImpl(this)) {
  SetEnabled(true);
}

void BrowserElement::DoClassRegister() {
  BasicElement::DoClassRegister();
  RegisterProperty("contentType",
                   NewSlot(&BrowserElement::GetContentType),
                   NewSlot(&BrowserElement::SetContentType));
  RegisterProperty("innerText", NULL,
                   NewSlot(&BrowserElement::SetContent));
  RegisterProperty("external", NULL,
                   NewSlot(&BrowserElement::SetExternalObject));
  RegisterProperty("alwaysOpenNewWindow",
                   NewSlot(&BrowserElement::IsAlwaysOpenNewWindow),
                   NewSlot(&BrowserElement::SetAlwaysOpenNewWindow));
  RegisterClassSignal("onerror", &BrowserElementImpl::onerror_signal_,
                      &BrowserElement::impl_);
  RegisterClassSignal("ongotourl", &BrowserElementImpl::ongotourl_signal_,
                      &BrowserElement::impl_);
}

BrowserElement::~BrowserElement() {
  delete impl_;
  impl_ = NULL;
}

std::string BrowserElement::GetContentType() const {
  return impl_->content_type_;
}

void BrowserElement::SetContentType(const char *content_type) {
  impl_->content_type_ = content_type && *content_type ? content_type :
                         "text/html";
}

void BrowserElement::SetContent(const std::string &content) {
  impl_->SetContent(content);
}

void BrowserElement::SetExternalObject(ScriptableInterface *object) {
  impl_->external_object_.Reset(object);
}

void BrowserElement::Layout() {
  BasicElement::Layout();
  impl_->Layout();
}

void BrowserElement::DoDraw(CanvasInterface *canvas) {
}

BasicElement *BrowserElement::CreateInstance(View *view, const char *name) {
  return new BrowserElement(view, name);
}

bool BrowserElement::IsAlwaysOpenNewWindow() const {
  return impl_->always_open_new_window_;
}

void BrowserElement::SetAlwaysOpenNewWindow(bool always_open_new_window) {
  impl_->SetAlwaysOpenNewWindow(always_open_new_window);
}

} // namespace gtkmoz
} // namespace ggadget
