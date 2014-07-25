// Copyright 2014 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//
# Chrome OS Korean Input Method

Chrome OS Korean Input Method is an Korean input method based on Native Client
which runs in Chrome OS.
It depends on [libhangul](https://code.google.com/p/libhangul/).

## Requirements

* [Native Client SDK](https://developers.google.com/native-client/sdk/)
* [Closure Compiler](https://code.google.com/p/closure-compiler/)
* [NaClPorts](https://code.google.com/p/naclports/)

## How to build

Set up these environment vairbles:

* `CLOSURE_COMPILER` to the closure compiler binary.
    By default it is `~/closure/compiler.jar`.
* `NACL_SDK_ROOT` to your Native Client SDK.

Check into folder `naclports/src`, and run

    ./make_all.sh jsoncpp
    ./make_all.sh hangul

Then check in the source folder, and run

    make

And you will find two folders `debug` and `release` generated. You can use
"Load unpacked extension" on the extension settings page of your Chrome OS to
load it. By running `make pack` you will get a tarball that contains the
`release` folder.
