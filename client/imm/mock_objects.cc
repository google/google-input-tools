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

#include "imm/mock_objects.h"

namespace ime_goopy {
namespace imm {
int MockImmLockPolicy::input_context_ref_ = 0;
INPUTCONTEXT MockImmLockPolicy::input_context_;

LPINPUTCONTEXT MockImmLockPolicy::ImmLockIMC(HIMC himc) {
  input_context_ref_++;
  return &input_context_;
}

BOOL MockImmLockPolicy::ImmUnlockIMC(HIMC himc) {
  EXPECT_GT(input_context_ref_, 0);
  input_context_ref_--;
  return TRUE;
}

LPVOID MockImmLockPolicy::ImmLockIMCC(HIMCC himcc) {
  MockImmLockPolicy::Component* component =
    reinterpret_cast<MockImmLockPolicy::Component*>(himcc);
  EXPECT_TRUE(component);
  component->ref++;
  return component->buffer;
}

BOOL MockImmLockPolicy::ImmUnlockIMCC(HIMCC himcc) {
  MockImmLockPolicy::Component* component =
    reinterpret_cast<MockImmLockPolicy::Component*>(himcc);
  EXPECT_TRUE(component);
  EXPECT_GT(component->ref, 0);
  component->ref--;
  return TRUE;
}

HIMCC MockImmLockPolicy::ImmCreateIMCC(DWORD size) {
  return CreateComponent(size);
}

HIMCC MockImmLockPolicy::ImmReSizeIMCC(HIMCC himcc, DWORD size) {
  MockImmLockPolicy::Component* component =
    reinterpret_cast<MockImmLockPolicy::Component*>(himcc);
  component->size = size;
  return himcc;
}

void MockImmLockPolicy::Reset() {
  input_context_ref_ = 0;
  ZeroMemory(&input_context_, sizeof(input_context_));
}

HIMCC MockImmLockPolicy::CreateComponent(int size) {
  Component* component = new Component;
  component->ref = 0;
  component->size = size;
  return reinterpret_cast<HIMCC>(component);
}

void MockImmLockPolicy::DestroyComponent(HIMCC himcc) {
  MockImmLockPolicy::Component* component =
    reinterpret_cast<MockImmLockPolicy::Component*>(himcc);
  delete component;
}

MockMessageQueue::MockMessageQueue(HIMC himc) {
  Reset();
}

MockMessageQueue::~MockMessageQueue() {
}

void MockMessageQueue::AddMessage(UINT message, WPARAM wparam, LPARAM lparam) {
  TRANSMSG transmsg = {message, wparam, lparam};
  messages_.push_back(transmsg);
}

bool MockMessageQueue::Send() {
  return true;
}

void MockMessageQueue::Attach(LPTRANSMSGLIST transmsg) {
  attach_called_ = true;
}

int MockMessageQueue::Detach() {
  detach_called_ = true;
  return 0;
}

void MockMessageQueue::Reset() {
  messages_.clear();
  attach_called_ = false;
  detach_called_ = false;
}

MockContext::MockContext(HIMC himc, MockMessageQueue* message_queue)
    : rect_from_composition_(false), rect_from_candidate_(false) {
}

MockContext::~MockContext() {
}

bool MockContext::GetCaretRectFromComposition(RECT* caret_rect) {
  *caret_rect = kTestRect;
  return rect_from_composition_;
}

bool MockContext::GetCaretRectFromCandidate(RECT* caret_rect) {
  *caret_rect = kTestRect;
  return rect_from_candidate_;
}

const CRect MockContext::kTestRect(100, 100, 200, 200);

}  // namespace imm
}  // namespace ime_goopy
