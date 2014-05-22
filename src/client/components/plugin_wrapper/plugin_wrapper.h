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

// Basic definitions of plugin wrapper
#ifndef GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_WRAPPER_H_
#define GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_WRAPPER_H_

// Type of an instance of a component in the dll. This type is opaque to the
// caller of dll interfaces.
typedef void* ComponentInstance;
// Type of the owner of a component in the dll. Usually it's a StubComponent.
// This type is opaque to the component inside the dll and it will be used in
// the callback functions.
typedef void* ComponentOwner;

#if defined(OS_WIN)
#define API_CALL __stdcall
#else
#define API_CALL
#endif

#endif  // GOOPY_COMPONENTS_PLUGIN_WRAPPER_PLUGIN_WRAPPER_H_
