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
// This file contains code for handling the 'reporting' flags.  These
// are flags that, when present, cause the program to report some
// information and then exit.  --help and --version are the canonical
// reporting flags, but we also have flags like --helpxml, etc.
//
// There's only one function that's meant to be called externally:
// HandleCommandLineHelpFlags().  (Well, actually,
// ShowUsageWithFlags() and ShowUsageWithFlagsRestrict() can be called
// externally too, but there's little need for it.)  These are all
// declared in the main commandlineflags.h header file.
//
// HandleCommandLineHelpFlags() will check what 'reporting' flags have
// been defined, if any -- the "help" part of the function name is a
// bit misleading -- and do the relevant reporting.  It should be
// called after all flag-values have been assigned, that is, after
// parsing the command-line.

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "base/commandlineflags.h"
#include "base/port.h"

// The 'reporting' flags.  They all call exit().
DEFINE_bool(help, false,
            "show help on all flags [tip: all flags can have two dashes]");
DEFINE_bool(helpxml, false,
            "produce an xml version of help");
DEFINE_bool(build_version, false,
            "show version and build info and exit");

// --------------------------------------------------------------------
// DescribeOneFlag()
// DescribeOneFlagInXML()
//    Routines that pretty-print info about a flag.  These use
//    a CommandLineFlagInfo, which is the way the commandlineflags
//    API exposes static info about a flag.
// --------------------------------------------------------------------

static const int kLineLength = 80;

static void AddString(const string& s,
                      string* final_string, int* chars_in_line) {
  const int slen = static_cast<int>(s.length());
  if (*chars_in_line + 1 + slen >= kLineLength) {  // < 80 chars/line
    *final_string += "\n      ";
    *chars_in_line = 6;
  } else {
    *final_string += " ";
    *chars_in_line += 1;
  }
  *final_string += s;
  *chars_in_line += slen;
}

// Create a descriptive string for a flag.
// Goes to some trouble to make pretty line breaks.
static string DescribeOneFlag(const CommandLineFlagInfo& flag) {
  string main_part = (string("    -") + flag.name +
                      " (" + flag.description + ')');
  const char* c_string = main_part.c_str();
  int chars_left = static_cast<int>(main_part.length());
  string final_string = "";
  int chars_in_line = 0;  // how many chars in current line so far?
  while (1) {
    assert(chars_left == strlen(c_string));  // Unless there's a \0 in there?
    const char* newline = strchr(c_string, '\n');
    if (newline == NULL && chars_in_line+chars_left < kLineLength) {
      // The whole remainder of the string fits on this line
      final_string += c_string;
      chars_in_line += chars_left;
      break;
    }
    if (newline != NULL && newline - c_string < kLineLength - chars_in_line) {
      int n = newline - c_string;
      final_string.append(c_string, n);
      chars_left -= n + 1;
      c_string += n + 1;
    } else {
      // Find the last whitespace on this 80-char line
      int whitespace = kLineLength-chars_in_line-1;  // < 80 chars/line
      while ( whitespace > 0 && !isspace(c_string[whitespace]) ) {
        --whitespace;
      }
      if (whitespace <= 0) {
        // Couldn't find any whitespace to make a line break.  Just dump the
        // rest out!
        final_string += c_string;
        chars_in_line = kLineLength;  // next part gets its own line for sure!
        break;
      }
      final_string += string(c_string, whitespace);
      chars_in_line += whitespace;
      while (isspace(c_string[whitespace]))  ++whitespace;
      c_string += whitespace;
      chars_left -= whitespace;
    }
    if (*c_string == '\0')
      break;
    final_string += "\n      ";
    chars_in_line = 6;
  }

  // Append data type
  AddString(string("type: ") + flag.type, &final_string, &chars_in_line);
  // Append default value
  if (strcmp(flag.type.c_str(), "string") == 0) {  // add quotes for strings
    AddString(string("default: \"") + flag.default_value + string("\""),
              &final_string, &chars_in_line);
  } else {
    AddString(string("default: ") + flag.default_value,
              &final_string, &chars_in_line);
  }

  final_string += '\n';
  return final_string;
}

// Simple routine to xml-escape a string: escape & and < only.
static string XMLText(const string& txt) {
  string ans = txt;
  for (string::size_type pos = 0; (pos=ans.find("&", pos)) != string::npos; )
    ans.replace(pos++, 1, "&amp;");
  for (string::size_type pos = 0; (pos=ans.find("<", pos)) != string::npos; )
    ans.replace(pos++, 1, "&lt;");
  return ans;
}

static string DescribeOneFlagInXML(const CommandLineFlagInfo& flag) {
  // The file and flagname could have been attributes, but default
  // and meaning need to avoid attribute normalization.  This way it
  // can be parsed by simple programs, in addition to xml parsers.
  return (string("<flag>") +
          "<file>" + XMLText(flag.filename) + "</file>" +
          "<name>" + XMLText(flag.name) + "</name>" +
          "<meaning>" + XMLText(flag.description) + "</meaning>" +
          "<default>" + XMLText(flag.default_value) + "</default>" +
          "<type>" + XMLText(flag.type) + "</type>" +
          string("</flag>"));
}

// --------------------------------------------------------------------
// ShowUsageWithFlags()
// ShowUsageWithFlagsRestrict()
// ShowXMLOfFlags()
//    These routines variously expose the registry's list of flag
//    values.  ShowUsage*() prints the flag-value information
//    to stdout in a user-readable format (that's what --help uses).
//    The Restrict() version limits what flags are shown.
//    ShowXMLOfFlags() prints the flag-value information to stdout
//    in a machine-readable format.  In all cases, the flags are
//    sorted: first by filename they are defined in, then by flagname.
// --------------------------------------------------------------------

static const char* Basename(const char* filename) {
  const char* sep = strrchr(filename, PATH_SEPARATOR);
  return sep ? sep + 1 : filename;
}

static string Dirname(const string& filename) {
  string::size_type sep = filename.rfind(PATH_SEPARATOR);
  return filename.substr(0, (sep == string::npos) ? 0 : sep);
}

void ShowUsageWithFlagsRestrict(const char *argv0, const char *restrict) {
#ifndef DO_NOT_SHOW_COMMANDLINE_HELP
  fprintf(stdout, "%s: %s\n", Basename(argv0), ProgramUsage());

  vector<CommandLineFlagInfo> flags;
  GetAllFlags(&flags);           // flags are sorted by filename, then flagname

  const bool have_restrict = (restrict != NULL) && (*restrict != '\0');
  string last_filename = "";     // so we know when we're at a new file
  bool first_directory = true;   // controls blank lines between dirs
  bool found_match = false;      // stays false iff no dir matches restrict
  for (vector<CommandLineFlagInfo>::const_iterator flag = flags.begin();
       flag != flags.end();
       ++flag) {
    if (have_restrict && strstr(flag->filename.c_str(), restrict) == NULL) {
      continue;             // this flag doesn't pass the restrict
    }
    found_match = true;     // this flag passed the restrict!
    if (flag->filename != last_filename) {                      // new file
      if (Dirname(flag->filename) != Dirname(last_filename)) {  // new dir!
        if (!first_directory)
          fprintf(stdout, "\n\n");   // put blank lines between directories
        first_directory = false;
      }
      fprintf(stdout, "\n  Flags from %s:\n", flag->filename.c_str());
      last_filename = flag->filename;
    }
    // Now print this flag
    fprintf(stdout, "%s", DescribeOneFlag(*flag).c_str());
  }
  if (!found_match && restrict == NULL) {
    fprintf(stdout, "\n  No modules matched program name `%s': use -help\n",
            Basename(argv0));
  }
#endif  // DO_NOT_SHOW_COMMANDLINE_HELP
}

void ShowUsageWithFlags(const char *argv0) {
  ShowUsageWithFlagsRestrict(argv0, "");
}

// Convert the help, program, and usage to xml.
static void ShowXMLOfFlags(const char *prog_name) {
  vector<CommandLineFlagInfo> flags;
  GetAllFlags(&flags);   // flags are sorted: by filename, then flagname

  // XML.  There is no corresponding schema yet
  fprintf(stdout, "<?xml version=\"1.0\"?>\n");
  // The document
  fprintf(stdout, "<AllFlags>\n");
  // the program name and usage
  fprintf(stdout, "<program>%s</program>\n",
          XMLText(Basename(prog_name)).c_str());
  fprintf(stdout, "<usage>%s</usage>\n",
          XMLText(ProgramUsage()).c_str());
  // All the flags
  for (vector<CommandLineFlagInfo>::const_iterator flag = flags.begin();
       flag != flags.end();
       ++flag) {
    fprintf(stdout, "%s\n", DescribeOneFlagInXML(*flag).c_str());
  }
  // The end of the document
  fprintf(stdout, "</AllFlags>\n");
}

static void ShowVersion() {
  fprintf(stdout, "%s, build at %s %s\n", ProgramInvocationShortName(),
      __DATE__, __TIME__);
}

// --------------------------------------------------------------------
// HandleCommandLineHelpFlags()
//    Checks all the 'reporting' commandline flags to see if any
//    have been set.  If so, handles them appropriately.  Note
//    that all of them, by definition, cause the program to exit
//    if they trigger.
// --------------------------------------------------------------------

void HandleCommandLineHelpFlags() {
  const char* progname = ProgramInvocationShortName();

  if (FLAGS_help) {
    // show all options
    ShowUsageWithFlagsRestrict(progname, "");   // empty restrict
    exit(1);
  } else if (FLAGS_helpxml) {
    ShowXMLOfFlags(progname);
    exit(1);
  } else if (FLAGS_build_version) {
    ShowVersion();
    // Unlike help, we may be asking for version in a script, so return 0
    exit(0);

  }
}
