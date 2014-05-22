/*
   Copyright 2008 Google Inc.

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

#include "dbus_proxy.h"
#include "dbus_utils.h"

#include <dbus/dbus.h>
#include <ggadget/common.h>
#include <ggadget/logger.h>
#include <ggadget/main_loop_interface.h>
#include <ggadget/scriptable_array.h>
#include <ggadget/signals.h>
#include <ggadget/slot.h>
#include <ggadget/string_utils.h>

#ifdef _DEBUG
// #define DBUS_VERBOSE_LOG
#endif

namespace ggadget {
namespace dbus {

static int MessageTypeToDBusType(MessageType type) {
  static const int kDBusTypes[] = {
    DBUS_TYPE_INVALID,
    DBUS_TYPE_BYTE,
    DBUS_TYPE_BOOLEAN,
    DBUS_TYPE_INT16,
    DBUS_TYPE_UINT16,
    DBUS_TYPE_INT32,
    DBUS_TYPE_UINT32,
    DBUS_TYPE_INT64,
    DBUS_TYPE_UINT64,
    DBUS_TYPE_DOUBLE,
    DBUS_TYPE_STRING,
    DBUS_TYPE_OBJECT_PATH,
    DBUS_TYPE_SIGNATURE,
    DBUS_TYPE_ARRAY,
    DBUS_TYPE_STRUCT,
    DBUS_TYPE_VARIANT,
    DBUS_TYPE_DICT_ENTRY
  };

  return (type >= MESSAGE_TYPE_INVALID && type <= MESSAGE_TYPE_DICT) ?
         kDBusTypes[type - MESSAGE_TYPE_INVALID] : DBUS_TYPE_INVALID;
}

static bool IsBasicType(const std::string &s) {
  return s.length() == 1 && dbus_type_is_basic(static_cast<int>(s[0]));
}

static std::string GetElementType(const char *signature) {
  if (!signature) return "";
  if (*signature == DBUS_TYPE_ARRAY)
    return std::string(DBUS_TYPE_ARRAY_AS_STRING) +
        GetElementType(signature + 1);
  // don't consider such invalid cases: {is(s}.
  // it is user's reponsibility to make them right.
  if (*signature == DBUS_STRUCT_BEGIN_CHAR ||
      *signature == DBUS_DICT_ENTRY_BEGIN_CHAR) {
    const char *index = signature;
    char start = *signature;
    char end = static_cast<char>(start == DBUS_STRUCT_BEGIN_CHAR ?
                                 DBUS_STRUCT_END_CHAR :
                                 DBUS_DICT_ENTRY_END_CHAR);
    int counter = 1;
    while (counter != 0) {
      char ch = *++index;
      if (!ch) return "";
      if (ch == start)
        counter++;
      else if (ch == end)
        counter--;
    }
    return std::string(signature, static_cast<size_t>(index - signature + 1));
  }
  return std::string(signature, 1);
}

// used for container type except array.
static bool GetSubElements(const std::string &signature,
                           StringVector *sig_list) {
  if (IsBasicType(signature) || signature[0] == DBUS_TYPE_ARRAY)
    return false;
  StringVector tmp_list;
  const char *begin = signature.c_str() + 1;
  const char *end = signature.c_str() + signature.length();
  while (begin < end - 1) {
    std::string sig = GetElementType(begin);
    if (sig.empty()) return false;
    tmp_list.push_back(sig);
    begin += sig.length();
  }
  sig_list->swap(tmp_list);
  return sig_list->size();
}

// Checks if a type signature is valid or not.
static bool ValidateSignature(const char *signature, bool single) {
  DBusError error;
  dbus_error_init(&error);
  bool ret = true;
  if ((single && !dbus_signature_validate_single(signature, &error)) ||
      !dbus_signature_validate(signature, &error)) {
    DLOG("Failed to check validity for signature %s, %s: %s",
        signature, error.name, error.message);
    ret = false;
  }
  dbus_error_free(&error);
  return ret;
}

#define VALID_INITIAL_NAME_CHAR(c) \
  (((c) >= 'A' && (c) <= 'Z') ||   \
   ((c) >= 'a' && (c) <= 'z') ||   \
   ((c) == '_') )

#define VALID_NAME_CHAR(c)       \
  (((c) >= '0' && (c) <= '9') || \
   ((c) >= 'A' && (c) <= 'Z') || \
   ((c) >= 'a' && (c) <= 'z') || \
   ((c) == '_'))

// Checks if an object path is valid or not.
bool ValidateObjectPath(const char *path) {
  if (!path || *path != '/')
    return false;

  const char *s = path;
  const char *last_slash = s++;
  while (*s) {
    if (*s == '/') {
      // Empty path component is not allowed.
      if ((s - last_slash) < 2)
        return false;
      last_slash = s;
    } else if (!VALID_NAME_CHAR(*s)) {
      return false;
    }
    ++s;
  }

  // Trailing slash is not allowed unless the path is "/".
  if ((s - last_slash) < 2 && (s - path) > 1)
    return false;

  return true;
}

bool ValidateInterface(const char *interface) {
  // Interface can't start with '.'
  if (!interface || !*interface || *interface == '.' ||
      !VALID_INITIAL_NAME_CHAR(*interface))
    return false;

  const char *s = interface;
  const char *last_dot = NULL;
  while(*s) {
    if (*s == '.') {
      if (*(s + 1) == '\0' || !VALID_INITIAL_NAME_CHAR(*(s + 1)))
        return false;
      last_dot = s;
      ++s;
    } else if (!VALID_NAME_CHAR(*s)) {
      return false;
    }
    ++s;
  }
  return last_dot != NULL;
}

#define VALID_INITIAL_BUS_NAME_CHAR(c) \
  (((c) >= 'A' && (c) <= 'Z') ||       \
   ((c) >= 'a' && (c) <= 'z') ||       \
   ((c) == '_') || ((c) == '-'))

#define VALID_BUS_NAME_CHAR(c)   \
  (((c) >= '0' && (c) <= '9') || \
   ((c) >= 'A' && (c) <= 'Z') || \
   ((c) >= 'a' && (c) <= 'z') || \
   ((c) == '_') || ((c) == '-'))

bool ValidateBusName(const char *name) {
  const char *s = name;
  const char *last_dot = NULL;
  if (*s == ':') {
    ++s;
    while (*s) {
      if (*s == '.') {
        if (*(s + 1) == '\0' || !VALID_BUS_NAME_CHAR(*(s + 1)))
          return false;
        ++s;
      } else if (!VALID_BUS_NAME_CHAR(*s)) {
        return false;
      }
      ++s;
    }
    return true;
  } else if (*s == '.') {
    return false;
  } else if (!VALID_INITIAL_BUS_NAME_CHAR(*s)) {
    return false;
  } else {
    ++s;
  }

  while (*s) {
    if (*s == '.') {
      if (*(s + 1) == '\0' || !VALID_INITIAL_BUS_NAME_CHAR(*(s + 1)))
        return false;
      last_dot = s;
      ++s;
    } else if (!VALID_BUS_NAME_CHAR(*s)) {
      return false;
    }
    ++s;
  }
  return last_dot != NULL;
}

// Helper class to get type signature of a scriptable array.
// Used in GetVariantSignature().
class ArraySignatureIterator {
 public:
  ArraySignatureIterator() : is_array_(true) {}
  std::string GetSignature() const {
    if (signature_list_.empty()) return "";
    if (is_array_)
      return std::string(DBUS_TYPE_ARRAY_AS_STRING) + signature_list_[0];
    std::string sig = DBUS_STRUCT_BEGIN_CHAR_AS_STRING;
    for (StringVector::const_iterator it = signature_list_.begin();
         it != signature_list_.end(); ++it)
      sig += *it;
    sig += DBUS_STRUCT_END_CHAR_AS_STRING;
    return sig;
  }
  bool Callback(int id, const Variant &value) {
    GGL_UNUSED(id);
    std::string sig = GetVariantSignature(value);
    if (sig.empty()) return true;
    if (is_array_ && !signature_list_.empty() && sig != signature_list_[0])
      is_array_ = false;
    signature_list_.push_back(sig);
    return true;
  }
 private:
  bool is_array_;
  StringVector signature_list_;
};

// Helper class to get type signature of a scriptable dictionary.
// Used in GetVariantSignature().
class DictSignatureIterator {
 public:
  std::string GetSignature() const { return signature_; }
  bool Callback(const char *name, ScriptableInterface::PropertyType type,
                const Variant &value) {
    GGL_UNUSED(name);
    if (type == ScriptableInterface::PROPERTY_METHOD ||
        value.type() == Variant::TYPE_VOID) {
      // Ignore method and void type properties.
      return true;
    }
    std::string sig = GetVariantSignature(value);
    if (signature_.empty()) {
      signature_ = sig;
    } else if (signature_ != sig) {
      return false;
    }
    return true;
  }
 private:
  std::string signature_;
};

// Gets type signature of a Variant.
std::string GetVariantSignature(const Variant &value) {
  switch (value.type()) {
    case Variant::TYPE_BOOL:
      return DBUS_TYPE_BOOLEAN_AS_STRING;
    case Variant::TYPE_INT64:
      return DBUS_TYPE_INT32_AS_STRING;
    case Variant::TYPE_DOUBLE:
      return DBUS_TYPE_DOUBLE_AS_STRING;
    case Variant::TYPE_STRING:
    case Variant::TYPE_UTF16STRING:
    case Variant::TYPE_JSON:
      return DBUS_TYPE_STRING_AS_STRING;
    case Variant::TYPE_SCRIPTABLE:
      {
        ScriptableInterface *scriptable =
            VariantValue<ScriptableInterface*>()(value);
        if (!scriptable)
          return "";
        Variant length = scriptable->GetProperty("length").v();
        if (length.type() != Variant::TYPE_VOID) {
          /* firstly treat as an array. */
          ArraySignatureIterator iterator;
          if(!scriptable->EnumerateElements(
              NewSlot(&iterator, &ArraySignatureIterator::Callback))) {
            DLOG("Failed to get array signature.");
          } else {
            std::string sig = iterator.GetSignature();
            if (!sig.empty()) return sig;
          }
        }
        DictSignatureIterator iterator;
        if (!scriptable->EnumerateProperties(
            NewSlot(&iterator, &DictSignatureIterator::Callback))) {
          DLOG("Failed to get dict signature.");
          return "";
        }
        std::string dict_signature(DBUS_TYPE_ARRAY_AS_STRING);
        dict_signature += DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING;
        dict_signature += DBUS_TYPE_STRING_AS_STRING;
        dict_signature += iterator.GetSignature();
        dict_signature += DBUS_DICT_ENTRY_END_CHAR_AS_STRING;
        return dict_signature;
      }
      break;
    default:
     DLOG("Unsupported Variant type %d for DBus.", value.type());
  }
  return "";
}

Variant::Type GetVariantTypeFromSignature(const std::string &signature) {
  switch (signature[0]) {
    case DBUS_TYPE_BYTE:
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_UINT16:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT64:
      return Variant::TYPE_INT64;
    case DBUS_TYPE_BOOLEAN:
      return Variant::TYPE_BOOL;
    case DBUS_TYPE_DOUBLE:
      return Variant::TYPE_DOUBLE;
    case DBUS_TYPE_STRING:
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_SIGNATURE:
      return Variant::TYPE_STRING;
    case DBUS_TYPE_VARIANT:
      return Variant::TYPE_VARIANT;
    case DBUS_TYPE_ARRAY:
    case DBUS_TYPE_STRUCT:
    case DBUS_TYPE_DICT_ENTRY:
    case DBUS_STRUCT_BEGIN_CHAR:
    case DBUS_DICT_ENTRY_BEGIN_CHAR:
      return Variant::TYPE_SCRIPTABLE;
    default:
      DLOG("Can't convert DBus type %s to Variant type.", signature.c_str());
  }
  return Variant::TYPE_VOID;
}

class DBusMarshaller::Impl {
 public:
  Impl(DBusMessage *message) : iter_(NULL), parent_iter_(NULL) {
    iter_ = new DBusMessageIter;
    dbus_message_iter_init_append(message, iter_);
  }
  Impl(DBusMessageIter *parent, int type, const char *sig) :
      iter_(NULL), parent_iter_(parent) {
    iter_ = new DBusMessageIter;
    dbus_message_iter_open_container(parent, type, sig, iter_);
  }
  ~Impl() {
    if (parent_iter_)
      dbus_message_iter_close_container(parent_iter_, iter_);
    delete iter_;
  }
  bool AppendArgument(const Argument &arg) {
    if (arg.signature.empty()) {
      std::string sig = GetVariantSignature(arg.value.v());
      if (sig.empty()) return false;
      return AppendArgument(Argument(sig, arg.value));
    }
    const char* index = arg.signature.c_str();
    if (!ValidateSignature(index, true)) return false;
    switch (*index) {
      case DBUS_TYPE_BYTE:
        {
          int64_t i;
          if (!arg.value.v().ConvertToInt64(&i)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_INT64, arg.value.v().Print().c_str());
            return false;
          }
          AppendByte(static_cast<unsigned char>(i));
          break;
        }
      case DBUS_TYPE_BOOLEAN:
        {
          bool b;
          if (!arg.value.v().ConvertToBool(&b)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_BOOL, arg.value.v().Print().c_str());
            return false;
          }
          AppendBoolean(b);
          break;
        }
      case DBUS_TYPE_INT16:
        {
          int64_t i;
          if (!arg.value.v().ConvertToInt64(&i)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_INT64, arg.value.v().Print().c_str());
            return false;
          }
          AppendInt16(static_cast<int16_t>(i));
          break;
        }
      case DBUS_TYPE_UINT16:
        {
          int64_t i;
          if (!arg.value.v().ConvertToInt64(&i)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_INT64, arg.value.v().Print().c_str());
            return false;
          }
          AppendUInt16(static_cast<uint16_t>(i));
          break;
        }
      case DBUS_TYPE_INT32:
        {
          int64_t i;
          if (!arg.value.v().ConvertToInt64(&i)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_INT64, arg.value.v().Print().c_str());
            return false;
          }
          AppendInt32(static_cast<int32_t>(i));
          break;
        }
      case DBUS_TYPE_UINT32:
        {
          int64_t i;
          if (!arg.value.v().ConvertToInt64(&i)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_INT64, arg.value.v().Print().c_str());
            return false;
          }
          AppendUInt32(static_cast<uint32_t>(i));
          break;
        }
      case DBUS_TYPE_INT64:
        {
          int64_t v;
          if (!arg.value.v().ConvertToInt64(&v)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_INT64, arg.value.v().Print().c_str());
            return false;
          }
          AppendInt64(v);
          break;
        }
      case DBUS_TYPE_UINT64:
        {
          int64_t i;
          if (!arg.value.v().ConvertToInt64(&i)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_INT64, arg.value.v().Print().c_str());
            return false;
          }
          AppendUInt64(static_cast<uint64_t>(i));
          break;
        }
      case DBUS_TYPE_DOUBLE:
        {
          double v;
          if (!arg.value.v().ConvertToDouble(&v)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_DOUBLE, arg.value.v().Print().c_str());
            return false;
          }
          AppendDouble(v);
          break;
        }
      case DBUS_TYPE_STRING:
        {
          std::string s;
          if (!arg.value.v().ConvertToString(&s)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_STRING, arg.value.v().Print().c_str());
            return false;
          }
          AppendString(s.c_str());
          break;
        }
      case DBUS_TYPE_OBJECT_PATH:
        {
          std::string path;
          if (!arg.value.v().ConvertToString(&path) ||
              !ValidateObjectPath(path.c_str())) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_STRING, arg.value.v().Print().c_str());
            return false;
          }
          AppendObjectPath(path.c_str());
          break;
        }
      case DBUS_TYPE_SIGNATURE:
        {
          std::string sig;
          if (!arg.value.v().ConvertToString(&sig) ||
              !ValidateSignature(sig.c_str(), false)) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_STRING, arg.value.v().Print().c_str());
            return false;
          }
          AppendSignature(sig.c_str());
          break;
        }
      case DBUS_TYPE_ARRAY:
        {
          if (arg.value.v().type() != Variant::TYPE_SCRIPTABLE) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_SCRIPTABLE, arg.value.v().Print().c_str());
            return false;
          }
          // handle dict case specially
          if (*(index + 1) == DBUS_DICT_ENTRY_BEGIN_CHAR) {
            std::string dict_sig = GetElementType(index + 1);
            StringVector sig_list;
            if (!GetSubElements(dict_sig.c_str(), &sig_list) ||
                sig_list.size() != 2 || !IsBasicType(sig_list[0])) {
              DLOG("Invalid dict type: %s.", dict_sig.c_str());
              return false;
            }
            ScriptableInterface *dict =
                VariantValue<ScriptableInterface*>()(arg.value.v());
            if (!dict) {
              DLOG("Dict is NULL");
              return false;
            }
            Impl *sub = new Impl(iter_, DBUS_TYPE_ARRAY, dict_sig.c_str());
            DictMarshaller slot(sub, sig_list[0], sig_list[1]);
            if (!dict->EnumerateProperties(
                NewSlot(&slot, &DictMarshaller::Callback))) {
              DLOG("Failed to marshal dict: %s.", dict_sig.c_str());
              return false;
            }
            index += dict_sig.size();
          } else {
            std::string signature = GetElementType(index + 1);
            Impl *sub = new Impl(iter_, DBUS_TYPE_ARRAY, signature.c_str());
            ScriptableInterface *array =
                VariantValue<ScriptableInterface*>()(arg.value.v());
            if (!array) {
              DLOG("Array is NULL");
              return false;
            }
            ArrayMarshaller slot(sub, signature);
            if (!array->EnumerateElements(
                NewSlot(&slot, &ArrayMarshaller::Callback))) {
              DLOG("Failed to marshal array: %s.", signature.c_str());
              return false;
            }
            index += signature.size();
          }
          break;
        }
      case DBUS_STRUCT_BEGIN_CHAR:
      // normally, {} should not exist without a.
      // We add it here just for safety.
      case DBUS_DICT_ENTRY_BEGIN_CHAR:
        {
          if (arg.value.v().type() != Variant::TYPE_SCRIPTABLE) {
            DLOG("Type mismatch. Expected type: %d, actual value:%s",
                 Variant::TYPE_SCRIPTABLE, arg.value.v().Print().c_str());
            return false;
          }
          StringVector sig_list;
          std::string struct_signature = GetElementType(index);
          if (!GetSubElements(struct_signature.c_str(), &sig_list)) {
            DLOG("Invalid structure type: %s", struct_signature.c_str());
            return false;
          }
          Impl *sub = new Impl(iter_, *index == DBUS_STRUCT_BEGIN_CHAR ?
                               DBUS_TYPE_STRUCT : DBUS_TYPE_DICT_ENTRY, NULL);
          ScriptableInterface *structure =
              VariantValue<ScriptableInterface*>()(arg.value.v());
          if (!structure) {
            DLOG("Structure is NULL");
            return false;
          }
          StructMarshaller slot(sub, sig_list);
          if (!structure->EnumerateElements(
              NewSlot(&slot, &StructMarshaller::Callback))) {
            DLOG("Failed to marshal struct: %s", struct_signature.c_str());
            return false;
          }
          index += struct_signature.size() - 1;
          break;
        }
      case DBUS_TYPE_VARIANT:
        {
          std::string sig = GetVariantSignature(arg.value.v());
          Impl sub(iter_, DBUS_TYPE_VARIANT, sig.c_str());
          if (!sub.AppendArgument(Argument(sig, arg.value)))
            return false;
          break;
        }
      default:
        DLOG("Unsupported type: %s", index);
        return false;
    }
    if (index != arg.signature.c_str() + arg.signature.size() - 1)
      return false;
    return true;
  }
  static bool ValistAdaptor(Arguments *in_args, MessageType first_arg_type,
                            va_list *va_args) {
    Arguments tmp_args;
    while (first_arg_type != MESSAGE_TYPE_INVALID) {
      Argument arg;
      if (!ValistItemAdaptor(&arg, first_arg_type, va_args))
        return false;
      tmp_args.push_back(arg);
      first_arg_type = static_cast<MessageType>(va_arg(*va_args, int));
    }
    in_args->swap(tmp_args);
    return true;
  }
 private:
  class ArrayMarshaller {
   public:
    ArrayMarshaller(Impl *impl, const std::string &sig)
      : marshaller_(impl), signature_(sig) {
    }
    ~ArrayMarshaller() {
      delete marshaller_;
    }
    bool Callback(int id, const Variant &value) {
      GGL_UNUSED(id);
      Argument arg(signature_, value);
      return marshaller_->AppendArgument(arg);
    }
   private:
    Impl *marshaller_;
    std::string signature_;
  };

  class StructMarshaller {
   public:
    StructMarshaller(Impl *impl, const StringVector& signatures)
      : marshaller_(impl), signature_list_(signatures), index_(0) {
    }
    ~StructMarshaller() {
      delete marshaller_;
    }
    bool Callback(int id, const Variant &value) {
      GGL_UNUSED(id);
      if (index_ >= signature_list_.size()) {
        DLOG("The signature of the variant does not match the specified "
             "signature.");
        return false;
      }
      Argument arg(signature_list_[index_], value);
      index_++;
      return marshaller_->AppendArgument(arg);
    }
   private:
    Impl *marshaller_;
    const StringVector &signature_list_;
    std::string::size_type index_;
  };

  class DictMarshaller {
   public:
    DictMarshaller(Impl *impl, const std::string &key_signature,
                   const std::string &value_signature)
      : marshaller_(impl),
        key_signature_(key_signature),
        value_signature_(value_signature) {
    }
    ~DictMarshaller() {
      delete marshaller_;
    }
    bool Callback(const char *name, ScriptableInterface::PropertyType type,
                  const Variant &value) {
      if (type == ScriptableInterface::PROPERTY_METHOD) {
        // Methods are ignored.
        return true;
      }
      Argument key_arg(key_signature_, Variant(name));
      Argument value_arg(value_signature_, value);
      Impl *sub = new Impl(marshaller_->iter_, DBUS_TYPE_DICT_ENTRY, NULL);
      bool ret = sub->AppendArgument(key_arg) && sub->AppendArgument(value_arg);
      delete sub;
      return ret;
    }
   private:
    Impl *marshaller_;
    std::string key_signature_;
    std::string value_signature_;
  };

  static bool ValistItemAdaptor(Argument *in_arg, MessageType first_arg_type,
                                va_list *va_args) {
    ASSERT(in_arg);
    MessageType type = first_arg_type;
    if (type == MESSAGE_TYPE_INVALID) return false;
    switch (type) {
      case MESSAGE_TYPE_BYTE:
        in_arg->signature = DBUS_TYPE_BYTE_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<int64_t>(va_arg(*va_args, int))));
        break;
      case MESSAGE_TYPE_BOOLEAN:
        in_arg->signature = DBUS_TYPE_BOOLEAN_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<bool>(va_arg(*va_args, int))));
        break;
      case MESSAGE_TYPE_INT16:
        in_arg->signature = DBUS_TYPE_INT16_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<int64_t>(va_arg(*va_args, int))));
        break;
      case MESSAGE_TYPE_UINT16:
        in_arg->signature = DBUS_TYPE_UINT16_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<int64_t>(va_arg(*va_args, int))));
        break;
      case MESSAGE_TYPE_INT32:
        in_arg->signature = DBUS_TYPE_INT32_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<int64_t>(va_arg(*va_args, int32_t))));
        break;
      case MESSAGE_TYPE_UINT32:
        in_arg->signature = DBUS_TYPE_UINT32_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<int64_t>(va_arg(*va_args, uint32_t))));
        break;
      case MESSAGE_TYPE_INT64:
        in_arg->signature = DBUS_TYPE_INT64_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<int64_t>(va_arg(*va_args, int64_t))));
        break;
      case MESSAGE_TYPE_UINT64:
        in_arg->signature = DBUS_TYPE_UINT64_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<int64_t>(va_arg(*va_args, uint64_t))));
        break;
      case MESSAGE_TYPE_DOUBLE:
        in_arg->signature = DBUS_TYPE_DOUBLE_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<double>(va_arg(*va_args, double))));
        break;
      case MESSAGE_TYPE_STRING:
        in_arg->signature = DBUS_TYPE_STRING_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<const char *>(va_arg(*va_args, const char*))));
        break;
      case MESSAGE_TYPE_OBJECT_PATH:
        in_arg->signature = DBUS_TYPE_OBJECT_PATH_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<const char *>(va_arg(*va_args, const char*))));
        break;
      case MESSAGE_TYPE_SIGNATURE:
        in_arg->signature = DBUS_TYPE_SIGNATURE_AS_STRING;
        in_arg->value = ResultVariant(
            Variant(static_cast<const char *>(va_arg(*va_args, const char*))));
        break;
      case MESSAGE_TYPE_ARRAY:
        {
          Argument arg;
          std::string signature(DBUS_TYPE_ARRAY_AS_STRING), item_sig;
          std::size_t size = static_cast<std::size_t>(va_arg(*va_args, int));
          bool ret = true;
          ScriptableHolder<ScriptableArray> array(new ScriptableArray());
          for (size_t i = 0; i < size && ret; ++i) {
            type = static_cast<MessageType>(va_arg(*va_args, int));
            ret = ValistItemAdaptor(&arg, type, va_args);
            if (item_sig.empty()) {
              item_sig = arg.signature;
            } else if (item_sig != arg.signature) {
              DLOG("Types of items in the array are not same.");
              ret = false;
            }
            array.Get()->Append(arg.value.v());
          }
          if (!ret) return false;
          signature.append(item_sig);
          in_arg->value = ResultVariant(Variant(array.Get()));
          in_arg->signature = signature;
          break;
        }
      case MESSAGE_TYPE_STRUCT:
        {
          std::size_t size = static_cast<std::size_t>(va_arg(*va_args, int));
          std::string signature(DBUS_STRUCT_BEGIN_CHAR_AS_STRING);
          Argument arg;
          bool ret = true;
          ScriptableHolder<ScriptableArray> array(new ScriptableArray());
          for (size_t i = 0; i < size && ret; ++i) {
            type = static_cast<MessageType>(va_arg(*va_args, int));
            ret = ValistItemAdaptor(&arg, type, va_args);
            signature.append(arg.signature);
            array.Get()->Append(arg.value.v());
          }
          if (!ret) return false;
          signature.append(DBUS_STRUCT_END_CHAR_AS_STRING);
          in_arg->value = ResultVariant(Variant(array.Get()));
          in_arg->signature = signature;
          break;
        }
      case MESSAGE_TYPE_VARIANT:
        type = static_cast<MessageType>(va_arg(*va_args, int));
        if (type == MESSAGE_TYPE_INVALID || type == MESSAGE_TYPE_VARIANT)
          return false;
        if (!ValistItemAdaptor(in_arg, type, va_args))
          return false;
        in_arg->signature = DBUS_TYPE_VARIANT_AS_STRING;
        break;
      case MESSAGE_TYPE_DICT:
        {
          std::size_t size = static_cast<std::size_t>(va_arg(*va_args, int));
          ScriptableDBusContainerHolder obj(new ScriptableDBusContainer());
          std::string signature(DBUS_TYPE_ARRAY_AS_STRING);
          signature.append(DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING);
          std::string key_sig, value_sig;
          Argument arg;
          bool ret = true;
          for (size_t i = 0; i < size && ret; ++i) {
            type = static_cast<MessageType>(va_arg(*va_args, int));
            ret = ValistItemAdaptor(&arg, type, va_args);
            if (!ret) break;
            if (key_sig.empty()) {
              key_sig = arg.signature;
            } else if (key_sig != arg.signature) {
              ret = false;
              break;
            }
            std::string str;
            if (!arg.value.v().ConvertToString(&str)) {
              DLOG("%s can not be converted to string to be a dict key",
                   arg.value.v().Print().c_str());
              ret = false;
              break;
            }
            type = static_cast<MessageType>(va_arg(*va_args, int));
            ret = ValistItemAdaptor(&arg, type, va_args);
            if (!ret) break;
            if (value_sig.empty()) {
              value_sig = arg.signature;
            } else if (value_sig != arg.signature) {
              ret = false;
              break;
            }
            obj.Get()->AddProperty(str, arg.value.v());
          }
          if (!ret) return false;
          signature.append(key_sig);
          signature.append(value_sig);
          signature.append(DBUS_DICT_ENTRY_END_CHAR_AS_STRING);
          in_arg->signature = signature;
          in_arg->value = ResultVariant(Variant(obj.Get()));
          break;
        }
      default:
        DLOG("Unsupported type: %d", type);
        return false;
    }
    return true;
  }

  void AppendByte(unsigned char value) {
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_BYTE, &value);
  }
  void AppendBoolean(bool value) {
    dbus_bool_t v = static_cast<dbus_bool_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_BOOLEAN, &v);
  }
  void AppendInt16(int16_t value) {
    dbus_int16_t v = static_cast<dbus_int16_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_INT16, &v);
  }
  void AppendUInt16(uint16_t value) {
    dbus_uint16_t v = static_cast<dbus_uint16_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_UINT16, &v);
  }
  void AppendInt32(int32_t value) {
    dbus_int32_t v = static_cast<dbus_int32_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_INT32, &v);
  }
  void AppendUInt32(uint32_t value) {
    dbus_uint32_t v = static_cast<dbus_uint32_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_UINT32, &v);
  }
  void AppendInt64(int64_t value) {
    dbus_int64_t v = static_cast<dbus_int64_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_INT64, &v);
  }
  void AppendUInt64(uint64_t value) {
    dbus_uint64_t v = static_cast<dbus_uint64_t>(value);
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_UINT64, &v);
  }
  void AppendDouble(double value) {
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_DOUBLE, &value);
  }
  void AppendString(const char *str) {
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_STRING, &str);
  }
  void AppendObjectPath(const char *path) {
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_OBJECT_PATH, &path);
  }
  void AppendSignature(const char *sig) {
    dbus_message_iter_append_basic(iter_, DBUS_TYPE_SIGNATURE, &sig);
  }
 private:
  DBusMessageIter *iter_;
  DBusMessageIter *parent_iter_;
};

DBusMarshaller::DBusMarshaller(DBusMessage *message)
  : impl_(new Impl(message)) {
}

DBusMarshaller::~DBusMarshaller() {
  delete impl_;
}

bool DBusMarshaller::AppendArguments(const Arguments &args) {
  for (Arguments::const_iterator it = args.begin(); it != args.end(); ++it)
    if (!AppendArgument(*it))
      return false;
  return true;
}

bool DBusMarshaller::AppendArgument(const Argument &arg) {
  return impl_->AppendArgument(arg);
}

bool DBusMarshaller::ValistAdaptor(Arguments *in_args,
                                   MessageType first_arg_type,
                                   va_list *va_args) {
  return Impl::ValistAdaptor(in_args, first_arg_type, va_args);
}

class DBusDemarshaller::Impl {
 public:
  Impl(DBusMessage *message) : iter_(NULL), parent_iter_(NULL) {
    iter_ = new DBusMessageIter;
    dbus_message_iter_init(message, iter_);
  }
  Impl(DBusMessageIter *parent) : iter_(NULL), parent_iter_(parent) {
    iter_ = new DBusMessageIter;
    dbus_message_iter_recurse(parent, iter_);
  }
  ~Impl() {
    delete iter_;
  }
  bool HasMoreItem() {
    return dbus_message_iter_get_arg_type(iter_) != DBUS_TYPE_INVALID;
  }
  bool MoveToNextItem() {
    return dbus_message_iter_next(iter_);
  }
  bool GetArgument(Argument *arg) {
    if (!HasMoreItem()) {
      DLOG("No argument left to be read in the message.");
      return false;
    }
    if (arg->signature.empty()) {
      char *sig = dbus_message_iter_get_signature(iter_);
      arg->signature = sig ? sig : "";
      dbus_free(sig);
      // no value remained in current message iterator
      if (arg->signature.empty()) return false;
    }
    int type = dbus_message_iter_get_arg_type(iter_);
    const char *index = arg->signature.c_str();
    if (!ValidateSignature(index, true))
      return false;
    if (type != GetTypeBySignature(index)) {
      DLOG("Demarshal failed. Type mismatch, message type: %c, expected: %c",
           type, GetTypeBySignature(index));
      return false;
    }
    switch (type) {
      case DBUS_TYPE_BYTE:
        {
          unsigned char value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = ResultVariant(Variant(static_cast<int64_t>(value)));
          break;
        }
      case DBUS_TYPE_BOOLEAN:
        {
          dbus_bool_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = ResultVariant(Variant(static_cast<bool>(value)));
          break;
        }
      case DBUS_TYPE_INT16:
        {
          dbus_int16_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = ResultVariant(Variant(static_cast<int64_t>(value)));
          break;
        }
      case DBUS_TYPE_UINT16:
        {
          dbus_uint16_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = ResultVariant(Variant(static_cast<int64_t>(value)));
          break;
        }
      case DBUS_TYPE_INT32:
        {
          dbus_int32_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = ResultVariant(Variant(static_cast<int64_t>(value)));
          break;
        }
      case DBUS_TYPE_UINT32:
        {
          dbus_uint32_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = ResultVariant(Variant(static_cast<int64_t>(value)));
          break;
        }
      case DBUS_TYPE_INT64:
        {
          dbus_int64_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = ResultVariant(Variant(static_cast<int64_t>(value)));
          break;
        }
      case DBUS_TYPE_UINT64:
        {
          dbus_uint64_t value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = ResultVariant(Variant(static_cast<int64_t>(value)));
          break;
        }
      case DBUS_TYPE_DOUBLE:
        {
          double value;
          dbus_message_iter_get_basic(iter_, &value);
          arg->value = ResultVariant(Variant(value));
          break;
        }
      case DBUS_TYPE_STRING:
      case DBUS_TYPE_OBJECT_PATH:
      case DBUS_TYPE_SIGNATURE:
        {
          const char *str;
          dbus_message_iter_get_basic(iter_, &str);
          arg->value = ResultVariant(Variant(str));
          break;
        }
      case DBUS_TYPE_ARRAY:
        {
          if (*(index + 1) == DBUS_DICT_ENTRY_BEGIN_CHAR) {  // it is a dict.
            std::string dict_sig = GetElementType(index + 1);
            StringVector sig_list;
            if (!GetSubElements(dict_sig, &sig_list))
              return false;
            if (sig_list.size() != 2 || !IsBasicType(sig_list[0]))
              return false;
            Impl dict(iter_);
            ScriptableDBusContainerHolder obj(new ScriptableDBusContainer());
            bool ret = true;
            if (dict.HasMoreItem()) {
              do {
                Impl sub(dict.iter_);
                Argument key(sig_list[0]);
                Argument value(sig_list[1]);
                ret = sub.GetArgument(&key) && sub.MoveToNextItem()
                    && sub.GetArgument(&value);
                if (!ret) break;
                std::string name;
                if (!key.value.v().ConvertToString(&name)) {
                  ret = false;
                  break;
                }
                obj.Get()->AddProperty(name, value.value.v());
              } while(dict.MoveToNextItem());
            }
            if (!ret) {
              DLOG("Failed to demarshal dictionary: %s", dict_sig.c_str());
              return false;
            }
            arg->value = ResultVariant(Variant(obj.Get()));
            index += dict_sig.size();
          } else {
            std::string sub_type = GetElementType(index + 1);
            Impl sub(iter_);
            bool ret = true;
            ScriptableHolder<ScriptableArray> array(new ScriptableArray());
            if (sub.HasMoreItem()) {
              do {
                Argument arg(sub_type);
                ret = sub.GetArgument(&arg);
                if (ret)
                  array.Get()->Append(arg.value.v());
              } while (ret && sub.MoveToNextItem());
            }
            if (!ret) {
              DLOG("Failed to demarshal array: %s", sub_type.c_str());
              return false;
            }
            arg->value = ResultVariant(Variant(array.Get()));
            index += sub_type.size();
          }
          break;
        }
      case DBUS_TYPE_STRUCT:
      case DBUS_TYPE_DICT_ENTRY:
        {
          std::string struct_sig = GetElementType(index);
          StringVector sig_list;
          if (!GetSubElements(struct_sig.c_str(), &sig_list))
            return false;
          if (type == DBUS_TYPE_DICT_ENTRY &&
              (sig_list.size() != 2 || !IsBasicType(sig_list[0])))
            return false;
          Impl sub(iter_);
          ScriptableHolder<ScriptableArray> array(new ScriptableArray());
          bool ret = false;
          for (StringVector::iterator it = sig_list.begin();
               it != sig_list.end(); ++it) {
            Argument arg(*it);
            ret = sub.GetArgument(&arg);
            if (!ret) break;
            array.Get()->Append(arg.value.v());
            sub.MoveToNextItem();
          }
          if (!ret) {
            DLOG("Failed to demarshal struct: %s", struct_sig.c_str());
            return false;
          }
          arg->value = ResultVariant(Variant(array.Get()));
          index += struct_sig.size() - 1;
          break;
        }
      case DBUS_TYPE_VARIANT:
        {
          DBusMessageIter subiter;
          dbus_message_iter_recurse(iter_, &subiter);
          char *sig = dbus_message_iter_get_signature(&subiter);
          if (!sig) {
            DLOG("Sub type of variant is invalid.");
            return false;
          }
          Impl sub(iter_);
          std::string str_sig(sig);
          Argument variant(str_sig);
          bool ret = sub.GetArgument(&variant);
          dbus_free(sig);
          if (!ret) {
            DLOG("Failed to demarshal variant: %s", str_sig.c_str());
            return false;
          }
          arg->value = variant.value;
          break;
        }
      default:
        DLOG("Unsupported type: %d", type);
        return false;
    }
    return true;
  }
  static bool ValistAdaptor(const Arguments &out_args,
                            MessageType first_arg_type, va_list *va_args) {
    MessageType type = first_arg_type;
    Arguments::const_iterator it = out_args.begin();
    while (type != MESSAGE_TYPE_INVALID) {
      if (it == out_args.end()) {
        DLOG("Too few arguments in reply.");
        return false;
      }
      int arg_type = GetTypeBySignature(it->signature.c_str());
      if (arg_type != MessageTypeToDBusType(type)) {
        DLOG("Type mismatch! the type in message is %d, "
             " but in this function it is %d", arg_type, type);
        ASSERT(false);
        return false;
      }
      if (!ValistItemAdaptor(*it, type, va_args))
        return false;
      ++it;
      type = static_cast<MessageType>(va_arg(*va_args, int));
    }
    return true;
  }
 private:
  static bool ValistItemAdaptor(const Argument &out_arg,
                                MessageType first_arg_type, va_list *va_args) {
    if (first_arg_type == MESSAGE_TYPE_INVALID)
      return false;
    void *return_storage = va_arg(*va_args, void*);
    if (return_storage) {
      switch (first_arg_type) {
        case MESSAGE_TYPE_BYTE:
          {
            int64_t i;
            if (!out_arg.value.v().ConvertToInt64(&i)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_INT64, out_arg.value.v().Print().c_str());
              return false;
            }
            unsigned char v = static_cast<unsigned char>(i);
            memcpy(return_storage, &v, sizeof(unsigned char));
            break;
          }
        case MESSAGE_TYPE_BOOLEAN:
          {
            bool v;
            if (!out_arg.value.v().ConvertToBool(&v)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_BOOL, out_arg.value.v().Print().c_str());
              return false;
            }
            memcpy(return_storage, &v, sizeof(bool));
            break;
          }
        case MESSAGE_TYPE_INT16:
          {
            int64_t i;
            if (!out_arg.value.v().ConvertToInt64(&i)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_INT64, out_arg.value.v().Print().c_str());
              return false;
            }
            int16_t v = static_cast<int16_t>(i);
            memcpy(return_storage, &v, sizeof(int16_t));
            break;
          }
        case MESSAGE_TYPE_UINT16:
          {
            int64_t i;
            if (!out_arg.value.v().ConvertToInt64(&i)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_INT64, out_arg.value.v().Print().c_str());
              return false;
            }
            uint16_t v = static_cast<uint16_t>(i);
            memcpy(return_storage, &v, sizeof(dbus_uint16_t));
            break;
          }
        case MESSAGE_TYPE_INT32:
          {
            int64_t i;
            if (!out_arg.value.v().ConvertToInt64(&i)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_INT64, out_arg.value.v().Print().c_str());
              return false;
            }
            int v = static_cast<int>(i);
            memcpy(return_storage, &v, sizeof(dbus_int32_t));
            break;
          }
        case MESSAGE_TYPE_UINT32:
          {
            int64_t i;
            if (!out_arg.value.v().ConvertToInt64(&i)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_INT64, out_arg.value.v().Print().c_str());
              return false;
            }
            uint32_t v = static_cast<uint32_t>(i);
            memcpy(return_storage, &v, sizeof(dbus_uint32_t));
            break;
          }
        case MESSAGE_TYPE_INT64:
          {
            int64_t i;
            if (!out_arg.value.v().ConvertToInt64(&i)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_INT64, out_arg.value.v().Print().c_str());
              return false;
            }
            memcpy(return_storage, &i, sizeof(dbus_int64_t));
            break;
          }
        case MESSAGE_TYPE_UINT64:
          {
            int64_t i;
            if (!out_arg.value.v().ConvertToInt64(&i)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_INT64, out_arg.value.v().Print().c_str());
              return false;
            }
            uint64_t v = static_cast<uint64_t>(i);
            memcpy(return_storage, &v, sizeof(dbus_uint64_t));
            break;
          }
        case MESSAGE_TYPE_DOUBLE:
          {
            double v;
            if (!out_arg.value.v().ConvertToDouble(&v)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_DOUBLE, out_arg.value.v().Print().c_str());
              return false;
            }
            memcpy(return_storage, &v, sizeof(double));
            break;
          }
        case MESSAGE_TYPE_STRING:
        case MESSAGE_TYPE_OBJECT_PATH:
        case MESSAGE_TYPE_SIGNATURE:
          {
            std::string str;
            if (!out_arg.value.v().ConvertToString(&str)) {
              DLOG("Type mismatch. Expected type: %d, actual value:%s",
                   Variant::TYPE_STRING, out_arg.value.v().Print().c_str());
              return false;
            }
            // The string must be duplicated, otherwise it'll be destroyed when
            // return.
            // So the caller must free it.
            char *s = new char[str.length() + 1];
            strncpy(s, str.c_str(), str.length());
            s[str.length()] = '\0';
            memcpy(return_storage, &s, sizeof(const char*));
            break;
          }
        // directly return the Variant for container types
        case MESSAGE_TYPE_ARRAY:
        case MESSAGE_TYPE_STRUCT:
        case MESSAGE_TYPE_VARIANT:
        case MESSAGE_TYPE_DICT:
          memcpy(return_storage, &out_arg.value, sizeof(Variant));
        default:
          DLOG("The DBus type %d is not supported yet!", first_arg_type);
          return false;
      }
    }
    return true;
  }
  static int GetTypeBySignature(const char *signature) {
    if (*signature == DBUS_STRUCT_BEGIN_CHAR)
      return DBUS_TYPE_STRUCT;
    if (*signature == DBUS_DICT_ENTRY_BEGIN_CHAR)
      return DBUS_TYPE_DICT_ENTRY;
    return static_cast<int>(*signature);
  }
  DBusMessageIter *iter_;
  DBusMessageIter *parent_iter_;
};

DBusDemarshaller::DBusDemarshaller(DBusMessage *message)
  : impl_(new Impl(message)) {
}

DBusDemarshaller::~DBusDemarshaller() {
  delete impl_;
}

bool DBusDemarshaller::GetArguments(Arguments *args) {
  Arguments tmp;
  bool ret = true;
  while (impl_->HasMoreItem()) {
    Argument arg;
    ret = GetArgument(&arg);
    if (ret)
      tmp.push_back(arg);
    else
      break;
  }
  args->swap(tmp);
  return ret;
}

bool DBusDemarshaller::GetArgument(Argument *arg) {
  bool ret = impl_->GetArgument(arg);
  impl_->MoveToNextItem();
  return ret;
}

bool DBusDemarshaller::ValistAdaptor(const Arguments &out_args,
                                     MessageType first_arg_type,
                                     va_list *va_args) {
  return Impl::ValistAdaptor(out_args, first_arg_type, va_args);
}

class DBusMainLoopClosure::Impl {
  // watch callback for calling dbus_connection_dispatch().
  class DBusDispatchCallback : public WatchCallbackInterface {
   public:
    DBusDispatchCallback(Impl *impl) : impl_(impl) { }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
#ifdef DBUS_VERBOSE_LOG
      DLOG("Dispatch DBus connection.");
#endif
      // Only dispatch once each time.
      int status = dbus_connection_dispatch(impl_->connection_);

      if (status == DBUS_DISPATCH_NEED_MEMORY)
        LOG("Out of memory when dispatching DBus connection.");

      // Keep the watch if there are still some data.
      return status == DBUS_DISPATCH_DATA_REMAINS;
    }
    virtual void OnRemove(MainLoopInterface* main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      impl_->dispatch_timeout_ = -1;
      delete this;
    }
   private:
    Impl *impl_;
  };

  // watch callback for handling dbus io watch.
  class DBusWatchCallback : public WatchCallbackInterface {
   public:
    DBusWatchCallback(Impl *impl, DBusWatch *watch)
      : impl_(impl), watch_(watch),
        read_id_(-1), write_id_(-1), refcount_(1) {
#ifdef DBUS_VERBOSE_LOG
      DLOG("Create DBusWatchCallback %p, watch %p", this, watch_);
#endif
      SetEnabled(dbus_watch_get_enabled(watch));
    }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      ASSERT(impl_);
      ASSERT(main_loop == impl_->main_loop_);
      ASSERT(watch_id == read_id_ || watch_id == write_id_);
      ASSERT(refcount_ > 0);
#ifdef DBUS_VERBOSE_LOG
      DLOG("Call DBusWatchCallback %p, watch %p, watch id: %d (%s)",
           this, watch_, watch_id, (watch_id == read_id_ ? "read" : "write"));
#endif

      if (!dbus_watch_get_enabled(watch_)) {
#ifdef DBUS_VERBOSE_LOG
        DLOG("Don't call disabled DBusWatchCallback %p, watch %p",
             this, watch_);
#endif
        return true;
      }

      int flags =
          (watch_id == read_id_ ? DBUS_WATCH_READABLE : DBUS_WATCH_WRITABLE);

      dbus_watch_handle(watch_, flags);
      impl_->CheckDispatchStatus();
#ifdef DBUS_VERBOSE_LOG
      DLOG("End call DBusWatchCallback %p, watch %p", this, watch_);
#endif
      // Keep this watch until RemoveWatch() or SetEnabled() is called
      // explicitly.
      return true;
    }
    virtual void OnRemove(MainLoopInterface* main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      ASSERT(main_loop == impl_->main_loop_);
      ASSERT(watch_id == read_id_ || watch_id == write_id_);
      if (read_id_ == watch_id)
        read_id_ = -1;
      else if (write_id_ == watch_id)
        write_id_ = -1;
      Unref();
    }

    void SetEnabled(bool enabled) {
#ifdef HAVE_DBUS_WATCH_GET_UNIX_FD
      int fd = dbus_watch_get_unix_fd(watch_);
#else
      int fd = dbus_watch_get_fd(watch_);
#endif
      int flags = dbus_watch_get_flags(watch_);
      if (enabled) {
        if ((flags & DBUS_WATCH_READABLE) && read_id_ <= 0) {
          read_id_ = impl_->main_loop_->AddIOReadWatch(fd, this);
          Ref();
        }
        if ((flags & DBUS_WATCH_WRITABLE) && write_id_ <= 0) {
          write_id_ = impl_->main_loop_->AddIOWriteWatch(fd, this);
          Ref();
        }
#ifdef DBUS_VERBOSE_LOG
        DLOG("Enable DBusWatchCallback %p, watch %p, "
             "fd:%d, flag:%d, rid:%d, wid:%d",
             this, watch_, fd, flags, read_id_, write_id_);
#endif
      } else {
#ifdef DBUS_VERBOSE_LOG
        DLOG("Disable DBusWatchCallback %p, watch %p, "
             "fd:%d, flag:%d, rid:%d, wid:%d",
             this, watch_, fd, flags, read_id_, write_id_);
#endif
        if (read_id_ > 0)
          impl_->main_loop_->RemoveWatch(read_id_);
        if (write_id_ > 0)
          impl_->main_loop_->RemoveWatch(write_id_);
      }
    }
    void Remove() {
      SetEnabled(false);
      Unref();
    }

   private:
    void Ref() {
      ASSERT(refcount_ > 0);
      ++refcount_;
    }
    void Unref() {
      ASSERT(refcount_ > 0);
      --refcount_;
      if (refcount_ <= 0)
        delete this;
    }
    virtual ~DBusWatchCallback() {
#ifdef DBUS_VERBOSE_LOG
      DLOG("Destroy DBusWatchCallback %p, watch %p", this, watch_);
#endif
    }

    Impl *impl_;
    DBusWatch *watch_;
    int read_id_;
    int write_id_;
    int refcount_;
  };

  // watch callback for handling dbus timeout.
  class DBusTimeoutCallback : public WatchCallbackInterface {
   public:
    DBusTimeoutCallback(Impl *impl, DBusTimeout* timeout)
      : impl_(impl), timeout_(timeout), watch_id_(-1), refcount_(1) {
#ifdef DBUS_VERBOSE_LOG
      DLOG("Create DBusTimeoutCallback %p, timeout %p", this, timeout_);
#endif
      SetEnabled(dbus_timeout_get_enabled(timeout));
    }
    virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      ASSERT(impl_);
      ASSERT(main_loop == impl_->main_loop_);
      ASSERT(watch_id == watch_id_);
      ASSERT(refcount_ > 0);
#ifdef DBUS_VERBOSE_LOG
      DLOG("Call DBusTimeoutCallback %p, timeout %p, watch id: %d",
           this, timeout_, watch_id_);
#endif

      if (!dbus_timeout_get_enabled(timeout_)) {
#ifdef DBUS_VERBOSE_LOG
        DLOG("Don't call disabled DBusTimeoutCallback %p, timeout_ %p",
             this, timeout_);
#endif
        return true;
      }

      dbus_timeout_handle(timeout_);
      impl_->CheckDispatchStatus();
#ifdef DBUS_VERBOSE_LOG
      DLOG("End call DBusTimeoutCallback %p, timeout %p", this, timeout_);
#endif
      // Keep this watch until RemoveWatch() or SetEnabled() is called
      // explicitly.
      return true;
    }
    virtual void OnRemove(MainLoopInterface* main_loop, int watch_id) {
      GGL_UNUSED(main_loop);
      GGL_UNUSED(watch_id);
      ASSERT(main_loop == impl_->main_loop_);
      ASSERT(watch_id == watch_id_);
      watch_id_ = -1;
      Unref();
    }
    void SetEnabled(bool enabled) {
      int interval = dbus_timeout_get_interval(timeout_);
      if (enabled && watch_id_ <= 0) {
        watch_id_ = impl_->main_loop_->AddTimeoutWatch(interval, this);
        Ref();
#ifdef DBUS_VERBOSE_LOG
        DLOG("Enable DBusTimeoutCallback %p, timeout %p, id:%d, interval:%d",
             this, timeout_, watch_id_, interval);
#endif
      } else if (!enabled && watch_id_ > 0) {
#ifdef DBUS_VERBOSE_LOG
        DLOG("Disable DBusTimeoutCallback %p, timeout %p, id:%d, interval:%d",
             this, timeout_, watch_id_, interval);
#endif
        impl_->main_loop_->RemoveWatch(watch_id_);
      }
    }
    void Remove() {
      SetEnabled(false);
      Unref();
    }

   private:
    void Ref() {
      ASSERT(refcount_ > 0);
      ++refcount_;
    }
    void Unref() {
      ASSERT(refcount_ > 0);
      --refcount_;
      if (refcount_ <= 0)
        delete this;
    }
    virtual ~DBusTimeoutCallback() {
#ifdef DBUS_VERBOSE_LOG
      DLOG("Destroy DBusTimeoutCallback %p, timeout %p", this, timeout_);
#endif
    }

    Impl *impl_;
    DBusTimeout *timeout_;
    int watch_id_;
    int refcount_;
  };

 public:
  Impl(DBusConnection* connection, MainLoopInterface* main_loop)
    : connection_(connection),
      main_loop_(main_loop),
      dispatch_timeout_(-1) {
    ASSERT(connection_);
    ASSERT(main_loop_);
    dbus_connection_ref(connection_);
    dbus_connection_set_dispatch_status_function(
        connection_, DispatchStatus, this, NULL);
    dbus_connection_set_wakeup_main_function(
        connection_, WakeUpMain, this, NULL);
    dbus_connection_set_watch_functions(
        connection_, AddWatch, RemoveWatch, WatchToggled, this, NULL);
    dbus_connection_set_timeout_functions(
        connection_, AddTimeout, RemoveTimeout, TimeoutToggled, this, NULL);
  }

  ~Impl() {
    dbus_connection_set_dispatch_status_function(
        connection_, NULL, NULL, NULL);
    dbus_connection_set_wakeup_main_function(
        connection_, NULL, NULL, NULL);
    dbus_connection_set_watch_functions(
        connection_, NULL, NULL, NULL, NULL, NULL);
    dbus_connection_set_timeout_functions(
        connection_, NULL, NULL, NULL, NULL, NULL);

    if (dispatch_timeout_ > 0)
      main_loop_->RemoveWatch(dispatch_timeout_);

    dbus_connection_unref(connection_);
  }

  void CheckDispatchStatus() {
    if (dbus_connection_get_dispatch_status(connection_) ==
        DBUS_DISPATCH_DATA_REMAINS && dispatch_timeout_ <= 0) {
      dispatch_timeout_ =
          main_loop_->AddTimeoutWatch(0, new DBusDispatchCallback(this));
    }
  }

 private:
  static void DispatchStatus(DBusConnection *connection,
                             DBusDispatchStatus new_status,
                             void *data) {
    GGL_UNUSED(connection);
    GGL_UNUSED(new_status);
#ifdef DBUS_VERBOSE_LOG
    DLOG("DispatchStatus");
#endif
    Impl *impl = reinterpret_cast<Impl*>(data);
    ASSERT(impl);
    impl->CheckDispatchStatus();
  }

  static void WakeUpMain(void *data) {
#ifdef DBUS_VERBOSE_LOG
    DLOG("DBus wake up main loop.");
#endif
    Impl *impl = reinterpret_cast<Impl*>(data);
    ASSERT(impl);
    impl->main_loop_->WakeUp();
  }

  static dbus_bool_t AddWatch(DBusWatch *watch, void* data) {
#ifdef DBUS_VERBOSE_LOG
    DLOG("DBus add watch.");
#endif
    Impl *impl = reinterpret_cast<Impl*>(data);
    ASSERT(impl);
    DBusWatchCallback *callback =
        new DBusWatchCallback(impl, watch);
    dbus_watch_set_data(watch, callback, NULL);
    return TRUE;
  }

  static void RemoveWatch(DBusWatch* watch, void* data) {
    GGL_UNUSED(data);
#ifdef DBUS_VERBOSE_LOG
    DLOG("DBus remove watch.");
#endif
    DBusWatchCallback *callback =
        reinterpret_cast<DBusWatchCallback *>(dbus_watch_get_data(watch));
    if (callback)
      callback->Remove();
  }

  static void WatchToggled(DBusWatch* watch, void* data) {
    GGL_UNUSED(data);
#ifdef DBUS_VERBOSE_LOG
    DLOG("DBus toggle watch.");
#endif
    DBusWatchCallback *callback =
        reinterpret_cast<DBusWatchCallback *>(dbus_watch_get_data(watch));
    if (callback)
      callback->SetEnabled(dbus_watch_get_enabled(watch));
  }

  static dbus_bool_t AddTimeout(DBusTimeout *timeout, void* data) {
#ifdef DBUS_VERBOSE_LOG
    DLOG("DBus add timeout.");
#endif
    Impl *impl = reinterpret_cast<Impl*>(data);
    ASSERT(impl);
    DBusTimeoutCallback *callback =
        new DBusTimeoutCallback(impl, timeout);
    dbus_timeout_set_data(timeout, callback, NULL);
    return TRUE;
  }

  static void RemoveTimeout(DBusTimeout *timeout, void* data) {
    GGL_UNUSED(data);
#ifdef DBUS_VERBOSE_LOG
    DLOG("DBus remove timeout.");
#endif
    DBusTimeoutCallback *callback =
        reinterpret_cast<DBusTimeoutCallback *>(dbus_timeout_get_data(timeout));
    if (callback)
      callback->Remove();
  }

  static void TimeoutToggled(DBusTimeout *timeout, void* data) {
    GGL_UNUSED(data);
#ifdef DBUS_VERBOSE_LOG
    DLOG("DBus toggle timeout.");
#endif
    DBusTimeoutCallback *callback =
        reinterpret_cast<DBusTimeoutCallback *>(dbus_timeout_get_data(timeout));
    if (callback)
      callback->SetEnabled(dbus_timeout_get_enabled(timeout));
  }

 private:
  DBusConnection* connection_;
  MainLoopInterface* main_loop_;
  int dispatch_timeout_;
};

DBusMainLoopClosure::DBusMainLoopClosure(DBusConnection* connection,
                                         MainLoopInterface* main_loop)
  : impl_(new Impl(connection, main_loop)) {
}

DBusMainLoopClosure::~DBusMainLoopClosure() {
  delete impl_;
}

}  // namespace dbus
}  // namespace ggadget
