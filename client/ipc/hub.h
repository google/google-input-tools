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

#ifndef GOOPY_IPC_HUB_H_
#define GOOPY_IPC_HUB_H_
#pragma once

#include "base/basictypes.h"

namespace ipc {

namespace proto {
class Message;
}

// An interface for implementing core logic of IPC layer. This interface does
// not have any external dependency except the protocol classes generated from
// protobuf definitions.
// The most important logic of this interface is to dispatch messages among all
// components.
// There should be only one Hub instance in a user desktop session.
class Hub {
 public:
  // An interface that others should implement in order to connect to the Hub.
  class Connector {
   public:
    // Send a message to its target component. This method must not be blocked.
    // The |message| is allocated using operator new, its ownership must be
    // taken by the Connector object and should be deleted once its contents has
    // been sent.
    // false should be returned if the message can not be sent due to any error.
    // The connector should delete the |message| object, before returning false.
    virtual bool Send(proto::Message* message) = 0;

    // Called when the connector is just attached to the Hub.
    virtual void Attached() {}

    // Called when the connector is just detached from the Hub.
    virtual void Detached() {}

   protected:
    virtual ~Connector() {}
  };

  virtual ~Hub() {}

  // Attach a Connector object to the Hub. Ownership of |connector| is not taken
  // by the Hub. The caller is responsible for destroying it after detaching it
  // from the hub by calling Detach() method. |connector|'s Attached() method
  // will be called synchronously by this method.
  virtual void Attach(Connector* connector) = 0;

  // Detach a Connector object from the hub. A connector must be detached from
  // the hub before being destroyed. |connector|'s Detached() method will be
  // called synchronously by this method.
  virtual void Detach(Connector* connector) = 0;

  // Ask the Hub to dispatch a message. The |message| must be allocated using
  // operator new, its ownership will be taken by the Hub and will be deleted
  // once it has been dispatched.
  // The |connector| must be attached to the Hub before calling this method.
  // When calling this method, the |connector|'s Send() method may be called
  // immediately to return the result to the connector, either an error message
  // or the actual result for a message handled by the Hub itself.
  // If any error occurs, for a message that does not need reply, false will be
  // returned. Otherwise an error message will be sent to the |connector| by
  // calling its Send() method and true will be returned by this method.
  virtual bool Dispatch(Connector* connector, proto::Message* message) = 0;
};

}  // namespace ipc

#endif  // GOOPY_IPC_HUB_H_
