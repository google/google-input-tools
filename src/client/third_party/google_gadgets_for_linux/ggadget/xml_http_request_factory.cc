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

#include "xml_http_request_interface.h"

namespace ggadget {

static XMLHttpRequestFactoryInterface *g_factory = NULL;

bool SetXMLHttpRequestFactory(XMLHttpRequestFactoryInterface *factory) {
  ASSERT(!g_factory && factory);
  if (!g_factory && factory) {
    g_factory = factory;
    return true;
  }
  return false;
}

XMLHttpRequestFactoryInterface *GetXMLHttpRequestFactory() {
  EXPECT_M(g_factory, ("The XMLHttpRequest factory has not been set yet."));
  return g_factory;
}

} // namespace ggadget
