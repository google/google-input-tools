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

#include "permissions.h"
#include "common.h"
#include "logger.h"
#include "messages.h"
#include "slot.h"
#include "string_utils.h"
#include "small_object.h"

namespace ggadget {

struct PermissionInfo {
  // Name of this permission
  const char *name;
  // Bitmask for this permission
  uint32_t mask;
  // For the permissions which are granted by default, this bitmask can be
  // set to the permissions which exclude this permission by default,
  // so that if any of the permission specified by this bitmask is
  // granted explicitly, then this permission will be denied by default, unless
  // it's granted explicitly.
  // This bitmask only affects the default grant state, the permission can
  // still be granted or denied explicitly.
  uint32_t excluded_by;
  // Bitmask for the permissions which imply this permission
  uint32_t included_by;
  // If this permission is granted by default.
  bool default_granted;
};

static const PermissionInfo kPermissionsInfo[] = {
  { "fileread",     0x1,  0x8, 0x20, true },
  { "filewrite",    0x2,  0x0, 0x20, false },
  { "devicestatus", 0x4,  0x8, 0x20, true },
  { "network",      0x8,  0x0, 0x20, false },
  { "personaldata", 0x10, 0x0, 0x20, false },
  { "allaccess",    0x20, 0x0, 0x0,  false }
};

class Permissions::Impl : public SmallValueObject<> {
 public:
  Impl() : required_(0), granted_(0), denied_(0) {
  }

  uint32_t required_; // bitmask for explicitly required permissions.
  uint32_t granted_;  // bitmask for explicitly granted_ permissions.
  uint32_t denied_;   // bitmask for explicitly denied_ permissions.
};

Permissions::Permissions()
  : impl_(new Impl()) {
}

Permissions::Permissions(const Permissions &source)
  : impl_(new Impl()) {
  *impl_ = *source.impl_;
}

Permissions::~Permissions() {
  delete impl_;
  impl_ = NULL;
}

Permissions& Permissions::operator=(const Permissions &source) {
  if (this != &source)
    *impl_ = *source.impl_;
  return *this;
}

bool Permissions::operator==(const Permissions &another) const {
  return impl_->required_ == another.impl_->required_ &&
         impl_->granted_ == another.impl_->granted_ &&
         impl_->denied_ == another.impl_->denied_;
}

void Permissions::SetGranted(int permission, bool granted) {
  ASSERT (permission >= FILE_READ && permission <= ALL_ACCESS);
  if (permission >= FILE_READ && permission <= ALL_ACCESS) {
    if (granted) {
      impl_->granted_ |= kPermissionsInfo[permission].mask;
      impl_->denied_ &= ~kPermissionsInfo[permission].mask;
    } else {
      impl_->granted_ &= ~kPermissionsInfo[permission].mask;
      impl_->denied_ |= kPermissionsInfo[permission].mask;
    }
  }
}

void Permissions::SetGrantedByName(const char *permission, bool granted) {
  SetGranted(GetByName(permission), granted);
}

void Permissions::SetGrantedByPermissions(const Permissions &another,
                                          bool granted) {
  if (granted) {
    impl_->granted_ |= another.impl_->granted_;
    impl_->denied_ &= ~another.impl_->granted_;
  } else {
    impl_->granted_ &= ~another.impl_->denied_;
    impl_->denied_ |= another.impl_->denied_;
  }
}

bool Permissions::IsGranted(int permission) const {
  ASSERT (permission >= FILE_READ && permission <= ALL_ACCESS);
  if (permission >= FILE_READ && permission <= ALL_ACCESS) {
    const PermissionInfo *info = &kPermissionsInfo[permission];
    // The permission is treated as granted if one of following criterias is
    // true:
    // 1. It's granted explicitly.
    // 2. Any permission, which implies this permission, is granted
    //    explicitly.
    // 3. All permissions, which exclude this permission, are not granted,
    //    and this permission is marked as granted by default, and this
    //    permission is not denied explicitly.
    return (impl_->granted_ & info->mask) ||
           (impl_->granted_ & info->included_by) ||
           (info->default_granted && !(impl_->granted_ & info->excluded_by) &&
            !(impl_->denied_ & info->mask));
  }
  return false;
}

void Permissions::SetRequired(int permission, bool required) {
  ASSERT (permission >= FILE_READ && permission <= ALL_ACCESS);
  if (permission >= FILE_READ && permission <= ALL_ACCESS) {
    if (required)
      impl_->required_ |= kPermissionsInfo[permission].mask;
    else
      impl_->required_ &= ~kPermissionsInfo[permission].mask;
  }
}

void Permissions::SetRequiredByName(const char *permission, bool required) {
  SetRequired(GetByName(permission), required);
}

void Permissions::SetRequiredByPermissions(const Permissions &another,
                                           bool required) {
  if (required)
    impl_->required_ |= another.impl_->required_;
  else
    impl_->required_ &= another.impl_->required_;
}

bool Permissions::IsRequired(int permission) const {
  ASSERT (permission >= FILE_READ && permission <= ALL_ACCESS);
  if (permission >= FILE_READ && permission <= ALL_ACCESS) {
    const PermissionInfo *info = &kPermissionsInfo[permission];
    // The permission is treated as required if one of following criterias is
    // true:
    // 1. It's required explicitly.
    // 2. Any permission, which implies this permission, is required
    //    explicitly.
    return (impl_->required_ & info->mask) ||
           (impl_->required_ & info->included_by);
  }
  return false;
}

bool Permissions::IsRequiredAndGranted(int permission) const {
  return IsRequired(permission) && IsGranted(permission);
}

bool Permissions::HasUngranted() const {
  for (int i = FILE_READ; i <= ALL_ACCESS; ++i) {
    if (IsRequired(i) && !IsGranted(i))
      return true;
  }
  return false;
}

void Permissions::GrantAllRequired() {
  impl_->granted_ |= impl_->required_;
  impl_->denied_ &= ~impl_->required_;
}

void Permissions::RemoveAllRequired() {
  impl_->required_ = 0;
}

std::string Permissions::ToString() const {
  std::string value;
  for (int i = FILE_READ; i <= ALL_ACCESS; ++i) {
    if (impl_->granted_ & kPermissionsInfo[i].mask) {
      value.append("+");
      value.append(kPermissionsInfo[i].name);
      value.append(",");
    }
    if (impl_->denied_ & kPermissionsInfo[i].mask) {
      value.append("-");
      value.append(kPermissionsInfo[i].name);
      value.append(",");
    }
    if (impl_->required_ & kPermissionsInfo[i].mask) {
      value.append("#");
      value.append(kPermissionsInfo[i].name);
      value.append(",");
    }
  }

  // Erases the trailing comma.
  if (value.length())
    value.erase(value.length() - 1);

  return value;
}

void Permissions::FromString(const char *str) {
  ASSERT(str);
  impl_->granted_ = impl_->denied_ = impl_->required_ = 0;
  std::string value_str = std::string(str ? str : "");
  while(value_str.length()) {
    std::string perm_str;
    SplitString(value_str, ",", &perm_str, &value_str);
    if (perm_str.length() > 1) {
      char signature = perm_str[0];
      int perm = GetByName(perm_str.c_str() + 1);
      if (perm != -1) {
        if (signature == '+') {
          impl_->granted_ |= kPermissionsInfo[perm].mask;
          impl_->denied_ &= ~kPermissionsInfo[perm].mask;
        } else if (signature == '-') {
          impl_->denied_ |= kPermissionsInfo[perm].mask;
          impl_->granted_ &= ~kPermissionsInfo[perm].mask;
        } else if (signature == '#') {
          impl_->required_ |= kPermissionsInfo[perm].mask;
        }
      }
    }
  }
}

bool Permissions::EnumerateAllGranted(Slot1<bool, int> *slot) const {
  ASSERT(slot);
  bool result = false;
  if (slot) {
    for (int i = FILE_READ; i <= ALL_ACCESS; ++i) {
      if (IsGranted(i)) {
        result = (*slot)(i);
        if (!result) {
          delete slot;
          return false;
        }
      }
    }
    delete slot;
  }
  return result;
}

bool Permissions::EnumerateAllRequired(Slot1<bool, int> *slot) const {
  ASSERT(slot);
  bool result = false;
  if (slot) {
    for (int i = FILE_READ; i <= ALL_ACCESS; ++i) {
      if (IsRequired(i)) {
        result = (*slot)(i);
        if (!result) {
          delete slot;
          return false;
        }
      }
    }
    delete slot;
  }
  return result;
}

std::string Permissions::GetName(int permission) {
  return (permission >= FILE_READ && permission <= ALL_ACCESS) ?
         kPermissionsInfo[permission].name : "";
}

int Permissions::GetByName(const char *name) {
  if (name && *name) {
    for (int i = FILE_READ; i <= ALL_ACCESS; ++i) {
      if (strcmp(kPermissionsInfo[i].name, name) == 0)
        return i;
    }
  }
  return -1;
}

std::string Permissions::GetDescription(int permission) {
  if (permission >= FILE_READ && permission <= ALL_ACCESS)
    return GMS_(("PERMISSION_" + ToUpper(GetName(permission))).c_str());
  return "";
}

std::string Permissions::GetDescriptionForLocale(int permission,
                                                 const char * locale) {
  if (permission >= FILE_READ && permission <= ALL_ACCESS)
    return GMSL_(("PERMISSION_" + ToUpper(GetName(permission))).c_str(),
                 locale);
  return "";
}

} // namespace ggadget
