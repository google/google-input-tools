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

#ifndef GOOPY_BASE_CPU_H_
#define GOOPY_BASE_CPU_H_
#pragma once

#include <string>

#include "build/build_config.h"

namespace base {

// Query information about the processor.
class CPU {
 public:
  // Constructor
  CPU();

  // Accessors for CPU information.
  const std::string& vendor_name() const { return cpu_vendor_; }
  int stepping() const { return stepping_; }
  int model() const { return model_; }
  int family() const { return family_; }
  int type() const { return type_; }
  int extended_model() const { return ext_model_; }
  int extended_family() const { return ext_family_; }
  int has_mmx() const { return has_mmx_; }
  int has_sse() const { return has_sse_; }
  int has_sse2() const { return has_sse2_; }
  int has_sse3() const { return has_sse3_; }
  int has_ssse3() const { return has_ssse3_; }
  int has_sse41() const { return has_sse41_; }
  int has_sse42() const { return has_sse42_; }

 private:
  // Query the processor for CPUID information.
  void Initialize();

  int type_;  // process type
  int family_;  // family of the processor
  int model_;  // model of processor
  int stepping_;  // processor revision number
  int ext_model_;
  int ext_family_;
  bool has_mmx_;
  bool has_sse_;
  bool has_sse2_;
  bool has_sse3_;
  bool has_ssse3_;
  bool has_sse41_;
  bool has_sse42_;
  std::string cpu_vendor_;
};

}  // namespace base

#endif  // GOOPY_BASE_CPU_H_
