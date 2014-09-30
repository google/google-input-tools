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

//
// Contains functions to initialize a google process.

#ifndef BASE_INIT_GOOGLE_H_
#define BASE_INIT_GOOGLE_H_

// Initializes misc google-related things in the binary. This includes
// initializing services such as command line flags, walltime, main-thread-id,
// logging directories, malloc extension, kernel version, and chrooting.
//
// In particular it does REQUIRE_MODULE_INITIALIZED(command_line_flags_parsing)
// parses command line flags and does RUN_MODULE_INITIALIZERS() (in that order).
// If a flag is defined more than once in the command line or flag
// file, the last definition is used.
//
// Typically called early on in main() and must be called before other
// threads start using functions from this file.
//
// 'usage' provides a short usage message passed to SetUsageMessage().
//         Most callers provide the name of the app as 'usage' ?!
// 'argc' and 'argv' are the command line flags to parse.
// If 'remove_flags' then parsed flags are removed.
void InitGoogle(const char* usage, int* argc, char*** argv, bool remove_flags);

// Normally, InitGoogle will chroot (if requested with the --chroot flag)
// and setuid to --uid and --gid (default nobody).
// This version will not, and you will be responsible for calling
// ChangeRootAndUser
// This option is provided for applications that need to read files outside
// the chroot before chrooting.
void InitGoogleExceptChangeRootAndUser(const char* usage, int* argc,
                                       char*** argv, bool remove_flags);

// Thread-hostile.
void ChangeRootAndUser();

// Checks (only in debug mode) if InitGoogle() has been fully executed
// and crashes if it has not been.
// Intended for checking that code that depends on complete execution
// of InitGoogle() for its proper functioning is safe to execute.
void AssertInitGoogleIsDone();

// Checks (in all modes) whether InitGoogle() has been fully executed.
// May either crash or print an error message if it has not been.
// The intent is that certain uses (based on stack trace) will cause errors
// initially, and be converted to crashes once the those uses are eliminated.
// Any error message output if prefixed with "message".
void CheckInitGoogleIsDone(const char *message);

// TODO: Remove as soon as TI-Envelope is deployed.
namespace borg { void InitBorgletLib(); }
class BorgletLibPhantomDetectionInit {
 private:
  // A temporary solution to start PhantomSpotter from BorgletLib.
  // May only be called once from borg::InitBorgletLib().
  static void Set(void (*initializer)());
  friend void ::borg::InitBorgletLib();
};

namespace unified_logging { void SetupLogSinkInit(); }
class UnifiedLoggingSinkInitializer {
 private:
  // Register an initialization routine for Unified Logging debug log handling.
  // May be called at most once.
  static void Set(void (*initializer)());
  friend void ::unified_logging::SetupLogSinkInit();
};

#endif  // BASE_INIT_GOOGLE_H_
