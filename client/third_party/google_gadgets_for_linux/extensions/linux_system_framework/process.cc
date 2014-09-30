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

#include <sys/types.h>
#include <dirent.h>
#include <cstdlib>

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#include "process.h"
#include "machine.h"

namespace ggadget {
namespace framework {
namespace linux_system {

static const char* kDirName = "/proc";

static bool ReadCmdPath(int pid, std::string *cmdline);

// ---------------------------PrcoessInfo Class-------------------------------//

ProcessInfo::ProcessInfo(int pid, const std::string &path) :
  pid_(pid), path_(path) {
}

void ProcessInfo::Destroy() {
  delete this;
}

int ProcessInfo::GetProcessId() const {
  return pid_;
}

std::string ProcessInfo::GetExecutablePath() const {
  return path_;
}

// ---------------------------Processes Class---------------------------------//

void Processes::Destroy() {
  delete this;
}

Processes::Processes() {
  InitProcesses();
}

int Processes::GetCount() const {
  return static_cast<int>(procs_.size());
}

ProcessInfoInterface *Processes::GetItem(int index) {
  if (index < 0|| index >= GetCount())
    return NULL;
  return new ProcessInfo(procs_[index].first, procs_[index].second);
}

void Processes::InitProcesses() {
  DIR *dir = NULL;
  struct dirent *entry = NULL;

  dir = opendir(kDirName);
  if (!dir) // if can't open the directory, just return
    return;

  while ((entry = readdir(dir)) != NULL) {
    char *name = entry->d_name;
    char *end;
    int pid = static_cast<int>(strtol(name, &end, 10));

    // if it is not a process folder, so skip it
    if (!pid || *end)
      continue;

    std::string cmdline;
    if (ReadCmdPath(pid, &cmdline) && cmdline != "") {
      procs_.push_back(IntStringPair(pid, cmdline));
    }
  }
}

// -----------------------------Process Class---------------------------------//

ProcessesInterface *Process::EnumerateProcesses() {
  return new Processes;
}

#ifdef HAVE_X11

static int IgnoreXError(Display *display, XErrorEvent *event) {
  GGL_UNUSED(display);
  DLOG("XError: %lu %d %d %d\n", event->serial, event->error_code,
       event->request_code, event->minor_code);
  return 0;
}

static int GetWindowPID(Display *display, Window window, Atom atom) {
  unsigned char *data = NULL;
  Atom type;
  int format;
  unsigned long count, after;
  XGetWindowProperty(display, window, atom, 0, 1, False, XA_CARDINAL,
                     &type, &format, &count, &after, &data);
  int pid = -1;
  if (data) {
    if (format == 32 && count == 1 && after == 0)
      pid = *reinterpret_cast<int32_t *>(data);
    XFree(data);
  }
  return pid;
}

ProcessInfoInterface *Process::GetForeground() {
  int pid = -1;
  int (*old_error_handler)(Display *, XErrorEvent *) =
      XSetErrorHandler(IgnoreXError);
  Display *display = XOpenDisplay(NULL);
  if (display) {
    // See http://standards.freedesktop.org/wm-spec/wm-spec-1.3.html#id2507760.
    Atom pid_atom = XInternAtom(display, "_NET_WM_PID", True);
    if (pid_atom != None) {
      Window focused;
      int revert_to;
      XGetInputFocus(display, &focused, &revert_to);
      if (focused != None) {
        // Walk up the window tree to find a window with _WM_NET_PID property.
        Window pid_window = focused;
        Window parent, root;
        Window *children = NULL;
        unsigned int nchildren;
        while (true) {
          pid = GetWindowPID(display, pid_window, pid_atom);
          if (pid != -1)
            break;
          if (!XQueryTree(display, pid_window, &root, &parent, &children,
                          &nchildren))
            break;
          if (children)
            XFree(children);
          if (parent == None || parent == root)
            break;
          pid_window = parent;
        }
        // Can't find a correct focused window in parents. try children.
        if (pid == -1 &&
            XQueryTree(display, focused, &root, &parent, &children,
                       &nchildren) &&
            children) {
          for (unsigned int i = 0; i < nchildren && pid == -1; ++i)
            pid = GetWindowPID(display, children[i], pid_atom);
          XFree(children);
        }
      }
    }
    XCloseDisplay(display);
  }

  XSetErrorHandler(old_error_handler);
  return pid == -1 ? NULL : GetInfo(pid);
}

#else // ifdef HAVE_X11

ProcessInfoInterface *Process::GetForeground() {
  return NULL;
}

#endif // HAVE_X11

ProcessInfoInterface *Process::GetInfo(int pid) {
  std::string cmdline;
  if (ReadCmdPath(pid, &cmdline)) {
    return new ProcessInfo(pid, cmdline);
  }
  return NULL;
}


// ---------------------------Utility Functions-------------------------------//

// Reads the command path with pid from proc system.
// EMPTY string will be returned if any error.
static bool ReadCmdPath(int pid, std::string *cmdline) {
  if (pid <= 0 || !cmdline)
    return false;

  char filename[PATH_MAX + 2] = {0};
  snprintf(filename, sizeof(filename) - 1, "%s/%d/exe", kDirName, pid);

  char command[PATH_MAX + 2] = {0};
  if (readlink(filename, command, sizeof(command) - 1) < 0) {
    *cmdline = "";
    return false;
  }

  for (int i = 0; command[i]; i++) {
    if (command[i] == ' ' || command[i] == '\n') {
      command[i] = 0;
      break;
    }
  }
  *cmdline = std::string(command);

  return true;
}

} // namespace linux_system
} // namespace framework
} // namespace ggadget
