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
#ifndef GOOPY_COMPONENTS_WIN_FRONTEND_FRONTEND_FACTORY_H_
#define GOOPY_COMPONENTS_WIN_FRONTEND_FRONTEND_FACTORY_H_

#include <map>
#include <vector>

#include "base/basictypes.h"
#include "base/singleton.h"
#include "common/framework_interface.h"
// TODO: Port this file from third_party.
#include "third_party/google_gadgets_for_linux/ggadget/win32/thread_local_singleton_holder.h"
#include "ipc/testing_prod.h"

namespace ime_goopy {

class EngineInterface;

// This class is responsible for creating new or returning existing
// EngineInterface for current thread. Another feature is providing shelving
// service.
//
// The reason why a frontend doesn't attach to a context id at beginning is
// that the context id may change during the life cycle of input context,
// the id is only used when a frontend instance is tranferred from an input
// context to another by shelving it by the former input context with its id,
// and unshelving it by the latter with the same id. during the transfering
// process, it is assumed that the ids of both input context keeps equal and
// do not change. if the assumption doesn't stand anymore, the the frontend
// would become a orphan one and could only be removed when the process quits.
class FrontendFactory {
 public:
  ~FrontendFactory();
  // Creates a engine.
  static EngineInterface* CreateFrontend();

  // Destroys a engine.
  static void DestroyFrontend(EngineInterface* frontend);

  // Shelves the engine temporarily which will be take over soon.
  // Returns true if success, false if not found.
  static bool ShelveFrontend(
      ContextInterface::ContextId id, EngineInterface* frontend);

  // Unshelves the engine identified by |id|, returns NULL if not
  // found.
  static EngineInterface* UnshelveFrontend(ContextInterface::ContextId id);

  // Convenient method for get a frontend from shelf or create a new one.
  // if id is |NULL| or not found, simply creates a new one.
  static EngineInterface* UnshelveOrCreateFrontend(
      ContextInterface::ContextId id);

 private:
  friend class ggadget::win32::ThreadLocalSingletonHolder<FrontendFactory>;
  FRIEND_TEST(FrontendFactoryTest, BaseTest);

  static FrontendFactory* GetThreadLocalInstance();

  void PurgeShelvedFrontends();

  // All created frontends.
  std::vector<EngineInterface*> frontends_;

  typedef std::map<ContextInterface::ContextId /* unique id of context */,
          EngineInterface* /* created engine */> ShelvedFrontends;
  // All shelved frontends.
  ShelvedFrontends shelved_frontends_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(FrontendFactory);
};

}  // namespace ime_goopy
#endif  // GOOPY_COMPONENTS_WIN_FRONTEND_FRONTEND_FACTORY_H_
