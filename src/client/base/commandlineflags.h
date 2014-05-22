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

// Revamped and reorganized by Craig Silverstein
//
// This is the file that should be included by any file which declares
// or defines a command line flag or wants to parse command line flags
// or print a program usage message (which will include information about
// flags).  Executive summary, in the form of an example foo.cc file:
//
//    #include "foo.h"         // foo.h has a line "DECLARE_int32(start);"
//
//    DEFINE_int32(end, 1000, "The last record to read");
//    DECLARE_bool(verbose);   // some other file has a DEFINE_bool(verbose, ...)
//
//    void MyFunc() {
//      if (FLAGS_verbose) printf("Records %d-%d\n", FLAGS_start, FLAGS_end);
//    }
//
//    Then, at the command-line:
//       ./foo --noverbose --start=5 --end=100
//
// For more details, see
//    http://www/eng/howto/infrastructure/commandlineflags.html

#ifndef BASE_COMMANDLINEFLAGS_H_
#define BASE_COMMANDLINEFLAGS_H_

#include <assert.h>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/commandlineflags_declare.h"
#include "base/port.h"

// --------------------------------------------------------------------
// These methods are the best way to get access to info about the
// list of commandline flags.  Note that these routines are pretty slow.
//   GetAllFlags: mostly-complete info about the list, sorted by file.
//   ShowUsageWithFlags: pretty-prints the list to stdout (what --help does)
//   ShowUsageWithFlagsRestrict: limit to filenames with restrict as a substr
//
// In addition to accessing flags, you can also access argv[0] (the program
// name) and argv (the entire commandline), which we sock away a copy of.
// These variables are static, so you should only set them once.

struct CommandLineFlagInfo {
  string name;              // the name of the flag
  string type;              // the type of the flag: int32, etc
  string description;       // the "help text" associated with the flag
  string current_value;     // the current value, as a string
  string default_value;     // the default value, as a string
  string filename;          // 'cleaned' version of filename holding the flag
  bool is_default;          // true if the flag has default value
};

extern void GetAllFlags(vector<CommandLineFlagInfo>* OUTPUT);
// These two are actually defined in commandlineflags_reporting.cc.
extern void ShowUsageWithFlags(const char *argv0);  // what --help does

extern void SetArgv(int argc, const char** argv);
extern const vector<string>& GetArgvs();    // all of argv = vector of strings
extern const char* GetArgv();               // all of argv as a string
extern const char* GetArgv0();              // only argv0
extern uint32 GetArgvSum();                 // simple checksum of argv
extern const char* ProgramInvocationName(); // argv0, or "UNKNOWN" if not set
extern const char* ProgramInvocationShortName();   // basename(argv0)
extern const char* ProgramUsage();          // string set by SetUsageMessage()

// --------------------------------------------------------------------
// Normally you access commandline flags by just saying "if (FLAGS_foo)"
// or whatever, and set them by calling "FLAGS_foo = bar" (or, more
// commonly, via the DEFINE_foo macro).  But if you need a bit more
// control, we have programmatic ways to get/set the flags as well.

// Return true iff the flagname was found.
// OUTPUT is set to the flag's value, or unchanged if we return false.
extern bool GetCommandLineOption(const char* name, string* OUTPUT);

// Return true iff the flagname was found. OUTPUT is set to the flag's
// CommandLineFlagInfo or unchanged if we return false.
extern bool GetCommandLineFlagInfo(const char* name,
                                   CommandLineFlagInfo* OUTPUT);

// Return the CommandLineFlagInfo of the flagname.  exit() if name not found.
// Example usage, to check if a flag's value is currently the default value:
//   if (GetCommandLineFlagInfoOrDie("foo").is_default) ...
extern CommandLineFlagInfo GetCommandLineFlagInfoOrDie(const char* name);

enum FlagSettingMode {
  // update the flag's value (can call this multiple times).
  SET_FLAGS_VALUE,
  // update the flag's value, but *only if* it has not yet been updated
  // with SET_FLAGS_VALUE, SET_FLAG_IF_DEFAULT, or "FLAGS_xxx = nondef".
  SET_FLAG_IF_DEFAULT,
  // set the flag's default value to this.  If the flag has not yet updated
  // yet (via SET_FLAGS_VALUE, SET_FLAG_IF_DEFAULT, or "FLAGS_xxx = nondef")
  // change the flag's current value to the new default value as well.
  SET_FLAGS_DEFAULT
};

// Set a particular flag ("command line option").  Returns a string
// describing the new value that the option has been set to.  The
// return value API is not well-specified, so basically just depend on
// it to be empty if the setting failed for some reason -- the name is
// not a valid flag name, or the value is not a valid value -- and
// non-empty else.

// SetCommandLineOption uses set_mode == SET_FLAGS_VALUE (the common case)
extern string SetCommandLineOption(const char* name, const char* value);
extern string SetCommandLineOptionWithMode(const char* name, const char* value,
                                           FlagSettingMode set_mode);

// --------------------------------------------------------------------
// Useful routines for initializing flags from the environment.
// In each case, if 'varname' does not exist in the environment
// return defval.  If 'varname' does exist but is not valid
// (e.g., not a number for an int32 flag), abort with an error.
// Otherwise, return the value.  NOTE: for booleans, for true use
// 't' or 'T' or 'true' or '1', for false 'f' or 'F' or 'false' or '0'.

extern bool BoolFromEnv(const char *varname, bool defval);
extern int32 Int32FromEnv(const char *varname, int32 defval);
extern int64 Int64FromEnv(const char *varname, int64 defval);
extern uint64 Uint64FromEnv(const char *varname, uint64 defval);
extern double DoubleFromEnv(const char *varname, double defval);
extern const char *StringFromEnv(const char *varname, const char *defval);

// --------------------------------------------------------------------
// The next two functions parse commandlineflags from main():

// Set the "usage" message for this program.  For example:
//   string usage("This program does nothing.  Sample usage:\n");
//   usage += argv[0] + " <uselessarg1> <uselessarg2>";
//   SetUsageMessage(usage);
// Do not include commandline flags in the usage: we do that for you!
extern void SetUsageMessage(const string& usage);

// Looks for flags in argv and parses them.  Rearranges argv to put
// flags first, or removes them entirely if remove_flags is true.
// See top-of-file for more details on this function.
#ifndef SWIG   // In swig, use ParseCommandLineFlagsScript() instead.
extern uint32 ParseCommandLineFlags(int *argc, char*** argv,
                                    bool remove_flags);
#endif

#ifdef OS_WINDOWS
// (liuli): These two functions are added for supporting unicode and ascii
// command line parsing in windows. Caller should choose the function based
// on there settings.
extern uint32 ParseCommandLineFlags(wchar_t *command_line);
extern uint32 ParseCommandLineFlags(char *command_line);
#endif

// Calls to ParseCommandLineNonHelpFlags and then to
// HandleCommandLineHelpFlags can be used instead of a call to
// ParseCommandLineFlags during initialization, in order to allow for
// changing default values for some FLAGS (via
// e.g. SetCommandLineOptionWithMode calls) between the time of
// command line parsing and the time of dumping help information for
// the flags as a result of command line parsing.
extern uint32 ParseCommandLineNonHelpFlags(int *argc, char*** argv,
                                           bool remove_flags);
// This is actually defined in commandlineflags_reporting.cc.
// This function is misnamed (it also handles --version, etc.), but
// it's too late to change that now. :-(
extern void HandleCommandLineHelpFlags();   // in commandlineflags_reporting.cc

// Allow command line reparsing.  Disables the error normaly generated
// when an unknown flag is found, since it may be found in a later parse.
extern void AllowCommandLineReparsing();

// Reparse the flags that have not yet been recognized.
// Only flags registered since the last parse will be recognized.
// Any flag value must be provided as part of the argument using "=",
// not as a separate command line argument that follows the flag argument.
// Intended for handling flags from dynamically loaded libraries,
// since their flags are not registered until they are loaded.
extern uint32 ReparseCommandLineNonHelpFlags();

// --------------------------------------------------------------------
// Now come the command line flag declaration/definition macros that
// will actually be used.  They're kind of hairy.  A major reason
// for this is initialization: we want people to be able to access
// variables in global constructors and have that not crash, even if
// their global constructor runs before the global constructor here.
// (Obviously, we can't guarantee the flags will have the correct
// default value in that case, but at least accessing them is safe.)
// The only way to do that is have flags point to a static buffer.
// So we make one, using a union to ensure proper alignment, and
// then use placement-new to actually set up the flag with the
// correct default value.  In the same vein, we have to worry about
// flag access in global destructors, so FlagRegisterer has to be
// careful never to destroy the flag-values it constructs.
//
// Note that when we define a flag variable FLAGS_<name>, we also
// preemptively define a junk variable, FLAGS_no<name>.  This is to
// cause a link-time error if someone tries to define 2 flags with
// names like "logging" and "nologging".  We do this because a bool
// flag FLAG can be set from the command line to true with a "-FLAG"
// argument, and to false with a "-noFLAG" argument, and so this can
// potentially avert confusion.
//
// We also put flags into their own namespace.  It is purposefully
// named in an opaque way that people should have trouble typing
// directly.  The idea is that DEFINE puts the flag in the weird
// namespace, and DECLARE imports the flag from there into the current
// namespace.  The net result is to force people to use DECLARE to get
// access to a flag, rather than saying "extern bool FLAGS_whatever;"
// or some such instead.  We want this so we can put extra
// functionality (like sanity-checking) in DECLARE if we want, and
// make sure it is picked up everywhere.
//
// We also put the type of the variable in the namespace, so that
// people can't DECLARE_int32 something that they DEFINE_bool'd
// elsewhere.

class FlagRegisterer {
 public:
  FlagRegisterer(const char* name, const char* type,
                 const char* help, const char* filename,
                 void* current_storage, void* defvalue_storage);
 private:
  class CommandLineFlag* flag_;
};

#define DEFINE_VARIABLE(namespc, type, name, value, help)                      \
  namespace Flag_Names_##type {                                                \
    static union { void* align; char store[sizeof(namespc type)]; } cur_##name;\
    static union { void* align; char store[sizeof(namespc type)]; } dfl_##name;\
    static FlagRegisterer object_##name(                                       \
      #name, #type, help, __FILE__,                                            \
      new (cur_##name.store) namespc type(value),                              \
      new (dfl_##name.store) namespc type(value));                             \
    namespc type& FLAGS_##name =                                               \
      *(reinterpret_cast<namespc type*>(cur_##name.store));                    \
    char FLAGS_no##name;                                                       \
  }                                                                            \
  using Flag_Names_##type::FLAGS_##name

#ifndef SWIG  // In swig, ignore the main flag declarations

#define DEFINE_bool(name, val, txt)    DEFINE_VARIABLE(, bool, name, val, txt)
#define DEFINE_int32(name, val, txt)   DEFINE_VARIABLE(, int32, name, val, txt)
#define DEFINE_int64(name, val, txt)   DEFINE_VARIABLE(, int64, name, val, txt)
#define DEFINE_uint64(name, val, txt)  DEFINE_VARIABLE(, uint64, name, val, txt)
#define DEFINE_double(name, val, txt)  DEFINE_VARIABLE(, double, name, val, txt)
#define DEFINE_string(name, val, txt)  DEFINE_VARIABLE(, string, name, val, txt)

#endif  // SWIG

#endif  // BASE_COMMANDLINEFLAGS_H_
