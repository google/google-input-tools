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

#ifndef GGADGET_TESTS_MAIN_LOOP_TEST_H__
#define GGADGET_TESTS_MAIN_LOOP_TEST_H__

#if !defined(OS_WIN)
#include <unistd.h>
#endif  // OS_WIN

#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>

#include "ggadget/common.h"
#include "ggadget/logger.h"
#include "unittest/gtest.h"
#include "ggadget/main_loop_interface.h"

using namespace ggadget;

// Callback class for a timeout watch.
// This callback may quit the main loop by calling MainLoopInterface::Quit() if
// the callback times reaches the specified quit times.
// It's for testing the ability of quitting main loop inside a callback.
class TimeoutWatchCallback : public WatchCallbackInterface {
 public:
  TimeoutWatchCallback(int interval, int *times, int quit_times)
    : interval_(interval),
      times_(times),
      quit_times_(quit_times) {
  }

  virtual ~TimeoutWatchCallback() {
  }

  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    EXPECT_TRUE(main_loop != NULL);
    if (main_loop) {
      EXPECT_EQ(MainLoopInterface::TIMEOUT_WATCH,
                main_loop->GetWatchType(watch_id));
      EXPECT_EQ(interval_, main_loop->GetWatchData(watch_id));
      ++(*times_);
      if (*times_ == quit_times_) {
        main_loop->Quit();
      }
    }
    return true;
  }

  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    EXPECT_TRUE(main_loop != NULL);
    if (main_loop) {
      EXPECT_EQ(MainLoopInterface::TIMEOUT_WATCH,
                main_loop->GetWatchType(watch_id));
      EXPECT_EQ(interval_, main_loop->GetWatchData(watch_id));
    }
    delete this;
  }

 private:
  int interval_;
  int *times_;
  int quit_times_;
};

const int kTimePiece = 100;

#if !defined(OS_WIN)
// Callback class for an IO read watch.
// Inside this callback, a timeout watch may be added/removed according to the
// content of input string. It's for testing the ability of adding/removing
// watches inside a callback.
class IOReadWatchCallback : public WatchCallbackInterface {
 public:
  IOReadWatchCallback(int fd, std::vector<std::string> *strings, int *times)
    : timeout_watch_id_(-1),
      fd_(fd),
      strings_(strings),
      times_(times) {
  }

  virtual ~IOReadWatchCallback() {
    // Do nothing.
  }

  virtual bool Call(MainLoopInterface *main_loop, int watch_id) {
    EXPECT_TRUE(main_loop != NULL);
    if (main_loop) {
      EXPECT_EQ(MainLoopInterface::IO_READ_WATCH,
                main_loop->GetWatchType(watch_id));
      EXPECT_EQ(fd_, main_loop->GetWatchData(watch_id));
    }

    char buf[256];
    ssize_t ret = read(fd_, buf, 256);
    EXPECT_GT(ret, 0);
    if (ret > 0) {
      buf[ret] = 0;
      strings_->push_back(buf);
      DLOG("Received: %s", buf);
    }

    // If the string is "quit" then remove the watch.
    if (strcmp(buf, "quit") == 0) {
      return false;
    } else if (strncmp(buf, "add ", 4) == 0) {
      double interval_scale = strtod(buf + 4, NULL);
      int interval = static_cast<int>(interval_scale * kTimePiece);
      EXPECT_GT(interval, 0);
      if (main_loop && interval > 0 && timeout_watch_id_ < 0) {
        timeout_watch_id_ = main_loop->AddTimeoutWatch(interval,
                              new TimeoutWatchCallback(interval, times_, -1));
        EXPECT_GE(timeout_watch_id_, 0);
        EXPECT_EQ(MainLoopInterface::TIMEOUT_WATCH,
                  main_loop->GetWatchType(timeout_watch_id_));
        EXPECT_EQ(interval, main_loop->GetWatchData(timeout_watch_id_));
        DLOG("Added a timeout watch with interval=%d", interval);
      }
    } else if (strcmp(buf, "remove") == 0) {
      if (main_loop && timeout_watch_id_ >= 0) {
        main_loop->RemoveWatch(timeout_watch_id_);
        timeout_watch_id_ = -1;
      }
    }

    return true;
  }

  virtual void OnRemove(MainLoopInterface *main_loop, int watch_id) {
    EXPECT_TRUE(main_loop != NULL);
    if (main_loop) {
      EXPECT_EQ(MainLoopInterface::IO_READ_WATCH,
                main_loop->GetWatchType(watch_id));
      EXPECT_EQ(fd_, main_loop->GetWatchData(watch_id));
    }

    EXPECT_GT(strings_->size(), 0U);
    if (strings_->size() > 0)
      EXPECT_STREQ("quit", (*strings_)[strings_->size() - 1].c_str());
    if (main_loop && timeout_watch_id_ >= 0)
      main_loop->RemoveWatch(timeout_watch_id_);
    main_loop->Quit();
    delete this;
  }

 private:
  int timeout_watch_id_;
  int fd_;
  std::vector<std::string> *strings_;
  int *times_;
};

void IOReadWatchTest(MainLoopInterface *main_loop) {
  static const char *test_strings[] = {
    "Hello",
    "World",
    "blablabla",
    "A test string",
    "testing",
    "add 0.49", // A timeout watch will be added with interval of a little
                // less than 1/2 time piece.
    "Timeout added",
    "Wait for a while",
    "Wait 1 time piece more",
    "let's remove the timeout",
    "remove",
    "let's remove all watches",
    "quit",
    NULL
  };

  std::vector<std::string> strings;
  int times_a = 0;
  int times_b = 0;
  int output_pipe[2];  // For outputting strings to IOReadWatchCallback
  int ret;
  int timeout_watch_id;
  int io_watch_id;
  int pid;

  ret = pipe(output_pipe);
  ASSERT_EQ(0, ret);

  // Generate a new process to send the strings.
  pid = fork();

  // Parent process to run main loop
  if (pid == 0) {
    close(output_pipe[0]);
    // Child process to write the strings.
    for (int i = 0; test_strings[i]; ++i) {
      // sleep for 1/2 time piece.
      usleep(kTimePiece / 2 * 1000);
      // Write the test strings to output_pipe one by one.
      write(output_pipe[1], test_strings[i], strlen(test_strings[i]));
      // sleep for 1/2 time piece.
      usleep(kTimePiece / 2 * 1000);
    }
    exit(0);
  }

  close(output_pipe[1]);
  // Adds an IO read watch.
  io_watch_id =
      main_loop->AddIOReadWatch(output_pipe[0],
                                new IOReadWatchCallback(output_pipe[0],
                                                        &strings,
                                                        &times_a));
  ASSERT_GE(io_watch_id, 0);
  ASSERT_EQ(MainLoopInterface::IO_READ_WATCH,
            main_loop->GetWatchType(io_watch_id));
  ASSERT_EQ(output_pipe[0], main_loop->GetWatchData(io_watch_id));

  // Add a timeout watch with kTimePiece interval.
  int interval_b = static_cast<int>(kTimePiece * 1.05);
  timeout_watch_id = main_loop->AddTimeoutWatch(
      interval_b, new TimeoutWatchCallback(interval_b, &times_b, -1));

  main_loop->Run();

  main_loop->RemoveWatch(timeout_watch_id);
  // Make sure the watch was actually removed.
  EXPECT_EQ(MainLoopInterface::INVALID_WATCH,
            main_loop->GetWatchType(timeout_watch_id));
  EXPECT_EQ(MainLoopInterface::INVALID_WATCH,
            main_loop->GetWatchType(io_watch_id));
  // This test is not very accurate on different machines.
  EXPECT_TRUE(times_a >= 9 && times_a <= 11);
  EXPECT_TRUE(times_b >= 11 && times_b <= 13);
  for (int i = 0; test_strings[i]; ++i)
    EXPECT_STREQ(test_strings[i], strings[i].c_str());
  close(output_pipe[0]);
}
#endif  // OS_WIN

// First, test basic functionalities of main loop in single thread by adding
// many timeout watches and check if they are called for expected times in a
// certain period.
void TimeoutWatchTest(MainLoopInterface *main_loop) {
  int watch_id;
  int times[11];

  for (int i = 0; i < 10; ++i) {
    int interval = (i+1) * kTimePiece;
    times[i] = 0;
    watch_id =
        main_loop->AddTimeoutWatch(
            interval, new TimeoutWatchCallback(interval, times + i, -1));
    ASSERT_GE(watch_id, 0);
  }

  // Loops for kNumLoops times, each interval is 10 * kTimePiece.
  times[10] = 0;
  const int kNumLoops = 3;
  watch_id =
      main_loop->AddTimeoutWatch(
          kTimePiece * 10,
          new TimeoutWatchCallback(kTimePiece * 10, times + 10, kNumLoops));
  ASSERT_GE(watch_id, 0);

  main_loop->Run();

  char msg[100];
  for (int i = 0; i < 10; ++i) {
    snprintf(msg, arraysize(msg), "i=%d interval=%d", i, (i+1) * kTimePiece);
    // Accept one time error.
    EXPECT_NEAR_M(times[i], 10 * kNumLoops/(i+1), 1, msg);
  }
  snprintf(msg, arraysize(msg), "last watch, interval=%d", kTimePiece * 10);
  EXPECT_EQ_M(kNumLoops, times[10], msg);
}

#endif //GGADGET_TESTS_MAIN_LOOP_TEST_H__
