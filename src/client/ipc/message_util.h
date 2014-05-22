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

#ifndef GOOPY_IPC_MESSAGE_UTIL_H_
#define GOOPY_IPC_MESSAGE_UTIL_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "ipc/protos/ipc.pb.h"

namespace ipc {

namespace proto {
class Message;
}

// News a message with specified attributes.
proto::Message* NewMessage(uint32 type,
                           uint32 source,
                           uint32 target,
                           uint32 icid,
                           bool need_reply);

// Checks if a message needs a reply message.
bool MessageNeedReply(const proto::Message* message);

// Checks if a message is a reply message.
bool MessageIsReply(const proto::Message* message);

// Checks if a message is a reply message with error code.
bool MessageIsErrorReply(const proto::Message* message);

// Gets the error code information of an error reply message.
std::string GetMessageErrorInfo(const proto::Message* message,
                                proto::Error::Code* error_code);

// Converts a given |message| object into a reply message by setting its reply
// mode to IS_REPLY and swapping the source and target component.
// Returns true if success, or false if the |message|'s reply_mode is not
// NEED_REPLY.
bool ConvertToReplyMessage(proto::Message* message);

// Converts a given |message| object into a reply message representing an error.
// Returns true if success, or false if the |message|'s reply_mode is not
// NEED_REPLY.
bool ConvertToErrorReplyMessage(proto::Message* message,
                                proto::Error::Code error_code,
                                const char* error_message);

// Converts a given |message| object into a reply message containing a bool.
// Returns true if success, or false if the |message|'s reply_mode is not
// NEED_REPLY.
bool ConvertToBooleanReplyMessage(proto::Message* message, bool value);

// Prints a message to a string in text format. Returns true if the message is
// printed successfully.
bool PrintMessageToString(const proto::Message& message,
                          std::string* text,
                          bool single_line);

// Parses a text-format message string to the given message object.
// Returns true if the text is parsed successfully.
bool ParseMessageFromString(const std::string& text, proto::Message* message);

}  // namespace ipc

#endif  // GOOPY_IPC_MESSAGE_UTIL_H_
