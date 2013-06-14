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
    By default it is `java -jar ~/closure/compiler.jar`.
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
