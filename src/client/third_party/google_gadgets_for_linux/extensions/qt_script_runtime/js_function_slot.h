/*
  Copyright 2007 Google Inc.

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

#ifndef GGADGET_QT_JS_FUNCTION_SLOT_H__
#define GGADGET_QT_JS_FUNCTION_SLOT_H__

#include <map>
#include <string>
#include <QtScript/QScriptEngine>
#include <ggadget/common.h>
#include <ggadget/slot.h>

namespace ggadget {
namespace qt {

/**
 * A Slot that wraps a JavaScript function object.
 */
class JSFunctionSlot : public Slot {
 public:
  JSFunctionSlot(const Slot *prototype, QScriptEngine *engine,
                 const char *script, const char *file_name, int lineno);
  JSFunctionSlot(const Slot *prototype,
                 QScriptEngine *engine, QScriptValue function);
  virtual ~JSFunctionSlot();

  virtual ResultVariant Call(ScriptableInterface *object,
                             int argc, const Variant argv[]) const;

  virtual bool HasMetadata() const { return prototype_ != NULL; }
  virtual Variant::Type GetReturnType() const {
    return prototype_ ? prototype_->GetReturnType() : Variant::TYPE_VARIANT;
  }
  virtual int GetArgCount() const {
    return prototype_ ? prototype_->GetArgCount() : 0;
  }
  virtual const Variant::Type *GetArgTypes() const {
    return prototype_ ? prototype_->GetArgTypes() : NULL;
  }
  virtual bool operator==(const Slot &another) const {
    return this == &another;
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JSFunctionSlot);

  class QtObject;
  QtObject *q_obj_;
  const Slot *prototype_;
  QScriptEngine *engine_;
  bool code_; // true if set by src code
  QString script_;
  std::string file_name_;
  int line_no_;
  QScriptValue function_;

  // This slot object may be deleted during Call(). In Call(), This pointer
  // should point to a local bool variable, and once *death_flag_ptr_ becomes
  // true, Call() should return immediately.
  mutable bool *death_flag_ptr_;
};

class JSFunctionSlot::QtObject : public QObject {
  Q_OBJECT
 public:
  QtObject(QScriptEngine *engine) : valid_(true) {
    connect(engine, SIGNAL(destroyed()),
            this, SLOT(OnScriptEngineDestroyed()));
  }
  bool valid_;
 public slots:
  void OnScriptEngineDestroyed() {
    valid_ = false;
  }
};


} // namespace qt
} // namespace ggadget

#endif  // GGADGET_QT_JS_FUNCTION_SLOT_H__
