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
#include "variant.h"

#include <cmath>
#include <cstring>
#include <cstdlib>
#include "common.h"  // It defines some int types.
#include "format_macros.h"  // It defines PRId64/PRIu64/PRIx64.
#include "logger.h"
#include "scriptable_interface.h"
#include "signals.h"
#include "slot.h"
#include "string_utils.h"

namespace ggadget {

#if GGADGET_STD_STRING_SHARED
// The values doesn't matter, because we only use their c_str() result to
// check if a string is constructed from these "NULL" strings.
// We choose the value "(nil)" to ease printing (see Print()).
// Don't use blank value, because all strings with blank values are shared
// in the standard impl of C++ library.
const std::string VariantString::kNullString("(nil)");
static const UTF16Char kNullUTF16StringValue[] =
    { '(', 'n', 'i', 'l', ')', 0 };
const UTF16String VariantUTF16String::kNullString(kNullUTF16StringValue);
#endif

Variant::Variant() : type_(TYPE_VOID) {
  memset(&v_, 0, sizeof(v_));
}

Variant::Variant(const Variant &source) : type_(TYPE_VOID) {
  memset(&v_, 0, sizeof(v_));
  operator=(source);
}

Variant::Variant(const ResultVariant &source) : type_(TYPE_VOID) {
  memset(&v_, 0, sizeof(v_));
  operator=(source.v());
}

Variant::Variant(Type type) : type_(type) {
  if (type_ == TYPE_STRING || type_ == TYPE_JSON) {
    new (&v_.string_place_) VariantString(NULL);
  } else if (type_ == TYPE_UTF16STRING) {
    new (&v_.utf16_string_place_) VariantUTF16String(NULL);
  } else {
    memset(&v_, 0, sizeof(v_));
  }
}

Variant::~Variant() {
  if (type_ == TYPE_STRING || type_ == TYPE_JSON) {
    // Don't delete because the pointer is allocated in place.
    reinterpret_cast<VariantString *>(&v_.string_place_)->~VariantString();
  } else if (type_ == TYPE_UTF16STRING) {
    // Don't delete because the pointer is allocated in place.
    reinterpret_cast<VariantUTF16String *>(&v_.utf16_string_place_)
        ->~VariantUTF16String();
  }
}

Variant &Variant::operator=(const Variant &source) {
  if (&source == this)
    return *this;

  if (type_ == TYPE_STRING || type_ == TYPE_JSON) {
    // Don't delete because the pointer is allocated in place.
    reinterpret_cast<VariantString *>(&v_.string_place_)->~VariantString();
  } else if (type_ == TYPE_UTF16STRING) {
    // Don't delete because the pointer is allocated in place.
    reinterpret_cast<VariantUTF16String *>(&v_.utf16_string_place_)
        ->~VariantUTF16String();
  }

  type_ = source.type_;
  switch (type_) {
    case TYPE_VOID:
      break;
    case TYPE_BOOL:
      v_.bool_value_ = source.v_.bool_value_;
      break;
    case TYPE_INT64:
      v_.int64_value_ = source.v_.int64_value_;
      break;
    case TYPE_DOUBLE:
      v_.double_value_ = source.v_.double_value_;
      break;
    case TYPE_STRING:
    case TYPE_JSON:
      new (&v_.string_place_) VariantString(
          *reinterpret_cast<const VariantString *>(&source.v_.string_place_));
      break;
    case TYPE_UTF16STRING:
      new (&v_.utf16_string_place_) VariantUTF16String(
           *reinterpret_cast<const VariantUTF16String *>(
               &source.v_.utf16_string_place_));
      break;
    case TYPE_SCRIPTABLE:
      v_.scriptable_value_ = source.v_.scriptable_value_;
      break;
    case TYPE_SLOT:
      v_.slot_value_ = source.v_.slot_value_;
      break;
    case TYPE_DATE:
      v_.int64_value_ = source.v_.int64_value_;
      break;
    case TYPE_ANY:
      v_.any_value_ = source.v_.any_value_;
      break;
    case TYPE_CONST_ANY:
      v_.const_any_value_ = source.v_.const_any_value_;
      break;
    case TYPE_VARIANT:
      // A Variant of type TYPE_VARIANT is only used as a prototype.
      break;
    default:
      break;
  }
  return *this;
}

bool Variant::operator==(const Variant &another) const {
  if (type_ != another.type_)
    return false;

  switch (type_) {
    case TYPE_VOID:
      return true;
    case TYPE_BOOL:
      return v_.bool_value_ == another.v_.bool_value_;
    case TYPE_INT64:
      return v_.int64_value_ == another.v_.int64_value_;
    case TYPE_DOUBLE:
      return v_.double_value_ == another.v_.double_value_;
    case TYPE_STRING: {
      const char *s1 = VariantValue<const char *>()(*this);
      const char *s2 = VariantValue<const char *>()(another);
      return (s1 == s2 || (s1 && s2 && strcmp(s1, s2) == 0));
    }
    case TYPE_JSON:
      return VariantValue<JSONString>()(*this) ==
             VariantValue<JSONString>()(another);
    case TYPE_UTF16STRING: {
      const UTF16Char *s1 = VariantValue<const UTF16Char *>()(*this);
      const UTF16Char *s2 = VariantValue<const UTF16Char *>()(another);
      return (s1 == s2 || (s1 && s2 && VariantValue<UTF16String>()(*this) ==
                                       VariantValue<UTF16String>()(another)));
    }
    case TYPE_SCRIPTABLE:
      return v_.scriptable_value_ == another.v_.scriptable_value_;
    case TYPE_SLOT: {
      Slot *slot1 = v_.slot_value_;
      Slot *slot2 = another.v_.slot_value_;
      return slot1 == slot2 || (slot1 && slot2 && *slot1 == *slot2);
    }
    case TYPE_DATE:
      return v_.int64_value_ == another.v_.int64_value_;
    case TYPE_ANY:
      return v_.any_value_ == another.v_.any_value_;
    case TYPE_CONST_ANY:
      return v_.const_any_value_ == another.v_.const_any_value_;
    case TYPE_VARIANT:
      // A Variant of type TYPE_VARIANT is only used as a prototype,
      // so they are all equal.
      return true;
    default:
      return false;
  }
}

static std::string FitString(const std::string &input) {
  return input.size() > 70 ? input.substr(0, 70) + "..." : input;
}

// Used in unittests.
std::string Variant::Print() const {
  switch (type_) {
    case TYPE_VOID:
      return "VOID";
    case TYPE_BOOL:
      return std::string("BOOL:") + (v_.bool_value_ ? "true" : "false");
    case TYPE_INT64:
      return "INT64:" + StringPrintf("%" PRId64, v_.int64_value_);
    case TYPE_DOUBLE:
      return "DOUBLE:" + StringPrintf("%g", v_.double_value_);
    case TYPE_STRING:
      return std::string("STRING:") +
         // Print "(nil)" for NULL string pointer.
         FitString(reinterpret_cast<const VariantString *>(
             &v_.string_place_)->string());
    case TYPE_JSON:
      return std::string("JSON:") +
             FitString(VariantValue<JSONString>()(*this).value);
    case TYPE_UTF16STRING: {
      std::string utf8_string;
      ConvertStringUTF16ToUTF8(
          // Print "(nil)" for NULL string pointer.
          reinterpret_cast<const VariantUTF16String *>(
              &v_.utf16_string_place_)->string(),
          &utf8_string);
      return "UTF16STRING:" + FitString(utf8_string);
    }
    case TYPE_SCRIPTABLE:
      return StringPrintf("SCRIPTABLE:%p(CLASS_ID=%" PRIx64 ")",
                          v_.scriptable_value_,
                          v_.scriptable_value_ ?
                              v_.scriptable_value_->GetClassId() : 0);
    case TYPE_SLOT:
      return StringPrintf("SLOT:%p", v_.slot_value_);
    case TYPE_DATE:
      return StringPrintf("DATE:%" PRIu64, v_.int64_value_);
    case TYPE_ANY:
      return StringPrintf("ANY:%p", v_.any_value_);
    case TYPE_CONST_ANY:
      return StringPrintf("ANY:%p", v_.const_any_value_);
    case TYPE_VARIANT:
      return "VARIANT";
    default:
      return "INVALID";
  }
}

bool Variant::ConvertToString(std::string *result) const {
  ASSERT(result);
  switch (type_) {
    case TYPE_VOID:
      *result = "";
      return true;
    case TYPE_BOOL:
      *result = v_.bool_value_ ? "true" : "false";
      return true;
    case TYPE_INT64:
      *result = StringPrintf("%" PRId64, v_.int64_value_);
      return true;
    case TYPE_DOUBLE:
      *result = StringPrintf("%g", v_.double_value_);
      return true;
    case TYPE_STRING:
      *result = VariantValue<std::string>()(*this);
      return true;
    case TYPE_JSON:
      return false;
    case TYPE_UTF16STRING:
      ConvertStringUTF16ToUTF8(VariantValue<UTF16String>()(*this), result);
      return true;
    case TYPE_SCRIPTABLE:
    case TYPE_SLOT:
    case TYPE_DATE:
    case TYPE_ANY:
    case TYPE_CONST_ANY:
    case TYPE_VARIANT:
    default:
      return false;
  }
}

static bool ParseStringToBool(const char *str_value, bool *result) {
  if (!*str_value || GadgetStrCmp(str_value, "false") == 0) {
    *result = false;
    return true;
  }
  if (GadgetStrCmp(str_value, "true") == 0) {
    *result = true;
    return true;
  }
  return false;
}

bool Variant::ConvertToBool(bool *result) const {
  ASSERT(result);
  switch (type_) {
    case TYPE_VOID:
      *result = false;
      return true;
    case TYPE_BOOL:
      *result = v_.bool_value_;
      return true;
    case TYPE_INT64:
      *result = v_.int64_value_ != 0;
      return true;
    case TYPE_DOUBLE:
      *result = v_.double_value_ != 0;
      return true;
    case TYPE_STRING:
      return ParseStringToBool(VariantValue<std::string>()(*this).c_str(),
                               result);
    case TYPE_JSON:
      return false;
    case TYPE_UTF16STRING: {
      std::string s;
      ConvertStringUTF16ToUTF8(VariantValue<UTF16String>()(*this), &s);
      return ParseStringToBool(s.c_str(), result);
    }
    case TYPE_SCRIPTABLE:
      *result = v_.scriptable_value_ != NULL;
      return true;
    case TYPE_SLOT:
      *result = v_.slot_value_ != NULL;
      return true;
    case TYPE_DATE:
      *result = true;
      return true;
    case TYPE_ANY:
      *result = v_.any_value_ != NULL;
      return true;
    case TYPE_CONST_ANY:
      *result = v_.const_any_value_ != NULL;
      return true;
    case TYPE_VARIANT:
    default:
      return false;
  }
}

bool Variant::ConvertToInt(int *result) const {
  int64_t i;
  if (ConvertToInt64(&i)) {
    *result = static_cast<int>(i);
    return true;
  }
  return false;
}

static bool ParseStringToDouble(const char *str_value, double *result) {
  char *end_ptr;
  // We don't allow hexidecimal floating-point numbers or INFINITY or NAN.
  if (strchr(str_value, 'x') || strchr(str_value, 'X') ||
      strchr(str_value, 'n') || strchr(str_value, 'N'))
    return false;

  double d = strtod(str_value, &end_ptr);
  // Allow space after double number.
  while(*end_ptr == ' ') ++end_ptr;
  if (*end_ptr == '\0') {
    *result = d;
    return true;
  }
  return false;
}

static bool ParseStringToInt64(const char *str_value, int64_t *result) {
  char *end_ptr;
  // TODO: Check if strtoll is available
  int64_t i = static_cast<int64_t>(strtoll(str_value, &end_ptr, 10));
  // Allow space after int64 number.
  while(*end_ptr == ' ') ++end_ptr;
  if (*end_ptr == '\0') {
    *result = i;
    return true;
  }
  // Then try to parse double.
  double d;
  if (ParseStringToDouble(str_value, &d)) {
    *result = static_cast<int64_t>(round(d));
    return true;
  }
  return false;
}

bool Variant::ConvertToInt64(int64_t *result) const {
  ASSERT(result);
  switch (type_) {
    case TYPE_VOID:
      return false;
    case TYPE_BOOL:
      *result = v_.bool_value_ ? 1 : 0;
      return true;
    case TYPE_INT64:
      *result = v_.int64_value_;
      return true;
    case TYPE_DOUBLE:
      *result = static_cast<int64_t>(v_.double_value_);
      return true;
    case TYPE_STRING:
      return ParseStringToInt64(VariantValue<std::string>()(*this).c_str(),
                                result);
    case TYPE_JSON:
      return false;
    case TYPE_UTF16STRING: {
      std::string s;
      ConvertStringUTF16ToUTF8(VariantValue<UTF16String>()(*this), &s);
      return ParseStringToInt64(s.c_str(), result);
    }
    case TYPE_SCRIPTABLE:
    case TYPE_SLOT:
    case TYPE_DATE:
    case TYPE_ANY:
    case TYPE_CONST_ANY:
    case TYPE_VARIANT:
    default:
      return false;
  }
}

bool Variant::ConvertToDouble(double *result) const {
  ASSERT(result);
  switch (type_) {
    case TYPE_VOID:
      return false;
    case TYPE_BOOL:
      *result = v_.bool_value_ ? 1 : 0;
      return true;
    case TYPE_INT64:
      *result = static_cast<double>(v_.int64_value_);
      return true;
    case TYPE_DOUBLE:
      *result = v_.double_value_;
      return true;
    case TYPE_STRING:
      return ParseStringToDouble(VariantValue<std::string>()(*this).c_str(),
                                 result);
    case TYPE_JSON:
      return false;
    case TYPE_UTF16STRING: {
      std::string s;
      ConvertStringUTF16ToUTF8(VariantValue<UTF16String>()(*this), &s);
      return ParseStringToDouble(s.c_str(), result);
    }
    case TYPE_SCRIPTABLE:
    case TYPE_SLOT:
    case TYPE_DATE:
    case TYPE_ANY:
    case TYPE_CONST_ANY:
    case TYPE_VARIANT:
    default:
      return false;
  }
}

bool Variant::CheckScriptableType(uint64_t class_id) const {
  ASSERT(type_ == TYPE_SCRIPTABLE);
  if (v_.scriptable_value_ &&
      !v_.scriptable_value_->IsInstanceOf(class_id)) {
    LOG("The parameter is not an instance pointer of 0x%" PRIx64, class_id);
    return false;
  }
  return true;
}

ResultVariant::ResultVariant(const Variant &v)
    : v_(v) {
  if (v_.type() == Variant::TYPE_SCRIPTABLE) {
    ScriptableInterface *scriptable =
        VariantValue<ScriptableInterface *>()(v_);
    if (scriptable)
      scriptable->Ref();
  }
}

ResultVariant::ResultVariant(const ResultVariant &v)
    : v_(v.v_) {
  if (v_.type() == Variant::TYPE_SCRIPTABLE) {
    ScriptableInterface *scriptable =
        VariantValue<ScriptableInterface *>()(v_);
    if (scriptable)
      scriptable->Ref();
  }
}

ResultVariant::~ResultVariant() {
  if (v_.type() == Variant::TYPE_SCRIPTABLE) {
    ScriptableInterface *scriptable =
        VariantValue<ScriptableInterface *>()(v_);
    if (scriptable)
      scriptable->Unref();
  }
}

ResultVariant &ResultVariant::operator=(const ResultVariant &v) {
  if (&v == this)
    return *this;

  if (v_.type() == Variant::TYPE_SCRIPTABLE) {
    ScriptableInterface *scriptable =
        VariantValue<ScriptableInterface *>()(v_);
    if (scriptable)
      scriptable->Unref();
  }
  v_ = v.v_;
  if (v_.type() == Variant::TYPE_SCRIPTABLE) {
    ScriptableInterface *scriptable =
        VariantValue<ScriptableInterface *>()(v_);
    if (scriptable)
      scriptable->Ref();
  }
  return *this;
}

} // namespace ggadget
