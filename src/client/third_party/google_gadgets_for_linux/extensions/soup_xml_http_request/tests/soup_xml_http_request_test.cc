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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <sys/poll.h>
#include <signal.h>
#include <gtk/gtk.h>

#include "ggadget/logger.h"
#include "ggadget/gadget_consts.h"
#include "ggadget/slot.h"
#include "ggadget/xml_dom_interface.h"
#include "ggadget/xml_http_request_interface.h"
#include "ggadget/xml_parser_interface.h"
#include "ggadget/memory_options.h"
#include "ggadget/tests/init_extensions.h"
#include "ggadget/gtk/main_loop.h"
#include "unittest/gtest.h"

using ggadget::XMLHttpRequestInterface;
using ggadget::NewSlot;
using ggadget::DOMDocumentInterface;
using ggadget::XMLParserInterface;
using ggadget::GetXMLParser;
using ggadget::OptionsInterface;
using ggadget::MemoryOptions;
using ggadget::SetGlobalOptions;

static ggadget::gtk::MainLoop g_main_loop;

XMLHttpRequestInterface *CreateXMLHttpRequest(int session_id) {
  return ggadget::GetXMLHttpRequestFactory()->CreateXMLHttpRequest(
      session_id, GetXMLParser());
}

TEST(XMLHttpRequest, States) {
  XMLHttpRequestInterface *request = CreateXMLHttpRequest(0);
  request->Ref();
  ASSERT_FALSE(request->IsSuccessful());
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  // Invalid request method.
  ASSERT_EQ(XMLHttpRequestInterface::SYNTAX_ERR,
            request->Open("DELETE", "http://localhost", false, NULL, NULL));
  ASSERT_FALSE(request->IsSuccessful());
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  // Invalid state.
  ASSERT_EQ(XMLHttpRequestInterface::INVALID_STATE_ERR,
            request->Send(std::string()));
  ASSERT_FALSE(request->IsSuccessful());
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  // Valid request.
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->Open("GET", "http://localhost", false, NULL, NULL));
  ASSERT_FALSE(request->IsSuccessful());
  ASSERT_EQ(XMLHttpRequestInterface::OPENED, request->GetReadyState());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->SetRequestHeader("aaa", "bbb"));
  request->Abort();
  ASSERT_FALSE(request->IsSuccessful());
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  ASSERT_EQ(XMLHttpRequestInterface::INVALID_STATE_ERR,
            request->SetRequestHeader("ccc", "ddd"));
  request->Unref();
}

class Callback {
 public:
  Callback(XMLHttpRequestInterface *request)
      : aborted_(false), abort_pos_(0), callback_count_(0),
        request_(request) {
  }
  Callback(XMLHttpRequestInterface *request, int abort_pos)
      : aborted_(false), abort_pos_(abort_pos), callback_count_(0),
        request_(request) {
  }
  void Call() {
    callback_count_++;
    LOG("Callback called %d times, state: %d", callback_count_,
        request_->GetReadyState());
    if (aborted_) {
      ASSERT_FALSE(request_->IsSuccessful());
      ASSERT_EQ(XMLHttpRequestInterface::DONE, request_->GetReadyState());
    } else {
      switch (callback_count_) {
        case 1:
          ASSERT_FALSE(request_->IsSuccessful());
          ASSERT_EQ(XMLHttpRequestInterface::OPENED, request_->GetReadyState());
          break;
        case 2:
          ASSERT_FALSE(request_->IsSuccessful());
          ASSERT_EQ(XMLHttpRequestInterface::OPENED, request_->GetReadyState());
          break;
        case 3:
          ASSERT_FALSE(request_->IsSuccessful());
          ASSERT_EQ(XMLHttpRequestInterface::HEADERS_RECEIVED,
                    request_->GetReadyState());
          break;
        case 4:
          ASSERT_FALSE(request_->IsSuccessful());
          ASSERT_EQ(XMLHttpRequestInterface::LOADING,
                    request_->GetReadyState());
          break;
        case 5:
          ASSERT_EQ(XMLHttpRequestInterface::DONE, request_->GetReadyState());
          break;
        default:
          ASSERT_TRUE(false);
          break;
      }
      if (callback_count_ == abort_pos_) {
        LOG("Abort the request.");
        aborted_ = true;
        request_->Abort();
      }
    }
  }
  bool aborted_;
  int abort_pos_;
  int callback_count_;
  XMLHttpRequestInterface *request_;
};

const char *kResponse0 = "HTTP/1.1 200 OK\r\n";
const char *kResponse1 = "Connection: Close\r\n"
                         "Set-Cookie: COOKIE1=Value1; Path=/\r\n"
                         "TestHeader1: Value1\r\n";
const char *kResponse2 = "TestHeader2: Value2a\r\n"
                         "TestHeader2: Value2b\r\n";
const char *kResponseSep = "\r\n";
const char *kResponse3 = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n";
const char *kResponse4 = "<root>\xBA\xBA\xD7\xD6</root>\r\n";
const char *kResponseText = "<?xml version=\"1.0\" encoding=\"gb2312\"?>\r\n"
                            "<root>\xE6\xB1\x89\xE5\xAD\x97</root>\r\n";

void Wait(int ms) {
  static timespec tm = { 0, ms * 1000000 }; // Sleep for 100ms.
  nanosleep(&tm, NULL);
}

class Server {
 public:
  // If instructed is true, the server thread will wait for the client to
  // instruct its next step; otherwise the server thread will run automatically.
  Server(bool instructed)
      : instructed_(instructed), succeeded_(false), instruction_(0), port_(0) {
    pthread_create(&thread_, NULL, Thread, this);
    while (port_ == 0) Wait(100);
    Wait(50);
  }

  class SocketCloser {
   public:
    SocketCloser(int socket) : socket_(socket) { }
    ~SocketCloser() { close(socket_); }
    int socket_;
  };

  void WaitFor(int value) {
    while (instruction_ != value)
      Wait(2);
  }

  static void *Thread(void *arg) {
    static_cast<Server *>(arg)->ThreadInner();
    return arg;
  }

  void ThreadInner() {
    int s = socket(PF_INET, SOCK_STREAM, 0);
    LOG("Server created socket: %d", s);
    ASSERT_GT(s, 0);
    SocketCloser closer1(s);

    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t socklen = sizeof(sin);
    ASSERT_EQ(0, bind(s, reinterpret_cast<sockaddr *>(&sin), socklen));
    ASSERT_EQ(0, listen(s, 5));
    ASSERT_EQ(0, getsockname(s, reinterpret_cast<sockaddr *>(&sin), &socklen));
    port_ = ntohs(sin.sin_port);
    LOG("Server bound to port: %d", port_);

    sockaddr_in cin;
    LOG("Server is waiting for connection");
    int s1 = accept(s, reinterpret_cast<sockaddr *>(&cin), &socklen);
    ASSERT_GT(s1, 0);
    SocketCloser closer2(s1);
    LOG("Server accepted a connection: %d", s1);

    int request_type = -1;  // 0: GET, 1: POST, 2: HEAD.
    while (true) {
      char b;
      read(s1, &b, 1);
      request_ += b;
      if (request_.size() > 4) {
        if (request_type == -1) {
          if (strncmp(request_.c_str(), "GET ", 4) == 0)
            request_type = 0;
          else if (strncmp(request_.c_str(), "POST", 4) == 0)
            request_type = 1;
          else if (strncmp(request_.c_str(), "HEAD", 4) == 0)
            request_type = 2;
        }

        const char *end_tag = request_.c_str() + request_.size() - 4;
        if ((request_type == 1 && strcmp(end_tag, "##\r\n") == 0) ||
            (request_type != 1 && strcmp(end_tag, "\r\n\r\n") == 0)) {
          // End of request.
          break;
        }
      }
    }
    LOG("Server got the whole request: %s", request_.c_str());

    if (instructed_) WaitFor(1); else Wait(100);
    LOG("Server write response0");
    write(s1, kResponse0, strlen(kResponse0));
    LOG("Server write response1");
    write(s1, kResponse1, strlen(kResponse1));
    if (instructed_) WaitFor(2); else Wait(100);
    LOG("Server write response2");
    write(s1, kResponse2, strlen(kResponse2));
    write(s1, kResponseSep, strlen(kResponseSep));
    if (instructed_) WaitFor(3); else Wait(100);
    if (request_type != 2) {
      LOG("Server write response3");
      write(s1, kResponse3, strlen(kResponse3));
    }
    if (instructed_) WaitFor(4); else Wait(100);
    if (request_type != 2) {
      LOG("Server write response4");
      write(s1, kResponse4, strlen(kResponse4));
    }
    LOG("Server succeeded");
    succeeded_ = true;
  }

  bool instructed_;
  bool succeeded_;
  int instruction_;
  int port_;
  pthread_t thread_;
  std::string request_;
};

TEST(XMLHttpRequest, SyncNetworkFile) {
  XMLHttpRequestInterface *request = CreateXMLHttpRequest(0);
  request->Ref();

  Server server(false);
  Callback callback(request);
  ASSERT_EQ(0, callback.callback_count_);
  request->ConnectOnReadyStateChange(NewSlot(&callback, &Callback::Call));
  char url[256];
  snprintf(url, sizeof(url), "http://localhost:%d/test", server.port_);
  LOG("URL=%s", url);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->Open("GET", url, false, NULL, NULL));
  ASSERT_EQ(1, callback.callback_count_);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->SetRequestHeader("TestHeader", "TestHeaderValue"));
  ASSERT_EQ(XMLHttpRequestInterface::OPENED, request->GetReadyState());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->Send(std::string()));
  ASSERT_EQ(XMLHttpRequestInterface::DONE, request->GetReadyState());
  ASSERT_EQ(5, callback.callback_count_);
  ASSERT_TRUE(request->IsSuccessful());

  ASSERT_EQ(0U, server.request_.find("GET /test HTTP/"));
  ASSERT_NE(std::string::npos,
            server.request_.find("TestHeader: TestHeaderValue\r\n"));
  ASSERT_EQ(std::string::npos, server.request_.find("Cookie:"));

  const std::string *str_p;
  std::string str;
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetAllResponseHeaders(&str_p));
  ASSERT_EQ(std::string(kResponse1) + kResponse2, *str_p);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetResponseBody(&str));
  ASSERT_EQ(std::string(kResponse3) + kResponse4, str);
  ASSERT_EQ(strlen(kResponse3) + strlen(kResponse4), str.size());
  unsigned short status;
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetStatus(&status));
  ASSERT_EQ(200, status);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetStatusText(&str_p));
  ASSERT_STREQ("OK", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("NoSuchHeader", &str_p));
  ASSERT_TRUE(NULL == str_p);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("TestHeader1", &str_p));
  ASSERT_STREQ("Value1", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("testheader1", &str_p));
  ASSERT_STREQ("Value1", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("testheader2", &str_p));
  ASSERT_STREQ("Value2a, Value2b", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("TestHeader2", &str_p));
  ASSERT_STREQ("Value2a, Value2b", str_p->c_str());

  pthread_join(server.thread_, NULL);
  ASSERT_TRUE(server.succeeded_);

  std::string text;
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetResponseText(&text));
  ASSERT_STREQ(kResponseText, text.c_str());
  DOMDocumentInterface *dom = NULL;
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetResponseXML(&dom));
  ASSERT_TRUE(dom);
  ASSERT_STREQ("\xE6\xB1\x89\xE5\xAD\x97",
               dom->GetDocumentElement()->GetTextContent().c_str());

  ASSERT_EQ(1, request->GetRefCount());
  request->Unref();
}

TEST(XMLHttpRequest, AsyncNetworkFile) {
  XMLHttpRequestInterface *request = CreateXMLHttpRequest(0);
  request->Ref();
  ASSERT_FALSE(request->IsSuccessful());

  Server server(true);
  Callback callback(request);
  ASSERT_EQ(0, callback.callback_count_);
  request->ConnectOnReadyStateChange(NewSlot(&callback, &Callback::Call));
  char url[256];
  snprintf(url, sizeof(url), "http://localhost:%d/test", server.port_);
  LOG("URL=%s", url);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->Open("GET", url, true, NULL, NULL));
  ASSERT_EQ(1, callback.callback_count_);
  ASSERT_FALSE(request->IsSuccessful());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->SetRequestHeader("TestHeader", "TestHeaderValue"));
  ASSERT_EQ(XMLHttpRequestInterface::OPENED, request->GetReadyState());
  ASSERT_FALSE(request->IsSuccessful());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->Send(std::string()));
  ASSERT_EQ(XMLHttpRequestInterface::OPENED, request->GetReadyState());
  ASSERT_EQ(2, callback.callback_count_);
  ASSERT_FALSE(request->IsSuccessful());

  const std::string *str_p;
  std::string str;
  server.instruction_ = 1;
  for (int i = 0; i < 10; i++) { Wait(10); g_main_loop.DoIteration(false); }
  ASSERT_EQ(0U, server.request_.find("GET /test HTTP/1."));
  ASSERT_NE(std::string::npos,
            server.request_.find("TestHeader: TestHeaderValue\r\n"));
  ASSERT_EQ(std::string::npos, server.request_.find("Cookie:"));

  ASSERT_EQ(XMLHttpRequestInterface::OPENED, request->GetReadyState());
  ASSERT_EQ(2, callback.callback_count_);
  // GetAllResponseHeaders and GetResponseBody return NULL in OPEN state.
  ASSERT_EQ(XMLHttpRequestInterface::INVALID_STATE_ERR,
            request->GetAllResponseHeaders(&str_p));
  ASSERT_TRUE(NULL == str_p);
  ASSERT_EQ(XMLHttpRequestInterface::INVALID_STATE_ERR,
            request->GetResponseBody(&str));
  ASSERT_EQ(0u, str.size());
  ASSERT_EQ(XMLHttpRequestInterface::INVALID_STATE_ERR,
            request->GetStatusText(&str_p));
  ASSERT_TRUE(NULL == str_p);
  ASSERT_FALSE(request->IsSuccessful());

  server.instruction_ = 2;
  for (int i = 0; i < 10; i++) { Wait(10); g_main_loop.DoIteration(false); }
  ASSERT_EQ(XMLHttpRequestInterface::HEADERS_RECEIVED, request->GetReadyState());
  ASSERT_FALSE(request->IsSuccessful());

  server.instruction_ = 3;
  for (int i = 0; i < 10; i++) { Wait(10); g_main_loop.DoIteration(false); }
  ASSERT_EQ(XMLHttpRequestInterface::LOADING, request->GetReadyState());
  ASSERT_EQ(4, callback.callback_count_);
  ASSERT_FALSE(request->IsSuccessful());

  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetAllResponseHeaders(&str_p));
  ASSERT_EQ(std::string(kResponse1) + kResponse2, *str_p);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetResponseBody(&str));
  ASSERT_STREQ(kResponse3, str.c_str());
  ASSERT_EQ(strlen(kResponse3), str.size());
  unsigned short status;
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetStatus(&status));
  ASSERT_EQ(200, status);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetStatusText(&str_p));
  ASSERT_STREQ("OK", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("NoSuchHeader", &str_p));
  ASSERT_TRUE(NULL == str_p);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("TestHeader1", &str_p));
  ASSERT_STREQ("Value1", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("testheader1", &str_p));
  ASSERT_STREQ("Value1", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("testheader2", &str_p));
  ASSERT_STREQ("Value2a, Value2b", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetResponseHeader("TestHeader2", &str_p));
  ASSERT_STREQ("Value2a, Value2b", str_p->c_str());
  ASSERT_FALSE(request->IsSuccessful());

  server.instruction_ = 4;
  for (int i = 0; i < 10; i++) { Wait(10); g_main_loop.DoIteration(false); }
  ASSERT_EQ(XMLHttpRequestInterface::DONE, request->GetReadyState());
  ASSERT_EQ(5, callback.callback_count_);
  ASSERT_TRUE(request->IsSuccessful());

  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->GetAllResponseHeaders(&str_p));
  ASSERT_EQ(std::string(kResponse1) + kResponse2, *str_p);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetResponseBody(&str));
  ASSERT_EQ(std::string(kResponse3) + kResponse4, str);
  ASSERT_EQ(strlen(kResponse3) + strlen(kResponse4), str.size());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetStatus(&status));
  ASSERT_EQ(200, status);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetStatusText(&str_p));
  ASSERT_STREQ("OK", str_p->c_str());

  pthread_join(server.thread_, NULL);
  ASSERT_TRUE(server.succeeded_);

  std::string text;
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetResponseText(&text));
  ASSERT_STREQ(kResponseText, text.c_str());
  DOMDocumentInterface *dom = NULL;
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->GetResponseXML(&dom));
  ASSERT_TRUE(dom);
  ASSERT_STREQ("\xE6\xB1\x89\xE5\xAD\x97",
               dom->GetDocumentElement()->GetTextContent().c_str());

  ASSERT_EQ(1, request->GetRefCount());
  ASSERT_TRUE(request->IsSuccessful());
  request->Unref();
}

TEST(XMLHttpRequest, ConcurrentHEADandPOSTandCookie) {
  int session = ggadget::GetXMLHttpRequestFactory()->CreateSession();
  ASSERT_GT(session, 0);

  XMLHttpRequestInterface *request1 = CreateXMLHttpRequest(session);
  XMLHttpRequestInterface *request2 = CreateXMLHttpRequest(session);
  request1->Ref();
  request2->Ref();

  // Start 2 server threads.
  Server server1(false), server2(false);
  char url1[256], url2[256];
  snprintf(url1, sizeof(url1), "http://localhost:%d/test1", server1.port_);
  snprintf(url2, sizeof(url2), "http://localhost:%d/test2", server2.port_);

  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request1->Open("HEAD", url1, true, NULL, NULL));
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request1->Send(std::string()));
  ASSERT_EQ(XMLHttpRequestInterface::OPENED, request1->GetReadyState());

  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request2->Open("POST", url2, true, NULL, NULL));
  const char *post_data = "Some Data To Post.##\r\n";
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request2->Send(post_data));
  ASSERT_EQ(XMLHttpRequestInterface::OPENED, request2->GetReadyState());

  for (int i = 0; i < 10; i++) { Wait(10); g_main_loop.DoIteration(false); }
  pthread_join(server1.thread_, NULL);
  pthread_join(server2.thread_, NULL);
  ASSERT_TRUE(server1.succeeded_);
  ASSERT_TRUE(server2.succeeded_);

  for (int i = 0; i < 30; i++) { Wait(10); g_main_loop.DoIteration(false); }
  unsigned short status;
  const std::string *str_p;
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request1->GetStatus(&status));
  ASSERT_EQ(200, status);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request1->GetStatusText(&str_p));
  ASSERT_STREQ("OK", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::DONE, request1->GetReadyState());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request2->GetStatus(&status));
  ASSERT_EQ(200, status);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request2->GetStatusText(&str_p));
  ASSERT_STREQ("OK", str_p->c_str());
  ASSERT_EQ(XMLHttpRequestInterface::DONE, request2->GetReadyState());

  ASSERT_EQ(0U, server1.request_.find("HEAD /test1 HTTP/"));
  ASSERT_EQ(0U, server2.request_.find("POST /test2 HTTP/"));
  ASSERT_NE(std::string::npos, server2.request_.find("Content-Type: "
      "application/x-www-form-urlencoded"));

  ASSERT_EQ(1, request1->GetRefCount());
  ASSERT_EQ(1, request2->GetRefCount());
  request1->Unref();
  request2->Unref();

  XMLHttpRequestInterface *request3 = CreateXMLHttpRequest(session);
  request3->Ref();
  Server server3(false);
  char url3[256];
  snprintf(url3, sizeof(url3), "http://localhost:%d/test3", server3.port_);

  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request3->Open("GET", url3, true, NULL, NULL));
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request3->Send(std::string()));
  Wait(100);
  request3->Abort();
  ASSERT_FALSE(request3->IsSuccessful());

  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request3->Open("GET", url3, true, NULL, NULL));
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request3->Send(std::string()));
  Wait(100);
  request3->Abort();
  ASSERT_FALSE(request3->IsSuccessful());

  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request3->Open("GET", url3, true, NULL, NULL));
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request3->Send(std::string()));

  for (int i = 0; i < 10; i++) { Wait(10); g_main_loop.DoIteration(false); }
  pthread_join(server3.thread_, NULL);
  ASSERT_TRUE(server3.succeeded_);
  for (int i = 0; i < 30; i++) { Wait(10); g_main_loop.DoIteration(false); }
  ASSERT_NE(std::string::npos,
            server3.request_.find("Cookie: COOKIE1=Value1"));

  ASSERT_EQ(1, request3->GetRefCount());
  request3->Unref();

  ggadget::GetXMLHttpRequestFactory()->DestroySession(session);
}

TEST(XMLHttpRequest, AbortInOpen) {
  XMLHttpRequestInterface *request = CreateXMLHttpRequest(0);
  request->Ref();

  // Abort after opened.
  Callback callback(request, 1);
  ASSERT_EQ(0, callback.callback_count_);
  request->ConnectOnReadyStateChange(NewSlot(&callback, &Callback::Call));
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->Open("GET", "http://localhost", false, NULL, NULL));
  // aborting request before calling send will not trigger another
  // readystatechange signal.
  ASSERT_EQ(1, callback.callback_count_);
  ASSERT_TRUE(callback.aborted_);
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  ASSERT_FALSE(request->IsSuccessful());

  ASSERT_EQ(1, request->GetRefCount());
  request->Unref();
}

TEST(XMLHttpRequest, AbortAfterGotHeaders) {
  XMLHttpRequestInterface *request = CreateXMLHttpRequest(0);
  request->Ref();

  Server server(false);
  // abort after got headers.
  Callback callback(request, 3);
  ASSERT_EQ(0, callback.callback_count_);
  request->ConnectOnReadyStateChange(NewSlot(&callback, &Callback::Call));
  char url[256];
  snprintf(url, sizeof(url), "http://localhost:%d/test", server.port_);
  LOG("URL=%s", url);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->Open("GET", url, false, NULL, NULL));
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->SetRequestHeader("TestHeader", "TestHeaderValue"));
  ASSERT_EQ(XMLHttpRequestInterface::OPENED, request->GetReadyState());
  ASSERT_EQ(XMLHttpRequestInterface::ABORT_ERR, request->Send(std::string()));
  // another readystatechange signal will be triggered.
  ASSERT_EQ(4, callback.callback_count_);
  ASSERT_TRUE(callback.aborted_);
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  ASSERT_FALSE(request->IsSuccessful());

  pthread_join(server.thread_, NULL);
  ASSERT_TRUE(server.succeeded_);

  ASSERT_EQ(1, request->GetRefCount());
  request->Unref();
}

TEST(XMLHttpRequest, AbortAfterFinished) {
  XMLHttpRequestInterface *request = CreateXMLHttpRequest(0);
  request->Ref();

  Server server(false);
  // abort after finished.
  Callback callback(request, 5);
  ASSERT_EQ(0, callback.callback_count_);
  request->ConnectOnReadyStateChange(NewSlot(&callback, &Callback::Call));
  char url[256];
  snprintf(url, sizeof(url), "http://localhost:%d/test", server.port_);
  LOG("URL=%s", url);
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->Open("GET", url, false, NULL, NULL));
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR,
            request->SetRequestHeader("TestHeader", "TestHeaderValue"));
  ASSERT_EQ(XMLHttpRequestInterface::OPENED, request->GetReadyState());
  ASSERT_EQ(XMLHttpRequestInterface::NO_ERR, request->Send(std::string()));
  // no additional readystatechange signal.
  ASSERT_EQ(5, callback.callback_count_);
  ASSERT_TRUE(callback.aborted_);
  ASSERT_EQ(XMLHttpRequestInterface::UNSENT, request->GetReadyState());
  ASSERT_FALSE(request->IsSuccessful());

  pthread_join(server.thread_, NULL);
  ASSERT_TRUE(server.succeeded_);

  ASSERT_EQ(1, request->GetRefCount());
  request->Unref();
}

static OptionsInterface *MemoryOptionsFactory(const char *name) {
  return new MemoryOptions();
}

int main(int argc, char **argv) {
  gtk_init(&argc, &argv);

  ggadget::SetGlobalMainLoop(&g_main_loop);
  testing::ParseGTestFlags(&argc, argv);

  // To prevent the server from calling exit when meet SIGPIPE.
  signal(SIGPIPE, SIG_IGN);

  SetOptionsFactory(MemoryOptionsFactory);

  static const char *kExtensions[] = {
    "soup_xml_http_request/soup-xml-http-request",
    "libxml2_xml_parser/libxml2-xml-parser",
    // Don't load options module to disable backoff.
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);

  return RUN_ALL_TESTS();
}
