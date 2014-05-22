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

// Author: Ray Sidney
// Revamped and reorganized by Craig Silverstein
//
// This file contains the implementation of all our command line flags
// stuff.
//
// TODO: What to do, if anything, with /home/amod/FLAGS.sh?

#include "base/commandlineflags.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#if !defined(OS_WINDOWS)
#include <pthread.h>
#endif  // !OS_WINDOWS
#include <string.h>
#if defined(OS_WINDOWS)
#include <windows.h>
#endif  // OS_WINDOWS

#include <algorithm>
#include <map>
#include <string>
#include <utility>     // for pair<>
#include <vector>

#include "base/basictypes.h"
#include "base/port.h"

static const char kError[] = "ERROR: ";

// Indicates that undefined options are to be ignored.
// Enables deferred processing of flags in dynamically loaded libraries.
static bool allow_command_line_reparsing = false;

static bool logging_is_probably_set_up = false;

// --------------------------------------------------------------------
// FlagValue
//    This represent the value a single flag might have.  The major
//    functionality is to convert from a string to an object of a
//    given type, and back.
// --------------------------------------------------------------------

class FlagValue {
 public:
  FlagValue(void* valbuf, const char* type);
  ~FlagValue();

  bool ParseFrom(const char* spec);
  string ToString() const;

 private:
  friend class CommandLineFlag;
  template <typename T> friend T GetFromEnv(const char*, const char*, T);

  enum ValueType {FV_BOOL, FV_INT32, FV_INT64, FV_UINT64, FV_DOUBLE, FV_STRING};

  const char* TypeName() const;
  bool Equal(const FlagValue& x) const;
  FlagValue* New() const;   // creates a new one with default value
  void CopyFrom(const FlagValue& x);

  void* value_buffer_;          // points to the buffer holding our data
  bool we_own_buffer_;          // true iff we new-ed the buffer
  ValueType type_;              // how to interpret value_

  FlagValue(const FlagValue&);   // no copying!
  void operator=(const FlagValue&);
};


// This could be a templated method of FlagValue, but doing so adds to the
// size of the .o.  Since there's no type-safety here anyway, macro is ok.
#define VALUE_AS(type)  *reinterpret_cast<type*>(value_buffer_)
#define OTHER_VALUE_AS(fv, type)  *reinterpret_cast<type*>(fv.value_buffer_)
#define SET_VALUE_AS(type, value)  VALUE_AS(type) = static_cast<type>(value)

FlagValue::FlagValue(void* valbuf, const char* type) : value_buffer_(valbuf) {
  if      (strcmp(type, "bool") == 0)  type_ = FV_BOOL;
  else if (strcmp(type, "int32") == 0)  type_ = FV_INT32;
  else if (strcmp(type, "int64") == 0)  type_ = FV_INT64;
  else if (strcmp(type, "uint64") == 0)  type_ = FV_UINT64;
  else if (strcmp(type, "double") == 0)  type_ = FV_DOUBLE;
  else if (strcmp(type, "string") == 0)  type_ = FV_STRING;
  else assert(false); // Unknown typename
}

FlagValue::~FlagValue() {
  switch (type_) {
    case FV_BOOL: delete reinterpret_cast<bool*>(value_buffer_); break;
    case FV_INT32: delete reinterpret_cast<int32*>(value_buffer_); break;
    case FV_INT64: delete reinterpret_cast<int64*>(value_buffer_); break;
    case FV_UINT64: delete reinterpret_cast<uint64*>(value_buffer_); break;
    case FV_DOUBLE: delete reinterpret_cast<double*>(value_buffer_); break;
    case FV_STRING: delete reinterpret_cast<string*>(value_buffer_); break;
  }
}

bool FlagValue::ParseFrom(const char* value) {
  if (type_ == FV_BOOL) {
    const char* kTrue[] = { "1", "t", "true", "y", "yes" };
    const char* kFalse[] = { "0", "f", "false", "n", "no" };
    COMPILE_ASSERT(sizeof(kTrue) == sizeof(kFalse), true_false_equal);
    for (int i = 0; i < sizeof(kTrue)/sizeof(*kTrue); ++i) {
      if (strcasecmp(value, kTrue[i]) == 0) {
        SET_VALUE_AS(bool, true);
        return true;
      } else if (strcasecmp(value, kFalse[i]) == 0) {
        SET_VALUE_AS(bool, false);
        return true;
      }
    }
    return false;   // didn't match a legal input

  } else if (type_ == FV_STRING) {
    SET_VALUE_AS(string, value);
    return true;
  }

  // OK, it's likely to be numeric, and we'll be using a strtoXXX method.
  if (value[0] == '\0')   // empty-string is only allowed for string type.
    return false;
  char* end;
  // Leading 0x puts us in base 16.  But leading 0 does not put us in base 8!
  // It caused too many bugs when we had that behavior.
  int base = 10;    // by default
  if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X'))
    base = 16;
  errno = 0;

  switch (type_) {
    case FV_INT32: {
      const int64 r = strtoq(value, &end, base);
      if (errno || end != value + strlen(value))  return false;  // bad parse
      if (static_cast<int32>(r) != r)  // worked, but number out of range
        return false;
      SET_VALUE_AS(int32, r);
      return true;
    }
    case FV_INT64: {
      const int64 r = strtoq(value, &end, base);
      if (errno || end != value + strlen(value))  return false;  // bad parse
      SET_VALUE_AS(int64, r);
      return true;
    }
    case FV_UINT64: {
      while (*value == ' ') value++;
      if (*value == '-') return false;  // negative number
      const uint64 r = strtouq(value, &end, base);
      if (errno || end != value + strlen(value))  return false;  // bad parse
      SET_VALUE_AS(uint64, r);
      return true;
    }
    case FV_DOUBLE: {
      const double r = strtod(value, &end);
      if (errno || end != value + strlen(value))  return false;  // bad parse
      SET_VALUE_AS(double, r);
      return true;
    }
    default: {
      assert(false); // unknown type
      return false;
    }
  }
}

string FlagValue::ToString() const {
  char intbuf[64];    // enough to hold even the biggest number
  switch (type_) {
    case FV_BOOL:
      return VALUE_AS(bool) ? "true" : "false";
    case FV_INT32:
      snprintf(intbuf, sizeof(intbuf), "%d", VALUE_AS(int32));
      return intbuf;
    case FV_INT64:
      snprintf(intbuf, sizeof(intbuf), "%lld", VALUE_AS(int64));
      return intbuf;
    case FV_UINT64:
      snprintf(intbuf, sizeof(intbuf), "%llu", VALUE_AS(uint64));
      return intbuf;
    case FV_DOUBLE:
      snprintf(intbuf, sizeof(intbuf), "%.17g", VALUE_AS(double));
      return intbuf;
    case FV_STRING:
      return VALUE_AS(string);
    default:
      assert(false); return ""; // unknown type
  }
}

const char* FlagValue::TypeName() const {
  switch (type_) {
    case FV_BOOL:   return "bool";
    case FV_INT32:  return "int32";
    case FV_INT64:  return "int64";
    case FV_UINT64: return "uint64";
    case FV_DOUBLE: return "double";
    case FV_STRING: return "string";
    default: assert(false); return ""; // unknown type
  }
}

bool FlagValue::Equal(const FlagValue& x) const {
  if (type_ != x.type_)
    return false;
  switch (type_) {
    case FV_BOOL:   return VALUE_AS(bool) == OTHER_VALUE_AS(x, bool);
    case FV_INT32:  return VALUE_AS(int32) == OTHER_VALUE_AS(x, int32);
    case FV_INT64:  return VALUE_AS(int64) == OTHER_VALUE_AS(x, int64);
    case FV_UINT64: return VALUE_AS(uint64) == OTHER_VALUE_AS(x, uint64);
    case FV_DOUBLE: return VALUE_AS(double) == OTHER_VALUE_AS(x, double);
    case FV_STRING: return VALUE_AS(string) == OTHER_VALUE_AS(x, string);
    default: assert(false); return false; // unknown type
  }
}

FlagValue* FlagValue::New() const {
  switch (type_) {
    case FV_BOOL:   return new FlagValue(new bool, "bool");
    case FV_INT32:  return new FlagValue(new int32, "int32");
    case FV_INT64:  return new FlagValue(new int64, "int64");
    case FV_UINT64: return new FlagValue(new uint64, "uint64");
    case FV_DOUBLE: return new FlagValue(new double, "double");
    case FV_STRING: return new FlagValue(new string, "string");
    default: assert(false); return NULL; // assert false
  }
}

void FlagValue::CopyFrom(const FlagValue& x) {
  assert(type_ == x.type_);
  switch (type_) {
    case FV_BOOL:   SET_VALUE_AS(bool, OTHER_VALUE_AS(x, bool));      break;
    case FV_INT32:  SET_VALUE_AS(int32, OTHER_VALUE_AS(x, int32));    break;
    case FV_INT64:  SET_VALUE_AS(int64, OTHER_VALUE_AS(x, int64));    break;
    case FV_UINT64: SET_VALUE_AS(uint64, OTHER_VALUE_AS(x, uint64));  break;
    case FV_DOUBLE: SET_VALUE_AS(double, OTHER_VALUE_AS(x, double));  break;
    case FV_STRING: SET_VALUE_AS(string, OTHER_VALUE_AS(x, string));  break;
    default: assert(false); // unknown type
  }
}

// --------------------------------------------------------------------
// CommandLineFlag
//    This represents a single flag, including its name, description,
//    default value, and current value.  Mostly this serves as a
//    struct, though it also knows how to register itself.
// --------------------------------------------------------------------

class CommandLineFlag {
 public:
  // Note: we take over memory-ownership of current_val and default_val.
  CommandLineFlag(const char* name, const char* help, const char* filename,
                  FlagValue* current_val, FlagValue* default_val);
  ~CommandLineFlag();

  const char* name() const { return name_; }
  const char* help() const { return help_; }
  const char* filename() const { return file_; }
  const char* CleanFileName() const;  // nixes irrelevant prefix such as homedir
  string current_value() const { return current_->ToString(); }
  string default_value() const { return defvalue_->ToString(); }
  const char* type_name() const { return defvalue_->TypeName(); }

  void FillCommandLineFlagInfo(struct CommandLineFlagInfo* result);

 private:
  friend class FlagRegistry;   // for SetFlagLocked()
  friend bool GetCommandLineOption(const char*, string*, bool*);

  // This copies all the non-const members: modified, processed, defvalue, etc.
  void CopyFrom(const CommandLineFlag& src);

  void UpdateModifiedBit();

  const char* const name_;     // Flag name
  const char* const help_;     // Help message
  const char* const file_;     // Which file did this come from?
  bool modified_;              // Set after default assignment?
  FlagValue* defvalue_;        // Default value for flag
  FlagValue* current_;         // Current value for flag

  CommandLineFlag(const CommandLineFlag&);   // no copying!
  void operator=(const CommandLineFlag&);
};

CommandLineFlag::CommandLineFlag(const char* name, const char* help,
                                 const char* filename,
                                 FlagValue* current_val, FlagValue* default_val)
    : name_(name), help_(help), file_(filename), modified_(false),
      defvalue_(default_val), current_(current_val) {
}

CommandLineFlag::~CommandLineFlag() {
  delete current_;
  delete defvalue_;
}

const char* CommandLineFlag::CleanFileName() const {
  // Compute top-level directory & file that this appears in
  // search full path backwards.  Set kMaxSlashes = 5,
  // as the current code has <= 4 levels of dirs.
  //   E.g. .../froogle/wrapping/autowrap/clustering/*.cc
  // Also, stop going backwards at "/google3/"; and skip by the first slash.
  // E.g.
  //   filename_where_defined = "froogle/wrapping/autowrap/clustering/**.cc"
  //   filename_where_defined = "file/util/fileutil.cc"
  static const int kMaxSlashes = 5;    // one more than max dir levels
  static const char kGoogle[] = "/google3/";

  if (sizeof(kGoogle)-1 == 0)          // no prefix to strip
    return filename();

  const char* clean_name = filename() + strlen(filename()) - 1;
  int slashes = 0;
  while ( clean_name > filename() ) {
    if (*clean_name == PATH_SEPARATOR) {
      ++slashes;
      if (slashes == kMaxSlashes) {
        break;    // no dirs now are deeper than this
      } else if (strncmp(clean_name, kGoogle, sizeof(kGoogle)-1) == 0) {
        // ".../google3/base/logging.cc" ==> "base/logging.cc"
        clean_name += sizeof(kGoogle)-1;    // past "/google3/"
        break;
      }
    }
    --clean_name;
  }
  while ( *clean_name == PATH_SEPARATOR ) ++clean_name;  // Skip any slashes
  return clean_name;
}

void CommandLineFlag::FillCommandLineFlagInfo(
    CommandLineFlagInfo* result) {
  result->name = name();
  result->type = type_name();
  result->description = help();
  result->current_value = current_value();
  result->default_value = default_value();
  result->filename = CleanFileName();
  UpdateModifiedBit();
  result->is_default = !modified_;
}

void CommandLineFlag::UpdateModifiedBit() {
  // Update the "modified" bit in case somebody bypassed the
  // Flags API and wrote directly through the FLAGS_name variable.
  if (!modified_ && !current_->Equal(*defvalue_)) {
    modified_ = true;
  }
}

void CommandLineFlag::CopyFrom(const CommandLineFlag& src) {
  // Note we only copy the non-const members; others are fixed at construct time
  modified_ = src.modified_;
  current_->CopyFrom(*src.current_);
  defvalue_->CopyFrom(*src.defvalue_);
}


// --------------------------------------------------------------------
// FlagRegistry
//    A FlagRegistry singleton object holds all flag objects indexed
//    by their names so that if you know a flag's name (as a C
//    string), you can access or set it.  If the function is named
//    FooLocked(), you must own the registry lock before calling
//    the function; otherwise, you should *not* hold the lock, and
//    the function will acquire it itself if needed.
// --------------------------------------------------------------------

struct StringCmp {  // Used by the FlagRegistry map class to compare char*'s
  bool operator() (const char* s1, const char* s2) const {
    return (strcmp(s1, s2) < 0);
  }
};

#define SAFE_PTHREAD(fncall)  do { if ((fncall) != 0) abort(); } while (0)

class FlagRegistry {
  // Internal lock implementation based on different OS.
  class ThreadLock {
    public:
      ThreadLock() {
#ifdef OS_WINDOWS
        InitializeCriticalSection(&lock_);
#else
        pthread_mutex_init(&lock_, NULL);
#endif
      }

      ~ThreadLock() {
#ifdef OS_WINDOWS
        DeleteCriticalSection(&lock_);
#else
        pthread_mutex_destroy(&lock_);
#endif
      }

      // Return 0 means success. Otherwaise fail.
      int Lock() {
#ifdef OS_WINDOWS
        EnterCriticalSection(&lock_);
        // Aways assume success.
        return 0;
#else
        return pthread_mutex_lock(&lock_);
#endif
      }

      // Return 0 means success. Otherwaise fail.
      int Unlock() {
#ifdef OS_WINDOWS
        LeaveCriticalSection(&lock_);
        // Aways assume success.
        return 0;
#else
        return pthread_mutex_unlock(&lock_);
#endif
      }

    private:
#ifdef OS_WINDOWS
      CRITICAL_SECTION lock_;
#else
      pthread_mutex_t lock_;
#endif
  };    // ThreadLock

 public:
  FlagRegistry() { }
  ~FlagRegistry() { }

  // Store a flag in this registry.  Takes ownership of the given pointer.
  void RegisterFlag(CommandLineFlag* flag);

  void Lock() { SAFE_PTHREAD(lock_.Lock()); }
  void Unlock() { SAFE_PTHREAD(lock_.Unlock()); }

  // Returns the flag object for the specified name, or NULL if not found.
  CommandLineFlag* FindFlagLocked(const char* name);

  // A fancier form of FindFlag that works correctly if name is of the
  // form flag=value.  In that case, we set key to point to flag, and
  // modify v to point to the value, and return the flag with the
  // given name (or NULL if not found).
  CommandLineFlag* SplitArgumentLocked(const char* argument,
                                       string* key, const char** v);

  // Set the value of a flag.  If the flag was successfully set to
  // value, set msg to indicate the new flag-value, and return true.
  // Otherwise, set msg to indicate the error, leave flag unchanged,
  // and return false.  msg can be NULL.
  bool SetFlagLocked(CommandLineFlag* flag, const char* value,
                     FlagSettingMode set_mode, string* msg);

  static FlagRegistry* GlobalRegistry();   // returns a singleton registry

 private:
  friend void GetAllFlags(vector<CommandLineFlagInfo>*);

  typedef map<const char*, CommandLineFlag*, StringCmp> FlagMap;
  typedef FlagMap::iterator FlagIterator;
  typedef FlagMap::const_iterator FlagConstIterator;
  FlagMap flags_;
  ThreadLock lock_;

  static FlagRegistry* global_registry_;   // a singleton registry

  static void InitGlobalRegistry();

  // Disallow
  FlagRegistry(const FlagRegistry&);
  FlagRegistry& operator=(const FlagRegistry&);
};

void FlagRegistry::RegisterFlag(CommandLineFlag* flag) {
  Lock();
  pair<FlagIterator, bool> ins =
    flags_.insert(pair<const char*, CommandLineFlag*>(flag->name(), flag));
  if (ins.second == false) {   // means the name was already in the map
    if (strcmp(ins.first->second->filename(), flag->filename()) != 0) {
      fprintf(stderr,
              "ERROR: flag '%s' was defined more than once "
              "(in files '%s' and '%s').\n",
              flag->name(),
              ins.first->second->filename(),
              flag->filename());
    } else {
      fprintf(stderr,
              "ERROR: something wrong with flag '%s' in file '%s'.  "
              "One possibility: file '%s' is being linked both statically "
              "and dynamically into this executable.\n",
              flag->name(),
              flag->filename(), flag->filename());
    }
    exit(1);
  }
  Unlock();
}

CommandLineFlag* FlagRegistry::FindFlagLocked(const char* name) {
  FlagConstIterator i = flags_.find(name);
  if (i == flags_.end()) {
    return NULL;
  } else {
    return i->second;
  }
}

CommandLineFlag* FlagRegistry::SplitArgumentLocked(const char* arg,
                                                   string* key,
                                                   const char** v) {
  // Find the flag object for this option
  const char* flag_name;
  const char* value = strchr(arg, '=');
  if (value == NULL) {
    key->assign(arg);
    *v = NULL;
  } else {
    // Strip out the "=value" portion from arg
    key->assign(arg, value-arg);
    *v = ++value;    // advance past the '='
  }
  flag_name = key->c_str();

  CommandLineFlag* flag = FindFlagLocked(flag_name);
  if (flag == NULL && (flag_name[0] == 'n') && (flag_name[1] == 'o')) {
    // See if we can find a boolean flag named "x" for an option
    // named "nox".
    flag = FindFlagLocked(flag_name+2);
    if (flag != NULL) {
      if (strcmp(flag->type_name(), "bool") != 0) {
        // This is not a boolean flag, so we should not strip the "no" prefix
        flag = NULL;
      } else {
        // Make up a fake value to replace the "no" we stripped out
        key->assign(flag_name+2);   // the name without the "no"
        *v = "0";
      }
    }
  }

  if (flag == NULL) {
    return NULL;
  }

  // Assign a value if this is a boolean flag
  if (*v == NULL && strcmp(flag->type_name(), "bool") == 0) {
    *v = "1";    // the --nox case was already handled, so this is the --x case
  }

  return flag;
}

// Can't make this static because of friendship.
inline bool TryParse(const CommandLineFlag* flag, FlagValue* flag_value,
                     const char* value, string* msg) {
  if (flag_value->ParseFrom(value)) {
    if (msg)
      *msg += (string(flag->name()) + " set to " + flag_value->ToString()
               + "\n");
    return true;
  } else {
    if (msg)
      *msg += (string(kError) + "illegal value '" + value +
               + "' specified for " + flag->type_name() + " flag '"
               + flag->name() + "'\n");
    return false;
  }
}

bool FlagRegistry::SetFlagLocked(CommandLineFlag* flag,
                                 const char* value,
                                 FlagSettingMode set_mode,
                                 string* msg) {
  flag->UpdateModifiedBit();
  switch (set_mode) {
    case SET_FLAGS_VALUE: {
      // set or modify the flag's value
      if (!TryParse(flag, flag->current_, value, msg))
        return false;
      flag->modified_ = true;
      break;
    }
    case SET_FLAG_IF_DEFAULT: {
      // set the flag's value, but only if it hasn't been set by someone else
      if (!flag->modified_) {
        if (!TryParse(flag, flag->current_, value, msg))
          return false;
        flag->modified_ = true;
      } else {
        *msg = string(flag->name()) + " set to " + flag->current_value();
      }
      break;
    }
    case SET_FLAGS_DEFAULT: {
      // modify the flag's default-value
      if (!TryParse(flag, flag->defvalue_, value, msg))
        return false;
      if (!flag->modified_) {
        // Need to set both defvalue *and* current, in this case
        TryParse(flag, flag->current_, value, NULL);
      }
      break;
    }
    default: {
      // unknown set_mode
      assert(false); return false;
    }
  }

  return true;
}

// Get the singleton FlagRegistry object
FlagRegistry* FlagRegistry::global_registry_ = NULL;

void FlagRegistry::InitGlobalRegistry() {
  global_registry_ = new FlagRegistry;
}

FlagRegistry* FlagRegistry::GlobalRegistry() {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    FlagRegistry::InitGlobalRegistry();
  }
  return global_registry_;
}

// --------------------------------------------------------------------
// FlagRegisterer
//    This class exists merely to have a global constructor (the
//    kind that runs before main(), that goes an initializes each
//    flag that's been declared.  Note that it's very important we
//    don't have a destructor that deletes flag_, because that would
//    cause us to delete current_storage/defvalue_storage as well,
//    which can cause a crash if anything tries to access the flag
//    values in a global destructor.
// --------------------------------------------------------------------

FlagRegisterer::FlagRegisterer(const char* name, const char* type,
                               const char* help, const char* filename,
                               void* current_storage, void* defvalue_storage) {
  FlagValue* current = new FlagValue(current_storage, type);
  FlagValue* defvalue = new FlagValue(defvalue_storage, type);
  // Importantly, flag_ will never be deleted, so storage is always good.
  flag_ = new CommandLineFlag(name, help, filename, current, defvalue);
  FlagRegistry::GlobalRegistry()->RegisterFlag(flag_);   // default registry
}


// --------------------------------------------------------------------
// GetAllFlags()
//    The main way the FlagRegistry class exposes its data.  This
//    returns, as strings, all the info about all the flags in
//    the main registry, sorted first by filename they are defined
//    in, and then by flagname.
// --------------------------------------------------------------------

struct FilenameFlagnameCmp {
  bool operator()(const CommandLineFlagInfo& a,
                  const CommandLineFlagInfo& b) const {
    int cmp = strcmp(a.filename.c_str(), b.filename.c_str());
    if (cmp == 0)
      cmp = strcmp(a.name.c_str(), b.name.c_str());  // secondary sort key
    return cmp < 0;
  }
};

void GetAllFlags(vector<CommandLineFlagInfo>* OUTPUT) {
  FlagRegistry* const registry = FlagRegistry::GlobalRegistry();
  registry->Lock();
  for (FlagRegistry::FlagConstIterator i = registry->flags_.begin();
       i != registry->flags_.end(); ++i) {
    CommandLineFlagInfo fi;
    i->second->FillCommandLineFlagInfo(&fi);
    OUTPUT->push_back(fi);
  }
  registry->Unlock();
  // Now sort the flags, first by filename they occur in, then alphabetically
  sort(OUTPUT->begin(), OUTPUT->end(), FilenameFlagnameCmp());
}

// --------------------------------------------------------------------
// SetArgv()
// GetArgvs()
// GetArgv()
// GetArgv0()
// ProgramInvocationName()
// ProgramInvocationShortName()
// SetUsageMessage()
// ProgramUsage()
//    Functions to set and get argv.  Typically the setter is called
//    by ParseCommandLineFlags.  Also can get the ProgramUsage string,
//    set by SetUsageMessage.
// --------------------------------------------------------------------

// These values are not protected by a Mutex because they are normally
// set only once during program startup.
static const char* argv0 = "UNKNOWN";      // just the program name
static const char* cmdline = "";           // the entire command-line
static vector<string> argvs;
static uint32 argv_sum = 0;
static const char* program_usage = "Warning: SetUsageMessage() never called";
static bool program_usage_set = false;

void SetArgv(int argc, const char** argv) {
  static bool called_set_argv = false;
  if (called_set_argv)         // we already have an argv for you
    return;

  called_set_argv = true;

  assert(argc > 0);            // every program has at least a progname
  argv0 = strdup(argv[0]);     // small memory leak, but fn only called once
  assert(argv0);

  string cmdline_string = string("");        // easier than doing strcats
  argvs.clear();
  for (int i = 0; i < argc; i++) {
    if (i != 0)
      cmdline_string += " ";
    cmdline_string += argv[i];
    argvs.push_back(argv[i]);
  }
  cmdline = strdup(cmdline_string.c_str());  // another small memory leak
  assert(cmdline);

  // Compute a simple sum of all the chars in argv
  argv_sum = 0;
  for (const char* c = cmdline; *c; c++)
    argv_sum += *c;
}

const vector<string>& GetArgvs() { return argvs; }
const char* GetArgv()            { return cmdline; }
const char* GetArgv0()           { return argv0; }
uint32 GetArgvSum()              { return argv_sum; }
const char* ProgramInvocationName() {             // like the GNU libc fn
  return GetArgv0();
}
const char* ProgramInvocationShortName() {        // like the GNU libc fn
  const char* slash = strrchr(argv0, '/');
#ifdef OS_WINDOWS
  if (!slash)  slash = strrchr(argv0, '\\');
#endif
  return slash ? slash + 1 : argv0;
}

void SetUsageMessage(const string& usage) {
  if (program_usage_set) {
    fprintf(stderr, "ERROR: SetUsageMessage() called more than once\n");
    exit(1);
  }

  program_usage = strdup(usage.c_str());      // small memory leak
  program_usage_set = true;
}

const char* ProgramUsage() {
  return program_usage;
}

// --------------------------------------------------------------------
// CommandLineFlagParser
//    Parsing is done in two stages.  In the first, we go through
//    argv.  For every flag-like arg we can make sense of, we parse
//    it and set the appropriate FLAGS_* variable.  For every flag-
//    like arg we can't make sense of, we store it in a vector,
//    along with an explanation of the trouble.  In stage 2, we
//    handle the 'reporting' flags like --help and --mpm_version.
//    (This is via a call to HandleCommandLineHelpFlags(), in
//    commandlineflags_reporting.cc.)
//    An optional stage 3 prints out the error messages.
//       This is a bit of a simplification.  For instance, --flagfile
//    is handled as soon as it's seen in stage 1, not in stage 2.
// --------------------------------------------------------------------

class CommandLineFlagParser {
 public:
  // The argument is the flag-registry to register the parsed flags in
  explicit CommandLineFlagParser(FlagRegistry* reg) : registry_(reg) {}
  ~CommandLineFlagParser() {}

  // Stage 1: Every time this is called, it reads all flags in argv.
  // However, it ignores all flags that have been successfully set
  // before.  Typically this is only called once, so this 'reparsing'
  // behavior isn't important.  It can be useful when trying to
  // reparse after loading a dll, though.
  uint32 ParseNewCommandLineFlags(int* argc, char*** argv, bool remove_flags);

  // Stage 2: print reporting info and exit, if requested.
  // In commandlineflags_reporting.cc:HandleCommandLineHelpFlags().

  // Stage 3: report any errors and return true if any were found.
  bool ReportErrors();

  // Set a particular command line option.  "newval" is a string
  // describing the new value that the option has been set to.  If
  // option_name does not specify a valid option name, or value is not
  // a valid value for option_name, newval is empty.  Does recursive
  // processing for --flagfile and --fromenv.  Returns the new value
  // if everything went ok, or empty-string if not.  (Actually, the
  // return-string could hold many flag/value pairs due to --flagfile.)
  // NB: Must have called registry_->Lock() before calling this function.
  string ProcessSingleOptionLocked(CommandLineFlag* flag,
                                   const char* value,
                                   FlagSettingMode set_mode);

 private:
  FlagRegistry* const registry_;
  map<string, string> error_flags_;     // map from name to error message
  // This could be a set<string>, but we reuse the map to minimize the .o size
  map<string, string> undefined_names_; // --name for name that's not registered
};


// Parse a list of (comma-separated) flags.
static void ParseFlagList(const char* value, vector<string>* flags) {
  for (const char *p = value; p && *p; value = p) {
    p = strchr(value, ',');
    size_t len;
    if (p) {
      len = p - value;
      p++;
    } else {
      len = strlen(value);
    }

    if (len == 0) {
      fprintf(stderr, "ERROR: empty flaglist entry\n");
      exit(1);
    }
    if (value[0] == '-') {
      fprintf(stderr, "ERROR: flag \"%*s\" begins with '-'\n", len, value);
      exit(1);
    }

    flags->push_back(string(value, len));
  }
}

// Snarf an entire file into a C++ string.  This is just so that we
// can do all the I/O in one place and not worry about it everywhere.
// Plus, it's convenient to have the whole file contents at hand.
// Adds a newline at the end of the file.
#define PFATAL(s)  do { perror(s); exit(1); } while (0)

static string ReadFileIntoString(const char* filename) {
  const int bufsize = 8092;
  char buffer[bufsize];
  string s;
  FILE* fp = fopen(filename, "r");
  if (!fp)  PFATAL(filename);
  size_t n;
  while ( (n=fread(buffer, 1, bufsize, fp)) > 0 ) {
    if (ferror(fp))  PFATAL(filename);
    s.append(buffer, n);
  }
  fclose(fp);
  return s;
}

uint32 CommandLineFlagParser::ParseNewCommandLineFlags(int* argc, char*** argv,
                                                       bool remove_flags) {
  const char *program_name = strrchr((*argv)[0], PATH_SEPARATOR);   // nix path
  program_name = (program_name == NULL ? (*argv)[0] : program_name+1);

  int first_nonopt = *argc;        // for non-options moved to the end

  registry_->Lock();
  for (int i = 1; i < first_nonopt; i++) {
    char* arg = (*argv)[i];

    // Like getopt(), we permute non-option flags to be at the end.
    if (arg[0] != '-') {           // must be a program argument
      memmove((*argv) + i, (*argv) + i+1, (*argc - (i+1)) * sizeof((*argv)[i]));
      (*argv)[*argc-1] = arg;      // we go last
      first_nonopt--;              // we've been pushed onto the stack
      i--;                         // to undo the i++ in the loop
      continue;
    }

    if (arg[0] == '-') arg++;      // allow leading '-'
    if (arg[0] == '-') arg++;      // or leading '--'

    // - and -- alone mean what they do for GNU: stop options parsing
    if (*arg == '\0') {
      first_nonopt = i+1;
      break;
    }

    // Find the flag object for this option
    string key;
    const char* value;
    CommandLineFlag* flag = registry_->SplitArgumentLocked(arg, &key, &value);
    if (flag == NULL) {
      undefined_names_[key] = "";    // value isn't actually used
      error_flags_[key] = (string(kError) +
                           "unknown command line flag '" + key + "'\n");
      continue;
    }

    if (value == NULL) {
      // Boolean options are always assigned a value by SplitArgumentLocked()
      assert(strcmp(flag->type_name(), "bool") != 0);
      if (i+1 >= first_nonopt) {
        // This flag needs a value, but there is nothing available
        error_flags_[key] = (string(kError) + "flag '" + (*argv)[i] + "'" +
                            + " is missing its argument\n");
        break;    // we treat this as an unrecoverable error
      } else {
        value = (*argv)[++i];                   // read next arg for value
      }
    }

    // TODO: only set a flag if we hadn't set it before here
    ProcessSingleOptionLocked(flag, value, SET_FLAGS_VALUE);
  }
  registry_->Unlock();

  if (remove_flags) {   // Fix up argc and argv by removing command line flags
    (*argv)[first_nonopt-1] = (*argv)[0];
    (*argv) += (first_nonopt-1);
    (*argc) -= (first_nonopt-1);
    first_nonopt = 1;   // because we still don't count argv[0]
  }

  logging_is_probably_set_up = true;   // because we've parsed --logdir, etc.

  return first_nonopt;
}

string CommandLineFlagParser::ProcessSingleOptionLocked(
    CommandLineFlag* flag, const char* value, FlagSettingMode set_mode) {
  string msg;
  if (value && !registry_->SetFlagLocked(flag, value, set_mode, &msg)) {
    error_flags_[flag->name()] = msg;
    return "";
  }

  return msg;
}

bool CommandLineFlagParser::ReportErrors() {
  // Likewise, if they decided to allow reparsing, all undefined-names
  // are ok; we just silently ignore them now, and hope that a future
  // parse will pick them up somehow.
  if (allow_command_line_reparsing) {
    for (map<string,string>::const_iterator it = undefined_names_.begin();
         it != undefined_names_.end();  ++it)
      error_flags_[it->first] = "";      // clear the error message
  }

  bool found_error = false;
  for (map<string,string>::const_iterator it = error_flags_.begin();
       it != error_flags_.end(); ++it) {
    if (!it->second.empty()) {
      fprintf(stderr, "%s", it->second.c_str());
      found_error = true;
    }
  }
  return found_error;
}

// --------------------------------------------------------------------
// GetCommandLineOption()
// GetCommandLineFlagInfo()
// GetCommandLineFlagInfoOrDie()
// SetCommandLineOption()
// SetCommandLineOptionWithMode()
//    The programmatic way to set a flag's value, using a string
//    for its name rather than the variable itself (that is,
//    SetCommandLineOption("foo", x) rather than FLAGS_foo = x).
//    There's also a bit more flexibility here due to the various
//    set-modes, but typically these are used when you only have
//    that flag's name as a string, perhaps at runtime.
//    All of these work on the default, global registry.
//       For GetCommandLineOption, return false if no such flag
//    is known, true otherwise.  We clear "value" if a suitable
//    flag is found.
// --------------------------------------------------------------------


bool GetCommandLineOption(const char* name, string* value) {
  if (NULL == name)
    return false;
  assert(value);

  FlagRegistry* const registry = FlagRegistry::GlobalRegistry();
  registry->Lock();
  CommandLineFlag* flag = registry->FindFlagLocked(name);
  if (flag == NULL) {
    registry->Unlock();
    return false;
  } else {
    *value = flag->current_value();
    registry->Unlock();
    return true;
  }
}

bool GetCommandLineFlagInfo(const char* name, CommandLineFlagInfo* OUTPUT) {
  if (NULL == name) return false;
  FlagRegistry* const registry = FlagRegistry::GlobalRegistry();
  registry->Lock();
  CommandLineFlag* flag = registry->FindFlagLocked(name);
  if (flag == NULL) {
    registry->Unlock();
    return false;
  } else {
    assert(OUTPUT);
    flag->FillCommandLineFlagInfo(OUTPUT);
    registry->Unlock();
    return true;
  }
}

CommandLineFlagInfo GetCommandLineFlagInfoOrDie(const char* name) {
  CommandLineFlagInfo info;
  if (!GetCommandLineFlagInfo(name, &info)) {
    fprintf(stderr, ": flag %s does not exist\n", name);
    abort();
  }
  return info;
}

string SetCommandLineOptionWithMode(const char* name, const char* value,
                                    FlagSettingMode set_mode) {
  string result;
  FlagRegistry* const registry = FlagRegistry::GlobalRegistry();
  registry->Lock();
  CommandLineFlag* flag = registry->FindFlagLocked(name);
  if (flag) {
    CommandLineFlagParser parser(registry);
    result = parser.ProcessSingleOptionLocked(flag, value, set_mode);
  }
  registry->Unlock();
  // The API of this function is that we return empty string on error
  return result;
}

string SetCommandLineOption(const char* name, const char* value) {
  return SetCommandLineOptionWithMode(name, value, SET_FLAGS_VALUE);
}

// --------------------------------------------------------------------
// BoolFromEnv()
// Int32FromEnv()
// Int64FromEnv()
// Uint64FromEnv()
// DoubleFromEnv()
// StringFromEnv()
//    Reads the value from the environment and returns it.
//    We use an FlagValue to make the parsing easy.
//    Example usage:
//       DEFINE_bool(myflag, BoolFromEnv("MYFLAG_DEFAULT", false), "whatever");
// --------------------------------------------------------------------

template<typename T>
T GetFromEnv(const char *varname, const char* type, T dflt) {
  const char* const valstr = getenv(varname);
  if (!valstr)
    return dflt;
  FlagValue ifv(new T, type);
  if (!ifv.ParseFrom(valstr)) {
    fprintf(stderr, "ERROR: error parsing env variable '%s' with value '%s'\n",
            varname, valstr);
    exit(1);
  }
  return OTHER_VALUE_AS(ifv, T);
}

bool BoolFromEnv(const char *v, bool dflt) {
  return GetFromEnv(v, "bool", dflt);
}
int32 Int32FromEnv(const char *v, int32 dflt) {
  return GetFromEnv(v, "int32", dflt);
}
int64 Int64FromEnv(const char *v, int64 dflt)    {
  return GetFromEnv(v, "int64", dflt);
}
uint64 Uint64FromEnv(const char *v, uint64 dflt) {
  return GetFromEnv(v, "uint64", dflt);
}
double DoubleFromEnv(const char *v, double dflt) {
  return GetFromEnv(v, "double", dflt);
}
const char *StringFromEnv(const char *varname, const char *dflt) {
  const char* const val = getenv(varname);
  return val ? val : dflt;
}


// --------------------------------------------------------------------
// ParseCommandLineFlags()
// ParseCommandLineNonHelpFlags()
// HandleCommandLineHelpFlags()
//    This is the main function called from main(), to actually
//    parse the commandline.  It modifies argc and argv as described
//    at the top of commandlineflags.h.  You can also divide this
//    function into two parts, if you want to do work between
//    the parsing of the flags and the printing of any help output.
// --------------------------------------------------------------------

static uint32 ParseCommandLineFlagsInternal(int* argc, char*** argv,
                                            bool remove_flags, bool do_report) {
  SetArgv(*argc, const_cast<const char**>(*argv));    // save it for later

  FlagRegistry* const registry = FlagRegistry::GlobalRegistry();
  CommandLineFlagParser parser(registry);

  // Now get the flags specified on the commandline
  const int r = parser.ParseNewCommandLineFlags(argc, argv, remove_flags);

  if (do_report)
    HandleCommandLineHelpFlags();   // may cause us to exit on --help, etc.
  if (parser.ReportErrors())        // may cause us to exit on illegal flags
    exit(1);
  return r;
}

uint32 ParseCommandLineFlags(int* argc, char*** argv, bool remove_flags) {
  return ParseCommandLineFlagsInternal(argc, argv, remove_flags, true);
}

#ifdef OS_WINDOWS

// Added by liuli
// Do the same as CommandLineToArgW.
static char** CommandLineToArgvA(char * cmd_line, int* argc) {
  string command_line(cmd_line);
  // Those characters are used to seperate different options.
  string seperator(" \r\n\t");
  string::size_type current_pos = 0;
  string::size_type last_pos = 0;
  vector<string> commands;

  while ((current_pos = command_line.find_first_of(seperator, last_pos))
      != std::string::npos) {
    if (current_pos != last_pos) {
      commands.push_back(string(command_line.substr(last_pos,
        current_pos - last_pos)));
    }
    last_pos = current_pos + 1;
  }

  // Store the last part of the command.
  if (last_pos <= command_line.size())
    commands.push_back(string(command_line.substr(last_pos)));

  *argc = static_cast<int>(commands.size());
  char *buffer = NULL;
  char **argv = NULL;
  if (!commands.empty()) {
    size_t cmd_len = strlen(cmd_line);
    int ptr_len = *argc * sizeof(char *);
    // We allocate a whole big chunk of memory to store all the information,
    // which are argv pointers, and the actually values. Hence the total buffer
    // length we should reserve is:
    // sizeof(pointer array) + sizeof(commands).
    // Note: each command is ended with a '0'.
    size_t total_len = cmd_len + ptr_len + sizeof('\0') * commands.size();
    buffer = static_cast<char*>(GlobalAlloc(GMEM_FIXED, total_len));
    memset(buffer, 0, total_len);

    argv = reinterpret_cast<char **>(buffer);
    // Fill in the commands and pointers.
    // Pointers are put from the begining of the memory.
    char *const start_of_command = buffer + ptr_len;
    char *const end_of_command = buffer + total_len;
    char *cur_command_start = start_of_command;

    char *const start_of_ptr = buffer;
    char *const end_of_ptr = start_of_command;
    char *cur_ptr_start = buffer;

    vector<string>::iterator iter;
    for (iter = commands.begin(); iter != commands.end(); ++iter) {
      string cmd = *iter;
      assert(cmd.size() + cur_command_start < end_of_command);
      assert(sizeof(char *) + cur_ptr_start <= end_of_ptr);

      strncpy(cur_command_start, cmd.c_str(), cmd.size() + 1 /*Ending 0 should
                                                               be copied*/);
      *reinterpret_cast<char **>(cur_ptr_start) = cur_command_start;
      cur_ptr_start += sizeof(char *);
      cur_command_start += cmd.size() + 1;
    }
  }

  return argv;
}

static std::string WideToUTF8(const wchar_t *wide) {
  string result;
  int charcount = WideCharToMultiByte(CP_UTF8, 0, wide, -1,
                                      NULL, 0, NULL, NULL);
  if (charcount <= 0)
    return "";
  char *buf = new char[charcount];
  WideCharToMultiByte(CP_UTF8, 0, wide, -1, buf, charcount,
      NULL, NULL);
  result = string(buf);
  delete []buf;
  return result;
}

uint32 ParseCommandLineFlags(wchar_t *command_line) {
  if (!command_line)
    return 0;
  int argc = 0;
  std::string utf8_command_line = WideToUTF8(command_line);
  return ParseCommandLineFlags(
    const_cast<char *>(utf8_command_line.c_str()));
}

uint32 ParseCommandLineFlags(char *command_line) {
  if (!command_line)
    return 0;
  int argc = 0;
  char** argv = CommandLineToArgvA(command_line, &argc);
  ParseCommandLineFlags(&argc, &argv, false);
  LocalFree(argv);
  return argc;
}
#endif

uint32 ParseCommandLineNonHelpFlags(int* argc, char*** argv,
                                    bool remove_flags) {
  return ParseCommandLineFlagsInternal(argc, argv, remove_flags, false);
}

// --------------------------------------------------------------------
// AllowCommandLineReparsing()
// ReparseCommandLineNonHelpFlags()
//    This is most useful for shared libraries.  The idea is if
//    a flag is defined in a shared library that is dlopen'ed
//    sometime after main(), you can ParseCommandLineFlags before
//    the dlopen, then ReparseCommandLineNonHelpFlags() after the
//    dlopen, to get the new flags.  But you have to explicitly
//    Allow() it; otherwise, you get the normal default behavior
//    of unrecognized flags calling a fatal error.
//    TODO: this isn't used.  Just delete it?
// --------------------------------------------------------------------

void AllowCommandLineReparsing() {
  allow_command_line_reparsing = true;
}

uint32 ReparseCommandLineNonHelpFlags() {
  // We make a copy of argc and argv to pass in
  const vector<string>& local_argvs = GetArgvs();
  int tmp_argc = static_cast<int>(local_argvs.size());
  char** tmp_argv = new char* [tmp_argc + 1];
  for (int i = 0; i < tmp_argc; ++i)
    tmp_argv[i] = strdup(local_argvs[i].c_str());  // TODO: don't dup

  const int retval = ParseCommandLineNonHelpFlags(&tmp_argc, &tmp_argv, false);

  for (int i = 0; i < tmp_argc; ++i)
    free(tmp_argv[i]);
  delete[] tmp_argv;

  return retval;
}
