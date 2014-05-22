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

#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <cstdlib>
#include <locale.h>
#include <signal.h>
#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QMenu>
#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtGui/QDesktopWidget>

#include <ggadget/gadget.h>
#include <ggadget/gadget_consts.h>
#include <ggadget/gadget_manager_interface.h>
#include <ggadget/host_interface.h>
#include <ggadget/host_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/string_utils.h>
#include <ggadget/messages.h>
#include <ggadget/qt/qt_view_widget.h>
#include <ggadget/qt/qt_view_host.h>
#include <ggadget/qt/qt_menu.h>
#include <ggadget/qt/qt_main_loop.h>
#include <ggadget/qt/utilities.h>
#include <ggadget/run_once.h>
#include <ggadget/script_runtime_interface.h>
#include <ggadget/xml_http_request_interface.h>

#include "qt_host.h"
#if defined(Q_WS_X11) && defined(HAVE_X11)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#endif

#ifndef GGL_QT_SCRIPT_ENGINE
#define GGL_QT_SCRIPT_ENGINE "smjs"
#endif

using ggadget::Variant;

static ggadget::qt::QtMainLoop *g_main_loop;
static const char kRunOnceSocketName[] = "ggl-host-socket";

static const char *kGlobalExtensions[] = {
  "smjs-script-runtime",
  "default-framework",
  "libxml2-xml-parser",
  "default-options",
  "dbus-script-class",
  "qtwebkit-browser-element",
  "qt-system-framework",
  "qt-edit-element",
  "gst-audio-framework",
  "gst-video-element",
#ifdef GGL_HOST_LINUX
  "linux-system-framework",
#endif
  "qt-xml-http-request",
  "analytics-usage-collector",
  "google-gadget-manager",
  NULL
};

static const char kHelpString[] =
  "Google Gadgets for Linux " GGL_VERSION
  " (Gadget API version " GGL_API_VERSION ")\n"
  "Usage: " GGL_APP_NAME " [Options] [Gadgets]\n"
  "Options:\n"
#ifdef _DEBUG
  "  -d mode, --debug mode\n"
  "      Specify debug modes for drawing View:\n"
  "      0 - No debug.\n"
  "      1 - Draw bounding boxes around container elements.\n"
  "      2 - Draw bounding boxes around all elements.\n"
  "      4 - Draw bounding boxes around clip region.\n"
#endif
#if QT_VERSION >= 0x040400
  "  -s script_runtime, --script-runtime script_runtime\n"
  "      Specify which script runtime to use, default: " GGL_QT_SCRIPT_ENGINE "\n"
  "      smjs - spidermonkey js runtime\n"
  "      qt   - QtScript js runtime(experimental)\n"
#endif
  "  -bg Run in background\n"
  "  -l loglevel, --log-level loglevel\n"
  "      Specify the minimum gadget.debug log level.\n"
  "      0 - Trace(All)  1 - Info  2 - Warning  3 - Error  >=4 - No log\n"
  "  -ll, --long-log\n"
  "      Output logs using long format.\n"
  "  -dc, --debug-console debug_console_config\n"
  "      Change debug console configuration:\n"
  "      0 - No debug console allowed\n"
  "      1 - Gadgets has debug console menu item\n"
  "      2 - Open debug console when gadget is added to debug startup code\n"
  "  -nc, --no-collector\n"
  "      Disable the usage collector\n"
  "  -h, --help\n"
  "      Print this message and exit.\n"
  "\n"
  "Gadgets:\n"
  "  Can specify one or more Desktop Gadget paths.\n"
  "  If any gadgets are specified, they will be installed by using\n"
  "  GadgetManager.\n";

enum ArgumentID {
  ARG_DEBUG = 1,
  ARG_SCRIPT_RUNTIME,
  ARG_BACKGROUND,
  ARG_LOG_LEVEL,
  ARG_LONG_LOG,
  ARG_DEBUG_CONSOLE,
  ARG_NO_COLLECTOR,
  ARG_HELP
};

static const ggadget::HostArgumentInfo kArgumentsInfo[] = {
#ifdef _DEBUG
  { ARG_DEBUG,          Variant::TYPE_INT64,  "-d",  "--debug" },
#endif
#if QT_VERSION >= 0x040400
  { ARG_SCRIPT_RUNTIME, Variant::TYPE_STRING, "-s", "--script-runtime" },
#endif
  { ARG_BACKGROUND,     Variant::TYPE_BOOL,   "-bg", "--background" },
  { ARG_LOG_LEVEL,      Variant::TYPE_INT64,  "-l",  "--log-level" },
  { ARG_LONG_LOG,       Variant::TYPE_BOOL,   "-ll", "--long-log" },
  { ARG_DEBUG_CONSOLE,  Variant::TYPE_INT64,  "-dc", "--debug-console" },
  { ARG_NO_COLLECTOR,   Variant::TYPE_BOOL,   "-nc", "--no-collector" },
  { ARG_HELP,           Variant::TYPE_BOOL,   "-h",  "--help" },
  { -1,                 Variant::TYPE_VOID, NULL, NULL } // End of list
};

struct Arguments {
  Arguments()
    : debug_mode(0),
      background(false),
#ifdef _DEBUG
      log_level(ggadget::LOG_TRACE),
      long_log(true),
#else
      log_level(ggadget::LOG_WARNING),
      long_log(false),
#endif
      debug_console(ggadget::Gadget::DEBUG_CONSOLE_DISABLED),
      no_collector(false) {
  }

  int debug_mode;
  std::string script_runtime;
  bool background;
  int log_level;
  bool long_log;
  ggadget::Gadget::DebugConsoleConfig debug_console;
  bool no_collector;
};

static ggadget::HostArgumentParser g_argument_parser(kArgumentsInfo);
static Arguments g_arguments;

static void ExtractArgumentsValue() {
  // Resets arguments value.
  g_arguments = Arguments();

  ggadget::Variant arg_value;
  if (g_argument_parser.GetArgumentValue(ARG_DEBUG, &arg_value))
    g_arguments.debug_mode = ggadget::VariantValue<int>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_SCRIPT_RUNTIME, &arg_value))
    g_arguments.script_runtime =
        ggadget::VariantValue<std::string>()(arg_value);
  if (g_arguments.script_runtime == "qt" ||
      g_arguments.script_runtime == "qt-script-runtime") {
    kGlobalExtensions[0] = "qt-script-runtime";
  } else {
    kGlobalExtensions[0] = GGL_QT_SCRIPT_ENGINE "-script-runtime";
  }

  if (g_argument_parser.GetArgumentValue(ARG_BACKGROUND, &arg_value))
    g_arguments.background = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_LOG_LEVEL, &arg_value))
    g_arguments.log_level = ggadget::VariantValue<int>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_LONG_LOG, &arg_value))
    g_arguments.long_log = ggadget::VariantValue<bool>()(arg_value);
  if (g_argument_parser.GetArgumentValue(ARG_DEBUG_CONSOLE, &arg_value))
    g_arguments.debug_console =
        static_cast<ggadget::Gadget::DebugConsoleConfig>(
            ggadget::VariantValue<int>()(arg_value));
  if (g_argument_parser.GetArgumentValue(ARG_NO_COLLECTOR, &arg_value))
    g_arguments.no_collector = ggadget::VariantValue<bool>()(arg_value);
}

#if defined(Q_WS_X11) && defined(HAVE_X11)
static Display *dpy;
static Colormap colormap = 0;
static Visual *visual = 0;
static bool InitArgb() {
  dpy = XOpenDisplay(0); // open default display
  if (!dpy) {
    qWarning("Cannot connect to the X server");
    exit(1);
  }

  int screen = DefaultScreen(dpy);
  int eventBase, errorBase;

  if (XRenderQueryExtension(dpy, &eventBase, &errorBase)) {
    int nvi;
    XVisualInfo templ;
    templ.screen  = screen;
    templ.depth   = 32;
    templ.c_class = TrueColor;
    XVisualInfo *xvi = XGetVisualInfo(dpy, VisualScreenMask |
                                      VisualDepthMask |
                                      VisualClassMask, &templ, &nvi);

    for (int i = 0; i < nvi; ++i) {
      XRenderPictFormat *format = XRenderFindVisualFormat(dpy,
                                                          xvi[i].visual);
      if (format->type == PictTypeDirect && format->direct.alphaMask) {
        visual = xvi[i].visual;
        colormap = XCreateColormap(dpy, RootWindow(dpy, screen),
                                   visual, AllocNone);
        return true;
      }
    }
  }
  return false;
}
static bool CheckCompositingManager(Display *dpy) {
  Atom net_wm_state = XInternAtom(dpy, "_NET_WM_CM_S0", False);
  return XGetSelectionOwner(dpy, net_wm_state);
}
#endif

static bool LoadLocalGadget(const std::string &gadget) {
  std::string path = ggadget::GetAbsolutePath(gadget.c_str());
  ggadget::GetGadgetManager()->NewGadgetInstanceFromFile(path.c_str());
  return true;
}

static void OnClientMessage(const std::string &data) {
  if (data == ggadget::HostArgumentParser::kStartSignature) {
    g_argument_parser.Start();
  } else if (data == ggadget::HostArgumentParser::kFinishSignature) {
    if (g_argument_parser.Finish()) {
      ExtractArgumentsValue();
      g_argument_parser.EnumerateRemainedArgs(
          ggadget::NewSlot(LoadLocalGadget));
    }
  } else if (data.length()) {
    g_argument_parser.AppendArgument(data.c_str());
  }
}

static void DefaultSignalHandler(int sig) {
  DLOG("Signal caught: %d, exit.", sig);
  g_main_loop->Quit();
}

static bool SendArgumentCallback(const std::string &arg,
                                 ggadget::RunOnce *run_once) {
  run_once->SendMessage(arg);
  return true;
}

static bool SendPathCallback(const std::string &path,
                             ggadget::RunOnce *run_once) {
  std::string abs_path = ggadget::GetAbsolutePath(path.c_str());
  if (abs_path.length())
    run_once->SendMessage(abs_path);
  return true;
}


int main(int argc, char* argv[]) {
  // set locale according to env vars
  setlocale(LC_ALL, "");

  // Parse command line.
  if (argc > 1) {
    g_argument_parser.Start();
    if (!g_argument_parser.AppendArguments(argc - 1, argv + 1) ||
        !g_argument_parser.Finish()) {
      printf("Invalid arguments.\n%s", kHelpString);
      return 1;
    }
  }

  // Check --help argument first.
  if (g_argument_parser.GetArgumentValue(ARG_HELP, NULL)) {
    printf("%s", kHelpString);
    return 0;
  }

  ExtractArgumentsValue();

  // Parse command line before create QApplication because it will eat some
  // argv like -bg
  bool composite = false;
#if defined(Q_WS_X11) && defined(HAVE_X11)
  if (InitArgb() && CheckCompositingManager(dpy)) {
    composite = true;
  } else {
    visual = NULL;
    if (colormap) {
      XFreeColormap(dpy, colormap);
      colormap = 0;
    }
  }
  QApplication app(dpy, argc, argv,
                   Qt::HANDLE(visual), Qt::HANDLE(colormap));
#else
  QApplication app(argc, argv);
#endif

  std::string profile_dir = ggadget::BuildFilePath(
      ggadget::GetHomeDirectory().c_str(),
      ggadget::kDefaultProfileDirectory,
      NULL);

  QString error;
  ggadget::qt::GGLInitFlags flags = ggadget::qt::GGL_INIT_FLAG_NONE;
  if (g_arguments.long_log)
    flags |= ggadget::qt::GGL_INIT_FLAG_LONG_LOG;
  if (!g_arguments.no_collector)
    flags |= ggadget::qt::GGL_INIT_FLAG_COLLECTOR;

  g_main_loop = new ggadget::qt::QtMainLoop();
  if (!ggadget::qt::InitGGL(g_main_loop, GGL_APP_NAME,
                            profile_dir.c_str(),
                            kGlobalExtensions,
                            g_arguments.log_level,
                            flags,
                            &error)) {
    QMessageBox::information(NULL, "Google Gadgets", error);
    return 1;
  }

  ggadget::RunOnce run_once(
      ggadget::BuildFilePath(profile_dir.c_str(),
                             kRunOnceSocketName,
                             NULL).c_str());
  run_once.ConnectOnMessage(ggadget::NewSlot(OnClientMessage));

  if (run_once.IsRunning()) {
    DLOG("Another instance already exists.");
    run_once.SendMessage(ggadget::HostArgumentParser::kStartSignature);
    g_argument_parser.EnumerateRecognizedArgs(
        NewSlot(SendArgumentCallback, &run_once));
    g_argument_parser.EnumerateRemainedArgs(
        NewSlot(SendPathCallback, &run_once));
    run_once.SendMessage(ggadget::HostArgumentParser::kFinishSignature);
    return 0;
  }

  if (g_arguments.background)
    ggadget::Daemonize();

  hosts::qt::QtHost host(composite, g_arguments.debug_mode,
                         g_arguments.debug_console);

  // Load gadget files.
  g_argument_parser.EnumerateRemainedArgs(ggadget::NewSlot(LoadLocalGadget));

  // Hook popular signals to exit gracefully.
  signal(SIGHUP, DefaultSignalHandler);
  signal(SIGINT, DefaultSignalHandler);
  signal(SIGTERM, DefaultSignalHandler);
  signal(SIGUSR1, DefaultSignalHandler);
  signal(SIGUSR2, DefaultSignalHandler);

  g_main_loop->Run();

  return 0;
}
