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

#include "ipc/message_util.h"

#include <google/protobuf/text_format.h>

#include "base/basictypes.h"
#include "base/logging.h"
#include "ipc/constants.h"
#include "ipc/message_types.h"

namespace ipc {

proto::Message* NewMessage(uint32 type,
                           uint32 source,
                           uint32 target,
                           uint32 icid,
                           bool need_reply) {
  proto::Message* message = new proto::Message();
  message->set_type(type);
  message->set_reply_mode(need_reply? proto::Message::NEED_REPLY :
                          proto::Message::NO_REPLY);
  message->set_source(source);
  message->set_target(target);
  message->set_icid(icid);
  return message;
}

bool MessageNeedReply(const proto::Message* message) {
  DCHECK(message);
  return message->reply_mode() == proto::Message::NEED_REPLY;
}

bool MessageIsReply(const proto::Message* message) {
  DCHECK(message);
  return message->reply_mode() == proto::Message::IS_REPLY;
}

bool MessageIsErrorReply(const proto::Message* message) {
  DCHECK(message);
  return (MessageIsReply(message) &&
      (message->payload().error().code() != proto::Error::NOT_ERROR));
}

std::string GetMessageErrorInfo(const proto::Message* message,
                                proto::Error::Code* error_code) {
  DCHECK(message);
  DCHECK(MessageIsErrorReply(message));
  if (error_code != NULL)
    *error_code = message->payload().error().code();
  return message->payload().error().message();
}

bool ConvertToReplyMessage(proto::Message* message) {
  DCHECK(message);
  DCHECK(message->icid() != kInputContextFocused);
  if (message->reply_mode() != proto::Message::NEED_REPLY)
    return false;

  message->set_reply_mode(proto::Message::IS_REPLY);
  uint32 old_target = message->target();
  message->set_target(message->source());
  message->set_source(old_target);
  return true;
}

bool ConvertToErrorReplyMessage(proto::Message* message,
                                proto::Error::Code error_code,
                                const char* error_message) {
  if (!ConvertToReplyMessage(message))
    return false;
  proto::MessagePayload* mutable_payload = message->mutable_payload();
  mutable_payload->Clear();
  proto::Error* error = mutable_payload->mutable_error();
  error->set_code(error_code);
  if (error_message && *error_message)
    error->set_message(error_message);
  return true;
}

bool ConvertToBooleanReplyMessage(proto::Message* message, bool value) {
  if (!ConvertToReplyMessage(message))
    return false;
  proto::MessagePayload* mutable_payload = message->mutable_payload();
  mutable_payload->Clear();
  mutable_payload->add_boolean(value);
  return true;
}

bool PrintMessageToString(const proto::Message& message,
                          std::string* text,
                          bool single_line) {
  DCHECK(text);
  text->clear();

  google::protobuf::TextFormat::Printer printer;
  // printer.SetUseUtf8StringEscaping(true);
  printer.SetSingleLineMode(single_line);

  // Don't output message name in single line mode.
  if (single_line)
    return printer.PrintToString(message, text);

  std::string output;
  if (!printer.PrintToString(message, &output))
    return false;

  const char* name = GetMessageName(message.type());
  text->append("# Start: ");
  text->append(name);
  text->append("\n");
  text->append(output);
  text->append("# End: ");
  text->append(name);
  text->append("\n");
  return true;
}

bool ParseMessageFromString(const std::string& text, proto::Message* message) {
  DCHECK(message);
  return google::protobuf::TextFormat::ParseFromString(text, message);
}

}  // namespace ipc
