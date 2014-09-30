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

#include "tsf/context_manager.h"
#include "tsf/context_event_sink.h"
#include "tsf/thread_manager_event_sink.h"

namespace ime_goopy {
namespace tsf {
ContextManager::ContextManager(
    ThreadManagerEventSink *owner) : owner_(owner) {
  DCHECK(owner != NULL);
}

ContextManager::~ContextManager() {
  RemoveAll();
}

void ContextManager::GetOrCreate(ITfContext *context,
                                 CComObject<ContextEventSink> **sink) {
  if (sink == NULL) {
    DCHECK(false);
    return;
  }
  if (owner_ == NULL) {
    DCHECK(false);
    *sink = NULL;
  }
  DCHECK(*sink == NULL);

  ContextMap::iterator iter = map_.find(context);
  if (iter != map_.end()) {
    *sink = iter->second;
    if (*sink)
      (*sink)->AddRef();
  } else {
    DVLOG(2) << L"Creating context sink.";
    SmartComObjPtr<ContextEventSink> new_sink;
    new_sink.CreateInstance();
    if (FAILED(new_sink->Initialize(owner_, context))) {
      // Some context are not accepting text inputs.
      new_sink->Uninitialize();
      new_sink.Release();
      return;
    }
    map_.insert(std::make_pair(context, new_sink.p()));

    // Adds ref for both key and value.
    context->AddRef();
    if (new_sink) new_sink->AddRef();

    // Returns the sink object.
    *sink = new_sink.p();
    if (*sink)
      (*sink)->AddRef();
  }
}

void ContextManager::RemoveByContext(ITfContext *context) {
  ContextMap::iterator iter = map_.find(context);
  if (iter != map_.end()) {
    ReleaseIterator(iter);
    map_.erase(iter);
  }
}

void ContextManager::RemoveByDocumentManager(ITfDocumentMgr *document_mgr) {
  vector<ContextMap::iterator> to_remove;
  for (ContextMap::iterator iter = map_.begin();
       iter != map_.end(); ++iter) {
    SmartComPtr<ITfDocumentMgr> entry_document_mgr;
    iter->first->GetDocumentMgr(entry_document_mgr.GetAddress());
    if (entry_document_mgr.p() == document_mgr)
      to_remove.push_back(iter);
  }
  for (size_t i = 0; i < to_remove.size(); ++i) {
    ReleaseIterator(to_remove[i]);
    map_.erase(to_remove[i]);
  }
}

void ContextManager::RemoveAll() {
  for (ContextMap::iterator iter = map_.begin();
       iter != map_.end(); ++iter) {
    ReleaseIterator(iter);
  }
  map_.clear();
}

void ContextManager::ReleaseIterator(
    const ContextManager::ContextMap::iterator &iter) {
  DCHECK(iter->first);
  if (iter->first)
    iter->first->Release();
  if (iter->second) {
    iter->second->Uninitialize();
    iter->second->Release();
  }
}
}  // namespace tsf
}  // namespace ime_goopy
