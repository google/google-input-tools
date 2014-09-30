/*
  Copyright 2014 Google Inc.

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

#ifndef GOOPY_TSF_CONTEXT_MANAGER_H_
#define GOOPY_TSF_CONTEXT_MANAGER_H_

#include <atlbase.h>
#include <atlcom.h>
#include <msctf.h>
#include <map>
#include "common/smart_com_ptr.h"

namespace ime_goopy {
namespace tsf {

class ContextEventSink;
class ThreadManagerEventSink;

class ContextManager {
 public:
  explicit ContextManager(ThreadManagerEventSink *owner);
  ~ContextManager();
  // Gets the event sink for a specific context.
  // Creates if the sink doesn't exist.
  void GetOrCreate(ITfContext *context, CComObject<ContextEventSink> **sink);
  // Removes a context entry.
  void RemoveByContext(ITfContext *context);
  // Removes all the context entries associated to a document manager.
  void RemoveByDocumentManager(ITfDocumentMgr *manager);
  // Removes all the context entries.
  void RemoveAll();
 private:
  typedef std::map<ITfContext*, CComObject<ContextEventSink>*> ContextMap;
  void ReleaseIterator(const ContextMap::iterator &iter);
  ThreadManagerEventSink *owner_;
  ContextMap map_;
  DISALLOW_COPY_AND_ASSIGN(ContextManager);
};
}  // namespace tsf
}  // namespace ime_goopy
#endif  // GOOPY_TSF_CONTEXT_MANAGER_H_
