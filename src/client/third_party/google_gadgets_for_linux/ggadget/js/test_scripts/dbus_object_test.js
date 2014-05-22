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

TEST("Test Normal", function() {
  name = "org.freedesktop.DBus";
  path = "/org/freedesktop/DBus";
  sysobj = new DBusSystemObject(name, path, name);
  ASSERT(TRUE(sysobj));
  names = sysobj.ListNames();
  ASSERT(LE(1, names.length));

  sessionobj = new DBusSessionObject(name, path, name);
  ASSERT(NE(null, sysobj));
  names2 = sessionobj.ListNames();
  ASSERT(LE(1, names2.length));
});

RUN_ALL_TESTS();
