#!/bin/bash

source gbash.sh || exit
cd $(gbash::get_google3_dir)

out_dir=/tmp/inputview-debug
rm -rf $out_dir
mkdir -p $out_dir

mkdir -p $out_dir/images
cp i18n/input/javascript/chos/inputview/images/* $out_dir/images/
cp i18n/input/javascript/chos/inputview/localdebug/index.html $out_dir/index.html
cp i18n/input/javascript/chos/inputview/localdebug/localdebug.js $out_dir/localdebug.js

mkdir -p $out_dir/inputview_layouts
blaze build i18n/input/javascript/chos/inputview/layouts:all
cp blaze-bin/i18n/input/javascript/chos/inputview/layouts/*.js $out_dir/inputview_layouts/
rm -rf $out_dir/inputview_layouts/*debug*
rm -rf $out_dir/inputview_layouts/*bundle*

mkdir -p $out_dir/config
blaze build i18n/input/javascript/chos/inputview/config:all
cp blaze-bin/i18n/input/javascript/chos/inputview/config/*.js $out_dir/config/
rm -rf $out_dir/config/*debug*
rm -rf $out_dir/config/*bundle*

blaze build i18n/input/javascript/chos/inputview:inputview-test-debug
cp blaze-bin/i18n/input/javascript/chos/inputview/inputview-test-debug.js $out_dir/inputview-chromeos-debug.js
blaze build i18n/input/javascript/chos/inputview:inputview_css
cp blaze-bin/i18n/input/javascript/chos/inputview/inputview_css.css $out_dir/

cd $out_dir
python -m SimpleHTTPServer 5050

