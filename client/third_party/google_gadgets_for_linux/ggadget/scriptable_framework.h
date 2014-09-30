/*
  Copyright 2011 Google Inc.

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

#ifndef GGADGET_SCRIPTABLE_FRAMEWORK_H__
#define GGADGET_SCRIPTABLE_FRAMEWORK_H__

#include <ggadget/scriptable_helper.h>

namespace ggadget {

class GadgetInterface;

namespace framework {

class AudioInterface;
class RuntimeInterface;
class MachineInterface;
class MemoryInterface;
class NetworkInterface;
class PerfmonInterface;
class PowerInterface;
class ProcessInterface;
class WirelessInterface;
class CursorInterface;
class ScreenInterface;
class UserInterface;

/**
 * @ingroup ScriptableObjects
 * @{
 */

/**
 * Scriptable counterpart of AudioInterface.
 *
 * Please note that ScriptableAudio is not NativeOwned, because it's bound to
 * a Gadget instance. Different Gadget must use different ScriptableAudio
 * instance.
 *
 * A framework extension must create a new ScriptableAudio instance and attach
 * it to the specified framework instance when its RegisterFrameworkExtension()
 * function is called each time.  So the ScriptableAudio object will be
 * destroyed correctly when the associated framework instance is destroyed by
 * the corresponding gadget.
 *
 * While each Gadget has its own ScriptableAudio object, all ScriptableAudio
 * objects can share one AudioInterface instance. So the specified audio
 * pointer can point to a static allocated AudioInterface instance.
 * ScriptableAudio object will never delete the audio pointer.
 */
class ScriptableAudio : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x7f460413b19241fe, ScriptableInterface);

  ScriptableAudio(AudioInterface *audio, GadgetInterface *gadget);
  virtual ~ScriptableAudio();

 protected:
  virtual void DoRegister();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableAudio);

  class Impl;
  Impl *impl_;
};

/** Scriptable counterpart of RuntimeInterface. */
class ScriptableRuntime : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x83df98ced129f243, ScriptableInterface);

  ScriptableRuntime(RuntimeInterface *runtime);
  virtual ~ScriptableRuntime();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableRuntime);
};

/**
 * Scriptable counterpart of PerfmonInterface.
 *
 * Please note that ScriptablePerfmon is not NativeOwned, because it's bound to
 * a Gadget instance. Different Gadget must use different ScriptablePerfmon
 * instance.
 *
 * A framework extension must create a new ScriptablePerfmon instance and attach
 * it to the specified framework instance when its RegisterFrameworkExtension()
 * function is called each time.  So the ScriptablePerfmon object will be
 * destroyed correctly when the associated framework instance is destroyed by
 * the corresponding gadget.
 *
 * While each Gadget has its own ScriptablePerfmon object, all ScriptablePerfmon
 * objects can share one PerfmonInterface instance. So the specified perfmon
 * pointer can point to a static allocated PerfmonInterface instance.
 * ScriptablePerfmon object will not delete the perfmon pointer upon destroying.
 */
class ScriptablePerfmon : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x07495B8910EE4BCC, ScriptableInterface);

  ScriptablePerfmon(PerfmonInterface *perfmon, GadgetInterface *gadget);
  virtual ~ScriptablePerfmon();

 protected:
  virtual void DoRegister();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptablePerfmon);

  class Impl;
  Impl *impl_;
};

/** Scriptable counterpart of NetworkInterface */
class ScriptableNetwork : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xF64768F323CB4FB3, ScriptableInterface);

  explicit ScriptableNetwork(NetworkInterface *network);
  virtual ~ScriptableNetwork();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableNetwork);

  class Impl;
  Impl *impl_;
};

/** Scriptable counterpart of ProcessInterface*/
class ScriptableProcess : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x838F203231104C25, ScriptableInterface);

  explicit ScriptableProcess(ProcessInterface *process);
  virtual ~ScriptableProcess();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableProcess);

  class Impl;
  Impl *impl_;
};

/** Scriptable counterpart of PowerInterface*/
class ScriptablePower : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x75E9FA8DCD644336, ScriptableInterface);

  explicit ScriptablePower(PowerInterface *power);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptablePower);
};

/** Scriptable counterpart of MemoryInterface*/
class ScriptableMemory : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x818FD51E538C46F9, ScriptableInterface);

  explicit ScriptableMemory(MemoryInterface *memory);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableMemory);
};

/** Scriptable counterpart of MachineInterface (BIOS part) */
class ScriptableBios : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xE0583342338C41AA, ScriptableInterface);

  explicit ScriptableBios(MachineInterface *machine);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableBios);
};


/** Scriptable counterpart of MachineInterface (Machine part) */
class ScriptableMachine : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xFF025C614F424D90, ScriptableInterface);

  explicit ScriptableMachine(MachineInterface *machine);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableMachine);
};

/** Scriptable counterpart of MachineInterface (Processor part) */
class ScriptableProcessor : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x179B736C8B78472B, ScriptableInterface);

  explicit ScriptableProcessor(MachineInterface *machine);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableProcessor);
};

/** Scriptable counterpart of CursorInterface */
class ScriptableCursor : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x1692D8615B2642A9, ScriptableInterface);

  explicit ScriptableCursor(CursorInterface *cursor);
  virtual ~ScriptableCursor();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableCursor);
  class Impl;
  Impl *impl_;
};

/** Scriptable counterpart of ScreenInterface */
class ScriptableScreen : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xA03F7A40B6F74178, ScriptableInterface);

  explicit ScriptableScreen(ScreenInterface *screen);
  virtual ~ScriptableScreen();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableScreen);
  class Impl;
  Impl *impl_;
};

/** Scriptable counterpart of UserInterface */
class ScriptableUser : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x458D248CFD23117B, ScriptableInterface);

  explicit ScriptableUser(UserInterface *user);
  virtual ~ScriptableUser();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableUser);
};

/**
 * Handy class of framework.graphics.
 *
 * It's registered in default_framework extension.
 * Using shared scriptable, because it's per gadget.
 */
class ScriptableGraphics : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x211b114e852e4a1b, ScriptableInterface);

  explicit ScriptableGraphics(GadgetInterface *gadget);
  virtual ~ScriptableGraphics();

 protected:
  virtual void DoRegister();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ScriptableGraphics);
  class Impl;
  Impl *impl_;
};

/** @} */

} // namespace framework
} // namespace ggadget

#endif  // GGADGET_SCRIPTABLE_FRAMEWORK_H__
