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
#include <cstring>
#include <vector>
#include <libsoup/soup.h>

#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome-features.h>
#endif

#include <ggadget/gadget_consts.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/logger.h>
#include <ggadget/scriptable_binary_data.h>
#include <ggadget/script_context_interface.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/signals.h>
#include <ggadget/string_utils.h>
#include <ggadget/light_map.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_http_request_utils.h>
#include <ggadget/xml_dom_interface.h>
#include <ggadget/xml_parser_interface.h>

#ifdef _DEBUG
// Uncomment it to print verbose log messages.
// #define SOUP_XHR_VERBOSE
#endif

namespace ggadget {
namespace soup {

static const int kMaxRedirections = 10;
static const int kSessionTimeout = 30; // seconds

// The maximum response body size of this class can process.
static const size_t kMaxResponseBodySize = 8 * 1024 * 1024;

static const char kSoupMessageXHRKey[] = "XHR";

static const char *kValidHttpMethods[] = {
  "GET", "HEAD", "POST", "PUT", NULL
};

static const Variant kOpenDefaultArgs[] = {
  Variant(), Variant(),
  Variant(true),
  Variant(static_cast<const char *>(NULL)),
  Variant(static_cast<const char *>(NULL))
};

static const Variant kSendDefaultArgs[] = { Variant("") };

class XMLHttpRequest : public ScriptableHelper<XMLHttpRequestInterface> {
 public:
  DEFINE_CLASS_ID(0x8f8453af7adb4a59, XMLHttpRequestInterface);

  XMLHttpRequest(SoupSession *session, XMLParserInterface *xml_parser)
      : message_(NULL),
        session_(session),
        xml_parser_(xml_parser),
        response_dom_(NULL),
        redirect_count_(0),
        status_(0),
        state_(UNSENT),
        async_(false),
        send_flag_(false),
        succeeded_(false) {
    VERIFY_M(EnsureXHRBackoffOptions(GetGlobalMainLoop()->GetCurrentTime()),
             ("Required options module have not been loaded"));
    g_object_ref(session);
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

  ~XMLHttpRequest() {
    Abort();
    g_object_unref(session_);
  }

  virtual Connection *ConnectOnReadyStateChange(Slot0<void> *handler) {
    return onreadystatechange_signal_.Connect(handler);
  }

  virtual State GetReadyState() {
    return state_;
  }

  bool ChangeState(State new_state) {
#ifdef SOUP_XHR_VERBOSE
    DLOG("%p: ChangeState from %d to %d", this, state_, new_state);
#endif
    state_ = new_state;
    onreadystatechange_signal_();
    // ChangeState may re-entered during the signal, so the current state_
    // may be different from the input parameter.
    return state_ == new_state;
  }

  virtual ExceptionCode Open(const char *method, const char *url, bool async,
                             const char *user, const char *password) {
    Abort();
    if (!method || !url)
      return NULL_POINTER_ERR;

    if (!IsValidWebURL(url)) {
      return SYNTAX_ERR;
    }

    if (!GetUsernamePasswordFromURL(url).empty()) {
      // GDWin Compatibility.
      LOG("%p: Username:password in URL is not allowed: %s", this, url);
      return SYNTAX_ERR;
    }

    url_ = url;
    host_ = GetHostFromURL(url);

    // effective url is same as url_ at beginning.
    effective_url_ = url_;

    for (size_t i = 0; kValidHttpMethods[i]; ++i) {
      if (strcasecmp(method, kValidHttpMethods[i]) == 0) {
        method_ = ToUpper(method);
        break;
      }
    }

    if (method_.empty()) {
      LOG("%p: Unsupported method: %s", this, method);
      return SYNTAX_ERR;
    }

    message_ = soup_message_new(method_.c_str(), url_.c_str());

#ifdef SOUP_XHR_VERBOSE
    DLOG("%p: Open(%s, %s, %d) message:%p", this, method, url, async, message_);
#endif

    g_signal_connect(G_OBJECT(message_), "finished",
                     G_CALLBACK(FinishedCallback), this);
    g_signal_connect(G_OBJECT(message_), "got-chunk",
                     G_CALLBACK(GotChunkCallback), this);
    g_signal_connect(G_OBJECT(message_), "got-headers",
                     G_CALLBACK(GotHeadersCallback), this);
    g_signal_connect(G_OBJECT(message_), "restarted",
                     G_CALLBACK(RestartedCallback), this);

#ifdef SOUP_XHR_VERBOSE
    g_signal_connect(G_OBJECT(message_), "got-body",
                     G_CALLBACK(GotBodyCallback), this);
    g_signal_connect(G_OBJECT(message_), "got-informational",
                     G_CALLBACK(GotInformationalCallback), this);
    g_signal_connect(G_OBJECT(message_), "wrote-body",
                     G_CALLBACK(WroteBodyCallback), this);
    g_signal_connect(G_OBJECT(message_), "wrote-body-data",
                     G_CALLBACK(WroteBodyDataCallback), this);
    g_signal_connect(G_OBJECT(message_), "wrote-chunk",
                     G_CALLBACK(WroteChunkCallback), this);
    g_signal_connect(G_OBJECT(message_), "wrote-headers",
                     G_CALLBACK(WroteHeadersCallback), this);
    g_signal_connect(G_OBJECT(message_), "wrote-informational",
                     G_CALLBACK(WroteInformationalCallback), this);
    g_object_weak_ref(G_OBJECT(message_), MessageDestroyed, this);
#endif

    g_object_set_data(G_OBJECT(message_), kSoupMessageXHRKey, this);

    soup_message_body_set_accumulate(message_->request_body, FALSE);
    soup_message_body_set_accumulate(message_->response_body, FALSE);

    user_ = user ? user : "";
    password_ = password ? password : "";

    async_ = async;
    ChangeState(OPENED);
    return NO_ERR;
  }

  virtual ExceptionCode SetRequestHeader(const char *header,
                                         const char *value) {
    if (state_ != OPENED || send_flag_) {
      LOG("%p: SetRequestHeader: Invalid state: %d", this, state_);
      return INVALID_STATE_ERR;
    }

    // message_ shouldn't be NULL.
    ASSERT(message_);

    if (!IsValidHTTPToken(header)) {
      LOG("%p: SetRequestHeader: Invalid header %s", this, header);
      return SYNTAX_ERR;
    }

    if (!IsValidHTTPHeaderValue(value)) {
      LOG("%p: SetRequestHeader: Invalid value: %s", this, value);
      return SYNTAX_ERR;
    }

    if (IsForbiddenHeader(header)) {
      DLOG("%p: SetRequestHeader: Forbidden header %s", this, header);
      return NO_ERR;
    }

    if (!value || !*value) return NO_ERR;

#ifdef SOUP_XHR_VERBOSE
    DLOG("%p: SetRequestHeader(%s, %s)", this, header, value);
#endif

    SoupMessageHeaders *headers = message_->request_headers;
    if (strcasecmp("Content-Type", header) == 0) {
      soup_message_headers_set_content_type(headers, value, NULL);
    } else if (strcmp("Cookie", header) == 0) {
      // strcasecmp shall be used, but it'll break gmail gadget.
      cookies_.push_back(value);
    } else {
      soup_message_headers_append(headers, header, value);
    }

    return NO_ERR;
  }

  virtual ExceptionCode Send(const std::string &data) {
    if (state_ != OPENED || send_flag_) {
      LOG("%p: Send: Invalid state: %d", this, state_);
      return INVALID_STATE_ERR;
    }

    // message_ shouldn't be NULL.
    ASSERT(message_);

    // As described in the spec, here don't change the state, but send
    // an event for historical reasons.
    if (!ChangeState(OPENED)) {
      return INVALID_STATE_ERR;
    }

    // Do backoff checking to avoid DDOS attack to the server.
    if (!IsXHRBackoffRequestOK(GetGlobalMainLoop()->GetCurrentTime(),
                               host_.c_str())) {
      Abort();
      if (async_) {
        // Don't raise exception here because async callers might not expect
        // this kind of exception.
        ChangeState(DONE);
        return NO_ERR;
      }
      return ABORT_ERR;
    }

#ifdef SOUP_XHR_VERBOSE
    DLOG("%p: Send(%zu, %s)", this, data.size(), method_.c_str());
#endif
    // As described in the spec, if method is GET then discard the data.
    if (method_ != "GET") {
      request_data_ = data;
      soup_message_body_append(message_->request_body, SOUP_MEMORY_STATIC,
                               request_data_.c_str(), request_data_.size());
      if (!soup_message_headers_get_content_type(
          message_->request_headers, NULL)) {
        // Set content type if it's not set yet.
        soup_message_headers_set_content_type(
            message_->request_headers, "application/x-www-form-urlencoded",
            NULL);
      }
      if (!data.size())
        soup_message_headers_set_content_length(message_->request_headers, 0);
    }

    send_flag_ = true;
    // Add an internal reference when this request is working to prevent
    // this object from being GC'ed during the request.
    Ref();
    if (async_) {
      soup_session_queue_message(session_, message_,
                                 MessageCompleteCallback, this);
      // message_ will be destroyed automatically after calling
      // MessageCompleteCallback(), where this->Unref() will be called.
    } else {
      guint result = soup_session_send_message(session_, message_);
      g_object_unref(message_);
      send_flag_ = false;
      message_ = NULL;
      // Remove internal reference.
      Unref();

      if (result == SOUP_STATUS_CANCELLED) {
        return ABORT_ERR;
      } else if (SOUP_STATUS_IS_TRANSPORT_ERROR(result)) {
        return NETWORK_ERR;
      }
    }
    return NO_ERR;
  }

  virtual ExceptionCode Send(const DOMDocumentInterface *data) {
    if (data && message_ && !soup_message_headers_get_content_type(
        message_->request_headers, NULL)) {
      // Set content type if it's not set yet.
      soup_message_headers_set_content_type(
          message_->request_headers, "application/xml;charset=UTF-8", NULL);
    }
    return Send(data ? data->GetXML() : std::string());
  }

  virtual void Abort() {
    CancelMessage(SOUP_STATUS_CANCELLED);
    ClearResponse();
    request_data_.clear();
    status_text_.clear();
    cookies_.clear();
    status_ = 0;
    redirect_count_ = 0;
    succeeded_ = false;

    // Don't dispatch this state change event, according to the spec.
    state_ = UNSENT;
  }

  virtual ExceptionCode GetAllResponseHeaders(const std::string **result) {
    ASSERT(result);
    if (state_ == HEADERS_RECEIVED || state_ == LOADING || state_ == DONE) {
      *result = &response_headers_;
      return NO_ERR;
    }

    *result = NULL;
    LOG("%p: GetAllResponseHeaders: Invalid state: %d", this, state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseHeader(const char *header,
                                          const std::string **result) {
    ASSERT(result);
    if (!header)
      return NULL_POINTER_ERR;

    *result = NULL;
    if (state_ == HEADERS_RECEIVED || state_ == LOADING || state_ == DONE) {
      CaseInsensitiveStringMap::const_iterator it = response_headers_map_.find(
          header);
      if (it != response_headers_map_.end())
        *result = &it->second;
      return NO_ERR;
    }
    LOG("%p: GetRequestHeader: Invalid state: %d", this, state_);
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
    LOG("%p: GetResponseText: Invalid state: %d", this, state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetResponseBody(std::string *result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = response_body_;
      return NO_ERR;
    }

    result->clear();
    LOG("%p: GetResponseBody: Invalid state: %d", this, state_);
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
    LOG("%p: GetResponseXML: Invalid state: %d", this, state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatus(unsigned short *result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = status_;
      return NO_ERR;
    }

    *result = 0;
    LOG("%p: GetStatus: Invalid state: %d", this, state_);
    return INVALID_STATE_ERR;
  }

  virtual ExceptionCode GetStatusText(const std::string **result) {
    ASSERT(result);

    if (state_ == LOADING || state_ == DONE) {
      *result = &status_text_;
      return NO_ERR;
    }

    *result = NULL;
    LOG("%p: GetStatusText: Invalid state: %d", this, state_);
    return INVALID_STATE_ERR;
  }

  virtual bool IsSuccessful() {
    return succeeded_;
  }

  virtual std::string GetEffectiveUrl() {
    return effective_url_;
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
      DLOG("%p: Set pending exception: %d", this, code);
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

 public:
  void Authenticate(SoupAuth *auth) {
    if (user_.length() || password_.length()) {
      soup_auth_authenticate(auth, user_.c_str(), password_.c_str());
    }
  }

  void SetupCookie() {
    if (cookies_.size()) {
      const char *old_cookie_p =
#ifdef HAVE_SOUP_MESSAGE_HEADERS_GET_ONE
          soup_message_headers_get_one(message_->request_headers, "Cookie");
#else
          soup_message_headers_get(message_->request_headers, "Cookie");
#endif

      std::string old_cookie(old_cookie_p ? old_cookie_p : "");

      std::string new_cookie;
      for (StringVector::iterator it = cookies_.begin();
           it != cookies_.end(); ++it) {
        // Keep the same behavior as Windows and curl-xml-http-request.
        if (strcasecmp(it->c_str(), "none") == 0) {
          new_cookie.clear();
          old_cookie.clear();
        } else {
          if (new_cookie.length()) {
            new_cookie.append("; ");
          }
          new_cookie.append(*it);
        }
      }

      if (old_cookie.length()) {
        if (new_cookie.length()) {
            new_cookie.append("; ");
        }
        new_cookie.append(old_cookie);
      }

      if (new_cookie.length()) {
        soup_message_headers_replace(message_->request_headers,
                                     "Cookie", new_cookie.c_str());
      } else {
        soup_message_headers_remove(message_->request_headers, "Cookie");
      }
    }
  }

 public:
#ifdef SOUP_XHR_VERBOSE
  static void PrintMessageInfo(XMLHttpRequest *self, const char *func,
                               SoupMessage *msg, SoupBuffer *chunk) {
    char *url = soup_uri_to_string(soup_message_get_uri(msg), FALSE);
    if (chunk) {
      DLOG("%p: %s: chunk length:%zd url:%s", self, func, chunk->length, url);
    } else {
      DLOG("%p: %s: url:%s", self, func, url);
    }
    DLOG("%p: state:%d  status:%d reason:%s", self, (self ? self->state_ : 0),
         msg->status_code, msg->reason_phrase);
    g_free(url);
  }
#endif

 private:
  void UpdateStatusInfo() {
    if (message_) {
      // If message is cancelled then keep previous status information.
      if (message_->status_code != SOUP_STATUS_CANCELLED) {
        if (SOUP_STATUS_IS_TRANSPORT_ERROR(message_->status_code)) {
          status_ = 0;
        } else {
          status_ = static_cast<unsigned short>(message_->status_code);
        }
        status_text_ = message_->reason_phrase ? message_->reason_phrase : "";
      }
    } else {
      status_ = 0;
      status_text_.clear();
    }
  }

  void CancelMessage(guint status) {
    if (message_) {
      if (send_flag_) {
#ifdef SOUP_XHR_VERBOSE
        DLOG("%p: CancelMessage(%u)", this, status);
#endif
        soup_session_cancel_message(session_, message_, status);
      } else {
        g_object_unref(message_);
      }
    }
  }

  void ClearResponse() {
    response_headers_.clear();
    response_headers_map_.clear();
    response_content_type_.clear();
    response_encoding_.clear();
    response_body_.clear();
    response_text_.clear();
    if (response_dom_) {
      response_dom_->Unref();
      response_dom_ = NULL;
    }
  }

  static void FinishedCallback(SoupMessage *msg, gpointer user_data) {
    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(user_data);
#ifdef SOUP_XHR_VERBOSE
    PrintMessageInfo(request, "FinishedCallback", msg, NULL);
#endif
    ASSERT(request->message_ == msg);

    if ((request->state_ == OPENED && request->send_flag_) ||
        request->state_ == HEADERS_RECEIVED || request->state_ == LOADING) {
      request->UpdateStatusInfo();
      request->succeeded_ = !SOUP_STATUS_IS_TRANSPORT_ERROR(msg->status_code);

      uint64_t now = GetGlobalMainLoop()->GetCurrentTime();
      if (msg->status_code != SOUP_STATUS_CANCELLED && XHRBackoffReportResult(
          now, request->host_.c_str(), request->status_)) {
        SaveXHRBackoffData(now);
      }
      request->ChangeState(DONE);
    }
  }

  static void GotChunkCallback(SoupMessage *msg, SoupBuffer *chunk,
                               gpointer user_data) {
    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(user_data);
#ifdef SOUP_XHR_VERBOSE
    PrintMessageInfo(request, "GotChunkCallback", msg, chunk);
#endif
    ASSERT(request->message_ == msg);
    ASSERT(request->send_flag_);

    bool success = true;
    if (request->state_ == HEADERS_RECEIVED) {
      request->UpdateStatusInfo();
      success = request->ChangeState(LOADING);
    }

    if (success) {
      ASSERT(request->state_ == LOADING);
      if (request->ondatareceived_signal_.HasActiveConnections()) {
        // Only emit ondatareceived_signal_() for correct data.
        if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
          std::string data(chunk->data, chunk->length);
          size_t result = request->ondatareceived_signal_(
              chunk->data, chunk->length);
          success = (result == chunk->length);
        }
      } else {
        request->response_body_.append(chunk->data, chunk->length);
        success = (request->response_body_.size() <= kMaxResponseBodySize);
      }

      if (!success) {
        request->CancelMessage(SOUP_STATUS_CANCELLED);
      }
    }
  }

#ifdef SOUP_XHR_VERBOSE
  static void PrintHeaderItem(const char *name, const char *value,
                              gpointer data) {
    DLOG("  %s : %s", name, value);
  }
#endif

  static void AddResponseHeaderItem(const char *name, const char *value,
                                    gpointer data) {
    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(data);
    if (name && value) {
      request->response_headers_.append(name);
      request->response_headers_.append(": ");
      request->response_headers_.append(value);
      request->response_headers_.append("\r\n");
      std::string old_value = request->response_headers_map_[name];
      if (old_value.length()) {
        old_value.append(", ");
      }
      old_value.append(value);
      request->response_headers_map_[name] = old_value;
    }
  }

  static void GotHeadersCallback(SoupMessage *msg, gpointer user_data) {
    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(user_data);
#ifdef SOUP_XHR_VERBOSE
    PrintMessageInfo(request, "GotHeadersCallback", msg, NULL);
    soup_message_headers_foreach(msg->response_headers, PrintHeaderItem, NULL);
#endif
    ASSERT(request->message_ == msg);
    ASSERT(request->send_flag_);
    ASSERT(request->state_ == OPENED);

    soup_message_headers_foreach(
        msg->response_headers, AddResponseHeaderItem, request);

    GHashTable *params = NULL;
    const char *content_type = soup_message_headers_get_content_type(
        msg->response_headers, &params);

    if (content_type) {
      request->response_content_type_ = content_type;
    }
    if (params) {
      const char *encoding = static_cast<const char *>(
          g_hash_table_lookup(params, "charset"));
      if (encoding) {
        request->response_encoding_ = encoding;
      }
      g_hash_table_destroy(params);
    }

    if (request->state_ == OPENED) {
      request->UpdateStatusInfo();
      request->ChangeState(HEADERS_RECEIVED);
    }
  }

  static void RestartedCallback(SoupMessage *msg, gpointer user_data) {
    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(user_data);
#ifdef SOUP_XHR_VERBOSE
    PrintMessageInfo(request, "RestartedCallback", msg, NULL);
#endif
    ASSERT(request->message_ == msg);
    ASSERT(request->send_flag_);

    if (SOUP_STATUS_IS_REDIRECTION(msg->status_code)) {
      if (++request->redirect_count_ > kMaxRedirections) {
        DLOG("Maximum redirections reached.");
        // Use MALFORMED to distinguish with normal CANCELLED.
        request->CancelMessage(SOUP_STATUS_MALFORMED);
        return;
      }

      // Update effective url only after redirection.
      char *url = soup_uri_to_string(soup_message_get_uri(msg), FALSE);
      if (url) {
        request->effective_url_ = url;
        g_free(url);
      }
    }

    request->ClearResponse();
    request->UpdateStatusInfo();
    request->ChangeState(OPENED);
  }

  static void MessageCompleteCallback(SoupSession *session, SoupMessage *msg,
                                      gpointer user_data) {
    GGL_UNUSED(session);
    GGL_UNUSED(msg);

    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(user_data);
#ifdef SOUP_XHR_VERBOSE
    PrintMessageInfo(request, "MessageCompleteCallback", msg, NULL);
#endif
    ASSERT(request->message_ == msg);
    ASSERT(request->send_flag_);

    // message will be destroyed automatically after calling this callback.
    request->message_ = NULL;
    request->send_flag_ = false;
    // Remove the internal reference that was added when the request was
    // started.
    request->Unref();
  }

#ifdef SOUP_XHR_VERBOSE
  static void MessageDestroyed(gpointer data, GObject *message) {
    DLOG("%p: MessageDestroyed:%p", data, message);
  }

  static void GotBodyCallback(SoupMessage *msg, gpointer user_data) {
    PrintMessageInfo(static_cast<XMLHttpRequest *>(user_data),
                     "GotBodyCallback", msg, NULL);
  }

  static void GotInformationalCallback(SoupMessage *msg, gpointer user_data) {
    PrintMessageInfo(static_cast<XMLHttpRequest *>(user_data),
                     "GotInformationalCallback", msg, NULL);
    soup_message_headers_foreach(msg->response_headers, PrintHeaderItem, NULL);
  }

  static void WroteBodyCallback(SoupMessage *msg, gpointer user_data) {
    PrintMessageInfo(static_cast<XMLHttpRequest *>(user_data),
                     "WroteBodyCallback", msg, NULL);
  }

  static void WroteBodyDataCallback(SoupMessage *msg, SoupBuffer *chunk,
                                    gpointer user_data) {
    PrintMessageInfo(static_cast<XMLHttpRequest *>(user_data),
                     "WroteBodyDataCallback", msg, chunk);
  }

  static void WroteChunkCallback(SoupMessage *msg, gpointer user_data) {
    PrintMessageInfo(static_cast<XMLHttpRequest *>(user_data),
                     "WroteChunkCallback", msg, NULL);
  }

  static void WroteHeadersCallback(SoupMessage *msg, gpointer user_data) {
    PrintMessageInfo(static_cast<XMLHttpRequest *>(user_data),
                     "WroteHeadersCallback", msg, NULL);
    soup_message_headers_foreach(msg->request_headers, PrintHeaderItem, NULL);
  }

  static void WroteInformationalCallback(SoupMessage *msg, gpointer user_data) {
    PrintMessageInfo(static_cast<XMLHttpRequest *>(user_data),
                     "WroteInformationalCallback", msg, NULL);
  }
#endif

 private:
  SoupMessage *message_;
  SoupSession *session_;

  XMLParserInterface *xml_parser_;
  DOMDocumentInterface *response_dom_;

  Signal0<void> onreadystatechange_signal_;
  Signal2<size_t, const void *, size_t> ondatareceived_signal_;

  CaseInsensitiveStringMap response_headers_map_;

  std::string url_;
  std::string host_;
  std::string method_;
  std::string user_;
  std::string password_;
  std::string effective_url_;
  std::string request_data_;

  std::string response_headers_;
  std::string response_content_type_;
  std::string response_encoding_;
  std::string response_body_;
  std::string response_text_;

  std::string status_text_;
  StringVector cookies_;

  int redirect_count_;

  unsigned short status_;
  State state_ : 3;

  bool async_     : 1;
  // Required by the specification.
  // It will be true after send() is called in async mode.
  bool send_flag_ : 1;
  bool succeeded_ : 1;
};

class XMLHttpRequestFactory : public XMLHttpRequestFactoryInterface {
 public:
  XMLHttpRequestFactory() : next_session_id_(1) {
  }

  ~XMLHttpRequestFactory() {
    for (Sessions::iterator it = sessions_.begin();
         it != sessions_.end(); ++it) {
      if (it->second) {
        // Abort all pending requests.
        soup_session_abort(it->second);
        g_object_unref(it->second);
      }
    }
    sessions_.clear();
  }

  virtual int CreateSession() {
    SoupSession *session = NewSoupSession();
    if (session) {
      int result = next_session_id_++;
      sessions_[result] = session;
      return result;
    }
    return -1;
  }

  virtual void DestroySession(int session_id) {
    Sessions::iterator it = sessions_.find(session_id);
    if (it != sessions_.end()) {
      if (it->second) {
        // Abort all pending requests.
        soup_session_abort(it->second);
        g_object_unref(it->second);
      }
      sessions_.erase(it);
    } else {
      DLOG("DestroySession Invalid session: %d", session_id);
    }
  }

  virtual XMLHttpRequestInterface *CreateXMLHttpRequest(
      int session_id, XMLParserInterface *parser) {
    if (session_id == 0) {
      SoupSession *session = NewSoupSession();
      XMLHttpRequestInterface *request =
          new XMLHttpRequest(session,  parser);
      // session should already be referenced by request.
      g_object_unref(session);
      return request;
    }

    Sessions::iterator it = sessions_.find(session_id);
    if (it != sessions_.end()) {
      return new XMLHttpRequest(it->second,  parser);
    }

    DLOG("CreateXMLHttpRequest: Invalid session: %d", session_id);
    return NULL;
  }

  virtual void SetDefaultUserAgent(const char *user_agent) {
    if (user_agent) {
      default_user_agent_ = user_agent;

      for (Sessions::iterator it = sessions_.begin();
           it != sessions_.end(); ++it) {
        g_object_set(G_OBJECT(it->second),
                     SOUP_SESSION_USER_AGENT, default_user_agent_.c_str(),
                     NULL);
      }
    }
  }

 private:
  SoupSession *NewSoupSession() {
    SoupSession *session = soup_session_async_new_with_options(
        SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_COOKIE_JAR,
#ifdef HAVE_LIBSOUP_GNOME
        SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_PROXY_RESOLVER_GNOME,
#endif
        NULL);

    if (session) {
#ifdef SOUP_XHR_VERBOSE
      SoupLogger *logger = soup_logger_new(SOUP_LOGGER_LOG_HEADERS, -1);
      soup_logger_set_printer(logger, LoggerPrinter, NULL, NULL);
      soup_session_add_feature(session, SOUP_SESSION_FEATURE(logger));
      g_object_unref(logger);
#endif
      g_object_set(G_OBJECT(session),
                   SOUP_SESSION_USER_AGENT, default_user_agent_.c_str(),
                   SOUP_SESSION_TIMEOUT, kSessionTimeout,
#ifdef GGL_DEFAULT_SSL_CA_FILE
                   SOUP_SESSION_SSL_CA_FILE, GGL_DEFAULT_SSL_CA_FILE,
#endif
                   NULL);

      g_signal_connect(G_OBJECT(session), "authenticate",
                       G_CALLBACK(AuthenticateCallback), this);
      g_signal_connect(G_OBJECT(session), "request-started",
                       G_CALLBACK(RequestStartedCallback), this);
#ifdef SOUP_XHR_VERBOSE
      g_signal_connect(G_OBJECT(session), "request-queued",
                       G_CALLBACK(RequestQueuedCallback), this);
      g_signal_connect(G_OBJECT(session), "request-unqueued",
                       G_CALLBACK(RequestUnqueuedCallback), this);
#endif
    }
    return session;
  }

#ifdef SOUP_XHR_VERBOSE
  static void LoggerPrinter(SoupLogger *logger, SoupLoggerLogLevel level,
                            char direction, const char *data,
                            gpointer user_data) {
    GGL_UNUSED(logger);
    GGL_UNUSED(level);
    GGL_UNUSED(direction);
    GGL_UNUSED(data);
    GGL_UNUSED(user_data);
    DLOG("%c %s\n", direction, data);
  }
#endif

  static void AuthenticateCallback(SoupSession *session,
                                   SoupMessage *msg,
                                   SoupAuth *auth,
                                   gboolean retrying,
                                   gpointer user_data) {
    GGL_UNUSED(session);
    GGL_UNUSED(user_data);

    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(
        g_object_get_data(G_OBJECT(msg), kSoupMessageXHRKey));
    ASSERT(request);
#ifdef SOUP_XHR_VERBOSE
    XMLHttpRequest::PrintMessageInfo(request, "AuthenticateCallback", msg,
                                     NULL);
#endif
    // TODO: Show authenticate dialog when necessary.
    if (!retrying && !soup_auth_is_for_proxy(auth)) {
      // Let XMLHttpRequest fill in user and password information
      // only if it's not retrying and not for proxy.
      request->Authenticate(auth);
    }
  }

  static void RequestStartedCallback(SoupSession *session,
                                     SoupMessage *msg,
                                     SoupSocket *socket,
                                     gpointer user_data) {
    GGL_UNUSED(session);
    GGL_UNUSED(socket);
    GGL_UNUSED(user_data);

    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(
        g_object_get_data(G_OBJECT(msg), kSoupMessageXHRKey));
#ifdef SOUP_XHR_VERBOSE
    XMLHttpRequest::PrintMessageInfo(request, "RequestStartedCallback", msg,
                                     NULL);
#endif
    // msg might be an additional message created by soup internally,
    // in this case, request will be null.
    if (request) {
      ASSERT(request->GetReadyState() == XMLHttpRequest::OPENED);
      request->SetupCookie();
    }
  }

#ifdef SOUP_XHR_VERBOSE
  static void RequestQueuedCallback(SoupSession *session,
                                    SoupMessage *msg,
                                    gpointer user_data) {
    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(
        g_object_get_data(G_OBJECT(msg), kSoupMessageXHRKey));
    XMLHttpRequest::PrintMessageInfo(request, "RequestQueuedCallback", msg,
                                     NULL);
  }

  static void RequestUnqueuedCallback(SoupSession *session,
                                      SoupMessage *msg,
                                      gpointer user_data) {
    XMLHttpRequest *request = static_cast<XMLHttpRequest *>(
        g_object_get_data(G_OBJECT(msg), kSoupMessageXHRKey));
    XMLHttpRequest::PrintMessageInfo(request, "RequestUnqueuedCallback", msg,
                                     NULL);
  }
#endif

 private:
  typedef LightMap<int, SoupSession *> Sessions;
  Sessions sessions_;
  int next_session_id_;
  std::string default_user_agent_;
};

} // namespace soup
} // namespace ggadget

#define Initialize soup_xml_http_request_LTX_Initialize
#define Finalize soup_xml_http_request_LTX_Finalize

static ggadget::soup::XMLHttpRequestFactory g_factory;

extern "C" {
  bool Initialize() {
    LOGI("Initialize soup_xml_http_request extension.");
    return ggadget::SetXMLHttpRequestFactory(&g_factory);
  }

  void Finalize() {
    LOGI("Finalize soup_xml_http_request extension.");
  }
}
