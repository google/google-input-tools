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
#include <vector>
#include <string>

#ifdef _DEBUG
#include <QtCore/QTextStream>
#endif
#include <QtCore/QUrl>
#include <QtCore/QRegExp>
#include <QtNetwork/QNetworkProxy>
#include <QtNetwork/QHttpHeader>

#if QT_VERSION >= 0x040400
#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkCookieJar>
#endif

#include <ggadget/gadget_consts.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/logger.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/signals.h>
#include <ggadget/string_utils.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_http_request_utils.h>
#include <ggadget/xml_dom_interface.h>
#include <ggadget/xml_parser_interface.h>
#include "qt_xml_http_request_internal.h"

namespace ggadget {
namespace qt {

static const int kMaxRedirectTimes = 5;

static const Variant kOpenDefaultArgs[] = {
  Variant(), Variant(),
  Variant(true),
  Variant(static_cast<const char *>(NULL)),
  Variant(static_cast<const char *>(NULL))
};

static const Variant kSendDefaultArgs[] = { Variant("") };

#if QT_VERSION >= 0x040400
// Cookie support is incomplete. Cookies are not persistent after application
// exits.
static QNetworkCookieJar* cookie_jar = NULL;

static void RestoreCookie(const QUrl& url, QHttpRequestHeader *header) {
  QList<QNetworkCookie> list = cookie_jar->cookiesForUrl(url);
  QStringList value;
  foreach (QNetworkCookie cookie, list) {
    value << cookie.toRawForm(QNetworkCookie::NameAndValueOnly);
  }
  if (!value.isEmpty()) {
    header->setValue("Cookie", value.join("; "));
    DLOG("Cookie:%s", value.join("; ").toStdString().c_str());
  }
}

static void SaveCookie(const QUrl& url, const QHttpResponseHeader& header) {
  QStringList list = header.allValues("Set-Cookie");
  if (list.size() != 0) DLOG("Get Cookie Line: %d", list.size());
  foreach (QString item, list) {
    QList<QNetworkCookie> cookies =
        QNetworkCookie::parseCookies(item.toAscii());
    cookie_jar->setCookiesFromUrl(cookies, url);
  }
}
#else
static void RestoreCookie(const QUrl& url, QHttpRequestHeader *header);
static void SaveCookie(const QUrl& url, const QHttpResponseHeader& header);
#endif

class XMLHttpRequest : public QObject,
                       public ScriptableHelper<XMLHttpRequestInterface> {
 public:
  DEFINE_CLASS_ID(0xa34d00e04d0acfbb, XMLHttpRequestInterface);

  XMLHttpRequest(QObject *parent, MainLoopInterface *main_loop,
                 XMLParserInterface *xml_parser,
                 const QString &default_user_agent)
      : QObject(parent),
        main_loop_(main_loop),
        xml_parser_(xml_parser),
        default_user_agent_(default_user_agent),
        http_(NULL),
        request_header_(NULL),
        send_data_(NULL),
        async_(false),
        no_cookie_(false),
        state_(UNSENT),
        send_flag_(false),
        redirected_times_(0),
        status_(0),
        succeeded_(false),
        response_dom_(NULL) {
    VERIFY_M(EnsureXHRBackoffOptions(main_loop->GetCurrentTime()),
             ("Required options module have not been loaded"));
  }

  ~XMLHttpRequest() {
    Abort();
  }

  virtual void DoClassRegister() {
    RegisterClassSignal("onreadystatechange",
                       &XMLHttpRequest::onreadystatechange_signal_);
    RegisterProperty("readyState",
                     NewSlot(&XMLHttpRequest::GetReadyState), NULL);
    RegisterMethod("open",
        NewSlotWithDefaultArgs(NewSlot(&XMLHttpRequest::ScriptOpen),
                               kOpenDefaultArgs));
    RegisterMethod("setRequestHeader",
                   NewSlot(&XMLHttpRequest::ScriptSetRequestHeader));
    RegisterMethod("send",
        NewSlotWithDefaultArgs(NewSlot(&XMLHttpRequest::ScriptSend),
                               kSendDefaultArgs));
    RegisterMethod("abort", NewSlot(&XMLHttpRequest::Abort));
    RegisterMethod("getAllResponseHeaders",
                   NewSlot(&XMLHttpRequest::ScriptGetAllResponseHeaders));
    RegisterMethod("getResponseHeader",
                   NewSlot(&XMLHttpRequest::ScriptGetResponseHeader));
    RegisterProperty("responseStream",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseBody),
                     NULL);
    RegisterProperty("responseBody",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseBody),
                     NULL);
    RegisterProperty("responseText",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseText),
                     NULL);
    RegisterProperty("responseXML",
                     NewSlot(&XMLHttpRequest::ScriptGetResponseXML),
                     NULL);
    RegisterProperty("status", NewSlot(&XMLHttpRequest::ScriptGetStatus),
                     NULL);
    RegisterProperty("statusText",
                     NewSlot(&XMLHttpRequest::ScriptGetStatusText), NULL);
  }

  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler) {
    return onreadystatechange_signal_.Connect(handler);
  }

  virtual State GetReadyState() {
    return state_;
  }

  bool ChangeState(State new_state) {
    DLOG("XMLHttpRequest: ChangeState from %d to %d this=%p",
         state_, new_state, this);
    state_ = new_state;
    onreadystatechange_signal_();
    // ChangeState may re-entered during the signal, so the current state_
    // may be different from the input parameter.
    return state_ == new_state;
  }

  // The maximum data size of this class can process.
  static const size_t kMaxDataSize = 8 * 1024 * 1024;

  static bool CheckSize(size_t current, size_t num_blocks, size_t block_size) {
    return current < kMaxDataSize && block_size > 0 &&
           (kMaxDataSize - current) / block_size > num_blocks;
  }

  ExceptionCode OpenInternal(const char *url) {
    QUrl qurl(url);
    if (!qurl.isValid()) return SYNTAX_ERR;

    QHttp::ConnectionMode mode;

    if (qurl.scheme().toLower() == "https")
      mode = QHttp::ConnectionModeHttps;
    else if (qurl.scheme().toLower() == "http")
      mode = QHttp::ConnectionModeHttp;
    else
      return SYNTAX_ERR;

    if (!qurl.userName().isEmpty() || !qurl.password().isEmpty()) {
      // GDWin Compatibility.
      DLOG("Username:password in URL is not allowed: %s", url);
      return SYNTAX_ERR;
    }

    url_ = url;
    host_ = qurl.host().toStdString();
    if (http_) http_->deleteLater();
    http_ = new MyHttp(qurl.host(), mode, this);
    http_->setUser(user_, password_);

    std::string path = "/";
    size_t sep = url_.find('/', qurl.scheme().length() + strlen("://"));
    if (sep != std::string::npos) path = url_.substr(sep);

    QHttpRequestHeader *header = new QHttpRequestHeader(method_, path.c_str());
    if (!default_user_agent_.isEmpty())
      header->setValue("User-Agent", default_user_agent_);

    // if request_header_ is not null, it's the original header before
    // redirect. Its header value should be copied to new header.
    if (request_header_) {
      QList<QPair<QString, QString> > values = request_header_->values();
      for (int i = 0; i < values.size(); i++) {
        header->setValue(values[i].first, values[i].second);
      }
      delete request_header_;
    }
    header->setValue("Host", host_.c_str());
    request_header_ = header;
    DLOG("HOST: %s, PATH: %s", host_.c_str(), path.c_str());
    return NO_ERR;
  }

  virtual ExceptionCode Open(const char *method, const char *url, bool async,
                             const char *user, const char *password) {
    DLOG("Open %s with %s", url, method);
    Abort();
    redirected_times_ = 0;

    if (strcasecmp(method, "HEAD") != 0 && strcasecmp(method, "GET") != 0
        && strcasecmp(method, "POST") != 0) {
      LOG("XMLHttpRequest: Unsupported method: %s", method);
      return SYNTAX_ERR;
    }
    method_ = method;
    async_ = async;
    user_ = user;
    password_ = password;
    ExceptionCode code = OpenInternal(url);
    if (code != NO_ERR) return code;
    ChangeState(OPENED);
    return NO_ERR;
  }

  virtual ExceptionCode SetRequestHeader(const char *header,
                                         const char *value) {
    if (!header)
      return NULL_POINTER_ERR;
    if (state_ != OPENED || send_flag_) {
      LOG("XMLHttpRequest: SetRequestHeader: Invalid state: %d", state_);
      return INVALID_STATE_ERR;
    }

    if (!IsValidHTTPToken(header)) {
      LOG("XMLHttpRequest::SetRequestHeader: Invalid header %s", header);
      return SYNTAX_ERR;
    }

    if (!IsValidHTTPHeaderValue(value)) {
      LOG("XMLHttpRequest::SetRequestHeader: Invalid value: %s", value);
      return SYNTAX_ERR;
    }

    if (IsForbiddenHeader(header)) {
      DLOG("XMLHttpRequest::SetRequestHeader: Forbidden header %s", header);
      return NO_ERR;
    }

    // strcasecmp shall be used, but it'll break gmail gadget.
    // Microsoft XHR is also case sensitive.
    if (strcmp(header, "Cookie") == 0 &&
        value && strcasecmp(value, "none") == 0) {
      // Microsoft XHR hidden feature: setRequestHeader('Cookie', 'none')
      // clears all cookies. Some gadgets (e.g. reader) use this.
      no_cookie_ = true;
      return NO_ERR;
    }

    if (IsUniqueHeader(header))
      request_header_->setValue(header, value);
    else
      request_header_->addValue(header, value);
    return NO_ERR;
  }

  virtual ExceptionCode Send(const std::string &data) {
    if (state_ != OPENED || send_flag_) {
      LOG("XMLHttpRequest: Send: Invalid state: %d", state_);
      return INVALID_STATE_ERR;
    }

    if (!CheckSize(data.size(), 0, 512)) {
      LOG("XMLHttpRequest: Send: Size too big: %zu", data.size());
      return SYNTAX_ERR;
    }

    // As described in the spec, here don't change the state, but send
    // an event for historical reasons.
    if (!ChangeState(OPENED))
      return INVALID_STATE_ERR;

    send_flag_ = true;
    if (async_) {
      // Do backoff checking to avoid DDOS attack to the server.
      if (!IsXHRBackoffRequestOK(main_loop_->GetCurrentTime(),
                                 host_.c_str())) {
        Abort();
        // Don't raise exception here because async callers might not expect
        // this kind of exception.
        return NO_ERR;
      }
      // Add an internal reference when this request is working to prevent
      // this object from being GC'ed.
      Ref();
      if (!no_cookie_) RestoreCookie(QUrl(url_.c_str()), request_header_);

      if (data.size()) {
        send_data_ = new QByteArray(data.c_str(),
                                    static_cast<int>(data.size()));
        http_->request(*request_header_, *send_data_);
      } else {
        http_->request(*request_header_);
      }
    } else {
      // QtXmlHttpRequest doesn't support Sync mode XHR.
      return NETWORK_ERR;
    }
    return NO_ERR;
  }

  virtual ExceptionCode Send(const DOMDocumentInterface *data) {
    return Send(data ? data->GetXML() : std::string());
  }

  void Done(bool aborting, bool succeeded) {
    bool save_send_flag = send_flag_;
    bool save_async = async_;
    // Set send_flag_ to false early, to prevent problems when Done() is
    // re-entered.
    send_flag_ = false;
    succeeded_ = succeeded;
    if (!succeeded) {
      response_body_.clear();
      response_headers_.clear();
      response_headers_map_.clear();
      response_text_.clear();
    }

    bool no_unexpected_state_change = true;
    if ((state_ == OPENED && save_send_flag) ||
        state_ == HEADERS_RECEIVED || state_ == LOADING) {
      uint64_t now = main_loop_->GetCurrentTime();
      if (!aborting &&
          XHRBackoffReportResult(now, host_.c_str(), status_)) {
        SaveXHRBackoffData(now);
      }
      // The caller may call Open() again in the OnReadyStateChange callback,
      // which may cause Done() re-entered.
      no_unexpected_state_change = ChangeState(DONE);
    }

    if (aborting && no_unexpected_state_change) {
      // Don't dispatch this state change event, according to the spec.
      state_ = UNSENT;
    }

    if (save_send_flag && save_async) {
      // Remove the internal reference that was added when the request was
      // started.
      Unref();
    }
  }

  void FreeResource() {
    delete request_header_;
    request_header_ = NULL;
    delete send_data_;
    send_data_ = NULL;
    if (http_) http_->deleteLater();
    http_ = NULL;

    response_headers_.clear();
    response_headers_map_.clear();
    response_body_.clear();
    response_text_.clear();
    status_ = 0;
    status_text_.clear();
    if (response_dom_) {
      response_dom_->Unref();
      response_dom_ = NULL;
    }
  }

  virtual void Abort() {
    FreeResource();
    Done(true, false);
  }

  virtual ExceptionCode GetAllResponseHeaders(const std::string **result) {
    ASSERT(result);
    if (state_ == LOADING || state_ == DONE) {
      *result = &response_headers_;
      return NO_ERR;
    }

    *result = NULL;
    LOG("XMLHttpRequest: GetAllResponseHeaders: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseHeader(const char *header,
                                          const std::string **result) {
    ASSERT(result);
    if (!header)
      return NULL_POINTER_ERR;

    *result = NULL;
    if (state_ == LOADING || state_ == DONE) {
      CaseInsensitiveStringMap::const_iterator it = response_headers_map_.find(
          header);
      if (it != response_headers_map_.end())
        *result = &it->second;
      return NO_ERR;
    }
    LOG("XMLHttpRequest: GetRequestHeader: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  void DecodeResponseText() {
    std::string encoding;
    xml_parser_->ConvertContentToUTF8(response_body_, url_.c_str(),
                                      response_content_type_.c_str(),
                                      response_encoding_.c_str(),
                                      kEncodingFallback,
                                      &encoding, &response_text_);
  }

  void ParseResponseToDOM() {
    std::string encoding;
    response_dom_ = xml_parser_->CreateDOMDocument();
    response_dom_->Ref();
    if (!xml_parser_->ParseContentIntoDOM(response_body_, NULL, url_.c_str(),
                                          response_content_type_.c_str(),
                                          response_encoding_.c_str(),
                                          kEncodingFallback,
                                          response_dom_,
                                          &encoding, &response_text_) ||
        !response_dom_->GetDocumentElement()) {
      response_dom_->Unref();
      response_dom_ = NULL;
    }
  }

  virtual ExceptionCode GetResponseText(std::string *result) {
    ASSERT(result);

    if (state_ == LOADING) {
      // Though the spec allows getting responseText while loading, we can't
      // afford this because we rely on XML/HTML parser to get the encoding.
      *result = "";
      return NO_ERR;
    } else if (state_ == DONE) {
      if (response_text_.empty() && !response_body_.empty())
        DecodeResponseText();

      *result = response_text_;
      return NO_ERR;
    }

    result->clear();
    LOG("XMLHttpRequest: GetResponseText: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseBody(std::string *result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = response_body_;
      return NO_ERR;
    }

    result->clear();
    LOG("XMLHttpRequest: GetResponseBody: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseXML(DOMDocumentInterface **result) {
    ASSERT(result);

    if (state_ == DONE) {
      if (!response_dom_ && !response_body_.empty())
        ParseResponseToDOM();

      *result = response_dom_;
      return NO_ERR;
    }

    result = NULL;
    LOG("XMLHttpRequest: GetResponseXML: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatus(unsigned short *result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = status_;
      return NO_ERR;
    }

    *result = 0;
    LOG("XMLHttpRequest: GetStatus: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatusText(const std::string **result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = &status_text_;
      return NO_ERR;
    }

    *result = NULL;
    LOG("XMLHttpRequest: GetStatusText: Invalid state: %d", state_);
    return INVALID_STATE_ERR;
  }

  virtual bool IsSuccessful() {
    return succeeded_;
  }

  virtual std::string GetEffectiveUrl() {
    // TODO:
    return url_;
  }

  virtual std::string GetResponseContentType() {
    return response_content_type_;
  }

  virtual Connection *ConnectOnDataReceived(
      Slot2<size_t, const void *, size_t> *receiver) {
    return ondatareceived_signal_.Connect(receiver);
  }

  // Used in the methods for script to throw an script exception on errors.
  bool CheckException(ExceptionCode code) {
    if (code != NO_ERR) {
      DLOG("XMLHttpRequest: Set pending exception: %d this=%p", code, this);
      SetPendingException(new XMLHttpRequestException(code));
      return false;
    }
    return true;
  }

  void ScriptOpen(const char *method, const char *url, bool async,
                  const char *user, const char *password) {
    CheckException(Open(method, url, async, user, password));
  }

  void ScriptSetRequestHeader(const char *header, const char *value) {
    CheckException(SetRequestHeader(header, value));
  }

  void ScriptSend(const Variant &v_data) {
    std::string data;
    if (v_data.ConvertToString(&data)) {
      CheckException(Send(data));
    } else if (v_data.type() == Variant::TYPE_SCRIPTABLE) {
      ScriptableInterface *scriptable =
          VariantValue<ScriptableInterface *>()(v_data);
      if (!scriptable) {
        CheckException(Send(std::string()));
      } else if (scriptable->IsInstanceOf(DOMDocumentInterface::CLASS_ID)) {
        CheckException(Send(down_cast<DOMDocumentInterface *>(scriptable)));
      } else if (scriptable->IsInstanceOf(ScriptableBinaryData::CLASS_ID)) {
        CheckException(
            Send(down_cast<ScriptableBinaryData *>(scriptable)->data()));
      } else {
        CheckException(SYNTAX_ERR);
      }
    } else {
      CheckException(SYNTAX_ERR);
    }
  }

  Variant ScriptGetAllResponseHeaders() {
    const std::string *result = NULL;
    CheckException(GetAllResponseHeaders(&result));
    return result ? Variant(*result) : Variant(static_cast<const char *>(NULL));
  }

  Variant ScriptGetResponseHeader(const char *header) {
    const std::string *result = NULL;
    CheckException(GetResponseHeader(header, &result));
    return result ? Variant(*result) : Variant(static_cast<const char *>(NULL));
  }

  // We can't return std::string here, because the response body may be binary
  // and can't be converted from UTF-8 to UTF-16 by the script adapter.
  ScriptableBinaryData *ScriptGetResponseBody() {
    std::string result;
    if (CheckException(GetResponseBody(&result)) && !result.empty())
      return new ScriptableBinaryData(result);
    return NULL;
  }

  std::string ScriptGetResponseText() {
    std::string result;
    CheckException(GetResponseText(&result));
    return result;
  }

  DOMDocumentInterface *ScriptGetResponseXML() {
    DOMDocumentInterface *result = NULL;
    CheckException(GetResponseXML(&result));
    return result;
  }

  unsigned short ScriptGetStatus() {
    unsigned short result = 0;
    CheckException(GetStatus(&result));
    return result;
  }

  Variant ScriptGetStatusText() {
    const std::string *result = NULL;
    CheckException(GetStatusText(&result));
    return result ? Variant(*result) : Variant(static_cast<const char *>(NULL));
  }

  void OnResponseHeaderReceived(const QHttpResponseHeader &header) {
    status_ = static_cast<unsigned short>(header.statusCode());
    if ((status_ >= 300 && status_ <= 303) || status_ == 307) {
      redirected_url_ = header.value("Location");
    } else {
      response_header_ = header;
      response_headers_ = header.toString().toUtf8().data();
      response_content_type_ = header.contentType().toStdString();
      SplitStatusFromResponseHeaders(&response_headers_, &status_text_);
      ParseResponseHeaders(response_headers_,
                           &response_headers_map_,
                           &response_content_type_,
                           &response_encoding_);

#if _DEBUG
      QTextStream out(stdout);
      out << "Receive Header:"
          << header.contentType() << "\n"
          << header.statusCode() << "\n"
          << header.toString()  << "\n";
#endif

      if (ChangeState(HEADERS_RECEIVED))
        ChangeState(LOADING);
    }
    SaveCookie(QUrl(url_.c_str()), header);
  }

  void OnRequestFinished(int id, bool error) {
    if ((status_ >= 300 && status_ <= 303) || status_ == 307) {
      Redirect();
    } else {
      if (error)
        LOG("Error %s", http_->errorString().toStdString().c_str());
      QByteArray array = http_->readAll();
      response_body_.clear();
      response_body_.append(array.data(), array.length());

      DLOG("responseFinished: %d, %zu, %d",
           id,
           response_body_.length(),
           array.length());
      Done(true, !error);
    }
  }

  // When redirect happens, request_header_, send_data_ will be reused.
  void Redirect() {
    if (redirected_times_ == kMaxRedirectTimes) {
      LOG("Too much redirect, abort this request");
      FreeResource();
      Done(false, false);
      return;
    }
    DLOG("Redirected to %s", redirected_url_.toUtf8().data());
    if (((status_ == 301 || status_ == 302) && method_ == "POST") ||
        status_ == 303) {
      method_ = "GET";
    }
    if (OpenInternal(redirected_url_.toUtf8().data()) != NO_ERR) {
      FreeResource();
      Done(false, false);
    } else {
      redirected_times_++;
      // FIXME(idlecat): What is the right behavior when redirected?
      if (!no_cookie_) RestoreCookie(QUrl(url_.c_str()), request_header_);

      if (send_data_)
        http_->request(*request_header_, *send_data_);
      else
        http_->request(*request_header_);
    }
  }

  MainLoopInterface *main_loop_;
  XMLParserInterface *xml_parser_;
  QString default_user_agent_;
  MyHttp *http_;
  QHttpRequestHeader *request_header_;
  QHttpResponseHeader response_header_;
  QByteArray *send_data_;
  Signal0<void> onreadystatechange_signal_;
  Signal2<size_t, const void *, size_t> ondatareceived_signal_;

  std::string url_, host_;
  bool async_;
  bool no_cookie_;

  State state_;
  // Required by the specification.
  // It will be true after send() is called in async mode.
  bool send_flag_;

  QString redirected_url_;
  int redirected_times_;
  std::string response_headers_;
  std::string response_content_type_;
  std::string response_encoding_;
  unsigned short status_;
  std::string status_text_;
  bool succeeded_;
  std::string response_body_;
  std::string response_text_;
  QString user_;
  QString password_;
  QString method_;
  DOMDocumentInterface *response_dom_;
  CaseInsensitiveStringMap response_headers_map_;
};

void MyHttp::OnResponseHeaderReceived(const QHttpResponseHeader& header) {
  request_->OnResponseHeaderReceived(header);
}

void MyHttp::OnDone(bool error) {
  request_->OnRequestFinished(0, error);
}

static size_t kMaxSessionNumber = 100000;   // An arbitary number
class XMLHttpRequestFactory : public XMLHttpRequestFactoryInterface {
 public:
  XMLHttpRequestFactory() : next_session_id_(1) {
  }

  virtual int CreateSession() {
    if (kMaxSessionNumber < sessions_.size()) return -1;
    while (1) {
      int result = next_session_id_++;
      if (result < 0) result = next_session_id_ = 1;
      if (sessions_.find(result) == sessions_.end()) {
        sessions_[result] = new QObject();
        return result;
      }
    }
  }

  virtual void DestroySession(int session_id) {
    Sessions::iterator it = sessions_.find(session_id);
    if (it != sessions_.end()) {
      QObject *session = it->second;
      delete session;
      sessions_.erase(it);
    } else {
      DLOG("XMLHttpRequestFactory::DestroySession Invalid session: %d",
           session_id);
    }
  }

  virtual XMLHttpRequestInterface *CreateXMLHttpRequest(
      int session_id, XMLParserInterface *parser) {
    if (session_id == 0)
      return new XMLHttpRequest(NULL, GetGlobalMainLoop(), parser,
                                default_user_agent_);

    Sessions::iterator it = sessions_.find(session_id);
    if (it != sessions_.end())
      return new XMLHttpRequest(it->second, GetGlobalMainLoop(), parser,
                                default_user_agent_);

    DLOG("XMLHttpRequestFactory::CreateXMLHttpRequest: "
         "Invalid session: %d", session_id);
    return NULL;
  }

  virtual void SetDefaultUserAgent(const char *user_agent) {
    if (user_agent)
      default_user_agent_ = user_agent;
  }

 private:
  typedef LightMap<int, QObject*> Sessions;
  Sessions sessions_;
  int next_session_id_;
  QString default_user_agent_;
};
} // namespace qt
} // namespace ggadget

#define Initialize qt_xml_http_request_LTX_Initialize
#define Finalize qt_xml_http_request_LTX_Finalize
#define CreateXMLHttpRequest qt_xml_http_request_LTX_CreateXMLHttpRequest

static ggadget::qt::XMLHttpRequestFactory gFactory;

// parse proxy env value, which may look like:
// "http://username:password@yourproxy.com:8080"
static bool ParseProxyEnv(const QString &value, QString *host, quint16 *port,
                          QString *user, QString *password) {
  QRegExp re("(^.*://)?((.+)(:(.+))?@)?([^:]+)(:([0-9]+))?");

  if (re.indexIn(value) == -1) return false;
  *host = re.cap(6);
  if (re.cap(8) != "") {
    *port = static_cast<quint16>(re.cap(8).toInt());
  } else {
    *port = 80;
  }
  *user = re.cap(3);
  *password = re.cap(5);
  return true;
}

extern "C" {
  bool Initialize() {
    LOGI("Initialize qt_xml_http_request extension.");
    const char *proxy_names[] = {
      "all_proxy", "http_proxy", "https_proxy", NULL
    };

    QString host, user, password;
    quint16 port = 0;
    for (int i = 0; proxy_names[i]; i++) {
      const char *env = getenv(proxy_names[i]);
      if (env && ParseProxyEnv(env, &host, &port, &user, &password)) {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(host);
        proxy.setPort(port);
        if (user != "") proxy.setUser(user);
        if (password != "") proxy.setPassword(password);
        QNetworkProxy::setApplicationProxy(proxy);
        DLOG("Using proxy %s:%d", host.toUtf8().data(), port);
        break;
      }
    }
    ggadget::qt::cookie_jar = new QNetworkCookieJar;
    return ggadget::SetXMLHttpRequestFactory(&gFactory);
  }

  void Finalize() {
    LOGI("Finalize qt_xml_http_request extension.");
  }
}
#include "qt_xml_http_request_internal.moc"
