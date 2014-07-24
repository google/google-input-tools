// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
/*
 * Copyright 2013 Google Inc. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "url_loader_util.h"

#include <stdio.h>
#include <iostream>

#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/url_loader.h>
#include <ppapi/cpp/url_request_info.h>
#include <ppapi/cpp/url_response_info.h>
#include <ppapi/cpp/var.h>
#include <ppapi/utility/completion_callback_factory.h>

namespace {

const int kReadBufferSize = 32768;

class URLLoaderHandler {
 public:
  URLLoaderHandler(pp::Instance *instance,
                   const string &url,
                   DownloadCallback callback,
                   void *data_user);
  void Start();

 private:
  // "delete this" is called in URLLoaderHandler::Complete().
  ~URLLoaderHandler();
  void OnOpen(int result);
  void OnRead(int result);
  void ReadBody();
  void AppendDataBytes(const char *buffer, int num_bytes);
  void Complete(bool result);

  pp::Instance *instance_;
  string url_;
  DownloadCallback callback_;
  pp::URLRequestInfo *url_request_;
  pp::URLLoader *url_loader_;
  char *tmp_buffer_;
  string data_buffer_;
  void *data_user_;
  pp::CompletionCallbackFactory<URLLoaderHandler> callback_factory_;

  URLLoaderHandler(const URLLoaderHandler&);
  void operator=(const URLLoaderHandler&);
};

URLLoaderHandler::URLLoaderHandler(pp::Instance *instance,
                                   const string &url,
                                   DownloadCallback callback,
                                   void *data_user)
    : instance_(instance),
      url_(url),
      callback_(callback),
      tmp_buffer_(new char[kReadBufferSize]),
      data_user_(data_user) {
}

URLLoaderHandler::~URLLoaderHandler() {
}

void URLLoaderHandler::Start() {
  // Start to trying to download data
  url_request_ = new pp::URLRequestInfo(instance_);
  url_loader_ = new pp::URLLoader(instance_);
  url_request_->SetURL(url_);
  url_request_->SetMethod("GET");
  url_request_->SetRecordDownloadProgress(true);
  callback_factory_.Initialize(this);
  // Start to open the request
  url_loader_->Open(*url_request_,
                    callback_factory_.NewCallback(&URLLoaderHandler::OnOpen));
}

void URLLoaderHandler::OnOpen(int result) {
  if (result != PP_OK) {
    std::cerr << "pp::URLLoader::Open() failed: " << url_ << std::endl;
    Complete(false);
    return;
  }
  const pp::URLResponseInfo &response = url_loader_->GetResponseInfo();
  if (response.GetStatusCode() != 200) {
    std::cerr << "pp::URLLoader::Open() failed: " << url_
         << " Status code: " << response.GetStatusCode() << std::endl;
    Complete(false);
    return;
  }
  int64_t bytes_received = 0;
  int64_t bytes_total = 0;
  if (url_loader_->GetDownloadProgress(&bytes_received, &bytes_total)) {
    if (bytes_total > 0) {
      data_buffer_.reserve(bytes_total);
    }
  }
  url_request_->SetRecordDownloadProgress(false);
  ReadBody();
}

void URLLoaderHandler::AppendDataBytes(const char *buffer, int num_bytes) {
  if (num_bytes <= 0) {
    return;
  }
  num_bytes = std::min(kReadBufferSize, num_bytes);
  data_buffer_.insert(data_buffer_.end(),
                      buffer,
                      buffer + num_bytes);
}

void URLLoaderHandler::OnRead(int result) {
  if (result == PP_OK) {
    Complete(true);
  } else if (result > 0) {
    AppendDataBytes(tmp_buffer_, result);
    ReadBody();
  } else {
    std::cerr << "pp::URLLoader::ReadResponseBody() result < 0: "
        << url_ << std::endl;
    Complete(false);
  }
}

void URLLoaderHandler::ReadBody() {
  pp::CompletionCallback completion_callback =
      callback_factory_.NewOptionalCallback(&URLLoaderHandler::OnRead);
  int result = PP_OK;
  // Begin to read data
  do {
    result = url_loader_->ReadResponseBody(tmp_buffer_,
                                           kReadBufferSize,
                                           completion_callback);
    if (result > 0) {
      AppendDataBytes(tmp_buffer_, result);
    }
  } while (result > 0);
  // Read data done
  if (result != PP_OK_COMPLETIONPENDING) {
    // Read data success
    completion_callback.Run(result);
  }
}

void URLLoaderHandler::Complete(bool result) {
  callback_(data_user_, url_, result, &data_buffer_);
  delete this;
}

}  // namespace

void URLLoaderUtil::StartDownload(pp::Instance *instance,
                                  const string &url,
                                  DownloadCallback callback,
                                  void *data_user) {
  URLLoaderHandler *handler = new URLLoaderHandler(instance,
                                                   url,
                                                   callback,
                                                   data_user);
  handler->Start();
}
