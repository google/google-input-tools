#ifndef GGADGET_QTWEBKIT_BROWSER_ELEMENT_INTERNAL_H__
#define GGADGET_QTWEBKIT_BROWSER_ELEMENT_INTERNAL_H__

#include <cmath>
#include <string>
#if QT_VERSION < 0x040400
#include <qwebnetworkinterface.h>
#else
#include <QtNetwork/QNetworkRequest>
#endif
#include <ggadget/gadget_interface.h>
#include <ggadget/scriptable_holder.h>

namespace ggadget {
namespace qt {

//class BrowserElement::Impl;

class WebPage : public QWebPage {
  Q_OBJECT
 public:
  WebPage(QObject *parent, BrowserElement::Impl *url_handler)
      : QWebPage(parent), handler_(url_handler) {
#if QT_VERSION >= 0x040400
    connect(this, SIGNAL(linkHovered(const QString &, const QString &,
                                     const QString &)),
            this, SLOT(OnLinkHovered(const QString &, const QString &,
                                     const QString &)));
#else
    connect(this, SIGNAL(hoveringOverLink(const QString &, const QString &,
                                          const QString &)),
            this, SLOT(OnLinkHovered(const QString &, const QString &,
                                     const QString &)));
#endif
  }
 protected:
  virtual QWebPage *createWindow(
#if QT_VERSION >= 0x040400
     WebWindowType type
#endif
   );

 private slots:
  void OnLinkHovered(const QString &link, const QString & title, const QString & textContent ) {
    GGL_UNUSED(title);
    GGL_UNUSED(textContent);
    url_ = link;
  }

 private:
  QString url_;
  BrowserElement::Impl *handler_;
};


class WebView : public QWebView {
  Q_OBJECT
 public:
  WebView(BrowserElement::Impl *owner) : owner_(owner) {
    setPage(new WebPage(this, owner));
#if QT_VERSION >= 0x040400
    page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    connect(this, SIGNAL(linkClicked(const QUrl&)),
            this, SLOT(OnLinkClicked(const QUrl&)));
#endif
  }
  BrowserElement::Impl *owner_;

 public slots:
  void OnParentDestroyed(QObject* obj);
  void OnLinkClicked(const QUrl& url);
};

class BrowserElement::Impl {
 public:
  Impl(BrowserElement *owner)
    : owner_(owner),
      parent_(NULL),
      child_(new WebView(this)),
      content_type_("text/html"),
      minimized_connection_(owner->GetView()->ConnectOnMinimizeEvent(
          NewSlot(this, &Impl::OnViewMinimized))),
      restored_connection_(owner->GetView()->ConnectOnRestoreEvent(
          NewSlot(this, &Impl::OnViewRestored))),
      popout_connection_(owner->GetView()->ConnectOnPopOutEvent(
          NewSlot(this, &Impl::OnViewPopOut))),
      popin_connection_(owner->GetView()->ConnectOnPopInEvent(
          NewSlot(this, &Impl::OnViewPopIn))),
      dock_connection_(owner->GetView()->ConnectOnDockEvent(
          NewSlot(this, &Impl::OnViewChanged))),
      undock_connection_(owner->GetView()->ConnectOnUndockEvent(
          NewSlot(this, &Impl::OnViewChanged))),
      minimized_(false),
      popped_out_(false),
      always_open_new_window_(true) {
  }

  ~Impl() {
    minimized_connection_->Disconnect();
    restored_connection_->Disconnect();
    popout_connection_->Disconnect();
    popin_connection_->Disconnect();
    dock_connection_->Disconnect();
    undock_connection_->Disconnect();
    DLOG("delete browser_element: webview %p, parent %p", child_, parent_);
    if (parent_) {
      parent_->SetChild(NULL);
    }
    delete child_;
  }

  void OpenUrl(const QString &url) const {
    std::string u = url.toStdString();
    GadgetInterface *gadget = owner_->GetView()->GetGadget();
    if (gadget) {
      // Let the gadget allow this OpenURL gracefully.
      bool old_interaction = gadget->SetInUserInteraction(true);
      gadget->OpenURL(u.c_str());
      gadget->SetInUserInteraction(old_interaction);
    }
  }

  void GetWidgetExtents(int *x, int *y, int *width, int *height) {
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

    *x = static_cast<int>(round(widget_x0));
    *y = static_cast<int>(round(widget_y0));
    *width = static_cast<int>(ceil(widget_x1 - widget_x0));
    *height = static_cast<int>(ceil(widget_y1 - widget_y0));
  }

  void Layout() {
    if (minimized_ && !popped_out_)
      return;

    int x, y, w, h;
    GetWidgetExtents(&x, &y, &w, &h);
    child_->setFixedSize(w, h);

    if (!parent_) {
      parent_ = static_cast<QtViewWidget*>(
          owner_->GetView()->GetNativeWidget()
          );
      if (!parent_) return;
      parent_->SetChild(child_);
      QObject::connect(parent_, SIGNAL(destroyed(QObject*)),
                       child_, SLOT(OnParentDestroyed(QObject*)));
    }
    child_->move(x, y);
    child_->show();
  }

  void SetContent(const std::string &content) {
    // DLOG("Content: %s", utf8str.c_str());
    child_->setContent(content.c_str());
  }

  void OnViewMinimized() {
    if (child_)
      child_->hide();
    minimized_ = true;
  }

  void OnViewRestored() {
    if (child_ && parent_)
      child_->show();
    minimized_ = false;
  }

  void OnViewPopIn() {
    popped_out_ = false;
    OnViewChanged();
  }

  void OnViewPopOut() {
    popped_out_ = true;
    OnViewChanged();
  }

  void OnViewChanged() {
    if (parent_) {
      child_->hide();
      parent_->SetChild(NULL);
      parent_ = NULL;
    }
  }

  void SetAlwaysOpenNewWindow(bool value) {
    always_open_new_window_ = value;
#if QT_VERSION >= 0x040400
    if (always_open_new_window_) {
      child_->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    } else {
      child_->page()->setLinkDelegationPolicy(QWebPage::DontDelegateLinks);
    }
#endif
  }

 public:
  BrowserElement *owner_;
  QtViewWidget *parent_;
  QWebView *child_;
  std::string content_type_;
  std::string content_;
  ScriptableHolder<ScriptableInterface> external_object_;
  Connection *minimized_connection_, *restored_connection_,
             *popout_connection_, *popin_connection_,
             *dock_connection_, *undock_connection_;
  bool minimized_ : 1;
  bool popped_out_ : 1;
  bool always_open_new_window_ : 1;
};

QWebPage *WebPage::createWindow(
#if QT_VERSION >= 0x040400
     WebWindowType
#endif
     ) {
  handler_->OpenUrl(url_);
  return NULL;
}

void WebView::OnParentDestroyed(QObject *obj) {
  if (owner_->parent_ == obj) {
    DLOG("Parent widget %p destroyed, this %p", obj, this);
    owner_->parent_ = NULL;
  }
}

void WebView::OnLinkClicked(const QUrl& url) {
  DLOG("LinkClicked: %s", url.toString().toStdString().c_str());
  owner_->OpenUrl(url.toString());
}

}
}
#endif
