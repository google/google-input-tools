POSIX compatibility layer for Non-POSIX platforms.

Currently it supports VC10 on Windows.

--------------------------------------------------

This directory mainly parallels POSIX's /usr/include subtree. The goal is to
make it possible for google3 sources to be compiled in non-POSIX platforms.

This library is modified from //depot/googleclient/posix/...
Original Author: Eric Fredricksen
We only include a subset of the original implementation which is sufficient
to compile all google3 library that shared engine depends on.

Sometimes there's a header file conflict, for example, <signal.h> exists in the
Windows SDK, but doesn't define everything POSIX expects. In such cases, a file
called "conflict-blah.h" (e.g. "conflict-signal.h") appears in this directory.
In these cases they must sometimes be explicitly included in .cc files.

In addition, the file unistd.h defines some of the things you'd expect to find
in some other header -- often for reasons of name conflicts for the includes.

---------------------------------------------------------------------

GCC notes.

Here's some things that occur in google3 code that are gcc
differences or extensions, and have to be changed to compile under VC7.
Eng should be trained to avoid these things.
- ?: operator -- replace by standard code (an if, perhaps)
- char array[nonconst] -- can be replaced by scoped_array<char>.
- 1LL or 1ULL suffix -- replace with GG_LONGLONG() or GG_ULONGLONG()
