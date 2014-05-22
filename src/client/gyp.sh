#!/bin/bash
#
# Copyright 2013 Google Inc. All Rights Reserved.
# Launcher script for gyp.py

DIR_NAME=$(cd $(dirname $0); pwd)

export GYP_DIR="${DIR_NAME}/../../../google3/third_party/gyp"

"$GYP_DIR/gyp" --depth="${DIR_NAME}" -I ${DIR_NAME}/build/common.gypi "$@"
