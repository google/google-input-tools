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

#include "components/keyboard_input/keyboard_input_component.h"

#include <string>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/stl_util.h"
#include "components/common/constants.h"
#include "components/common/file_utils.h"
#include "components/common/keystroke_util.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"
#include "ipc/protos/ipc.pb.h"

namespace {

// The message types keyboard input ime can consume.
const uint32 kConsumeMessages[] = {
  // Input context related messages.
  ipc::MSG_ATTACH_TO_INPUT_CONTEXT,
  ipc::MSG_PROCESS_KEY_EVENT,
  // Composition related messages.
  ipc::MSG_CANCEL_COMPOSITION,
  ipc::MSG_COMPLETE_COMPOSITION,
};

const uint32 kProduceMessages[] = {
  ipc::MSG_INSERT_TEXT,
};

static const char kEnglishImeLanguage[] = "en";
static const char kEnglishImeIcon[]  = "english.png";
static const char kEnglishImeOverIcon[]  = "english_over.png";
static const char kResourcePackPathPattern[] = "/keyboard_input_[LANG].pak";
}  // namespace
namespace ime_goopy {
namespace components {

using ipc::proto::Message;


KeyboardInputComponent::KeyboardInputComponent() {
}

KeyboardInputComponent::~KeyboardInputComponent() {
}

void KeyboardInputComponent::GetInfo(ipc::proto::ComponentInfo* info) {
  info->set_string_id(kKeyboardInputComponentStringId);
  info->add_language(kEnglishImeLanguage);
  for (int i = 0; i < arraysize(kConsumeMessages); ++i)
    info->add_consume_message(kConsumeMessages[i]);
  for (int i = 0; i < arraysize(kProduceMessages); ++i)
    info->add_produce_message(kProduceMessages[i]);
  GetSubComponentsInfo(info);
  std::string dir = FileUtils::GetDataPathForComponent(
      kKeyboardInputComponentStringId);
  std::string filename = dir + "/" + kEnglishImeOverIcon;
  ipc::proto::IconGroup icon;
  if (!FileUtils::ReadFileContent(filename,
                                  icon.mutable_over()->mutable_data())) {
    icon.clear_over();
  }
  filename = dir + "/" + kEnglishImeIcon;
  if (FileUtils::ReadFileContent(filename,
                                 icon.mutable_normal()->mutable_data())) {
    info->mutable_icon()->CopyFrom(icon);
  }
  std::string name = "English";
  info->set_name(name.c_str());
}

void KeyboardInputComponent::Handle(Message* message) {
  DCHECK(id());
  // This message is consumed by sub components.
  if (HandleMessageBySubComponents(message))
    return;
  scoped_ptr<Message> mptr(message);

  switch (mptr->type()) {
    case ipc::MSG_ATTACH_TO_INPUT_CONTEXT:
      OnMsgAttachToInputContext(mptr.release());
      break;
    case ipc::MSG_PROCESS_KEY_EVENT:
      OnMsgProcessKey(mptr.release());
      break;
    case ipc::MSG_CANCEL_COMPOSITION:
      // ReplyTrue in case the source component uses SendWithReply.
      ReplyTrue(mptr.release());
      break;
    case ipc::MSG_COMPLETE_COMPOSITION:
      ReplyTrue(mptr.release());
      break;
    default: {
      DLOG(ERROR) << "Unexpected message received:"
                  << "type = " << mptr->type()
                  << "icid = " << mptr->icid();
      ReplyError(mptr.release(), ipc::proto::Error::INVALID_MESSAGE,
                 "unknown type");
      break;
    }
  }
}

void KeyboardInputComponent::OnMsgAttachToInputContext(Message* message) {
  scoped_ptr<Message> mptr(message);
  DCHECK_EQ(mptr->reply_mode(), Message::NEED_REPLY);
  ReplyTrue(mptr.release());
}

void KeyboardInputComponent::OnMsgProcessKey(Message* message) {
  ReplyFalse(message);
}

}  // namespace components
}  // namespace ime_goopy
