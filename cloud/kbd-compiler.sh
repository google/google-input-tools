#!/bin/bash
src=/usr/local/google/home/wuyingbing/opensource/google-input-tools/src/cloud
closure=/usr/local/google/home/wuyingbing/opensource/closure
$closure/closure/bin/calcdeps.py -i $src/keyboard/standalone.js \
  -p $closure -o compiled -p $src \
  -c $closure/compiler.jar \
  -f "--compilation_level=ADVANCED_OPTIMIZATIONS" \
  > kbd.js
