:: Copyright 2014 Google Inc.
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::      http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS-IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

@echo off
setlocal
set PROTOC=%~dp0\depends\protoc.exe
set PROTOBUF=%~dp0\depends\protobuf-2.5.0
set GTEST=%~dp0\depends\gtest-1.7.0
set ZLIB=%~dp0\depends\zlib-1.2.8
set WTL80=%~dp0\depends\wtl80\
set GYP=%~dp0\depends\gyp
if not defined MSVS_VERSION set MSVS_VERSION=2012
call %gyp%\gyp.bat --depth %~dp0 -I build\common.gypi -G msvs_version=%MSVS_VERSION%
python build_x86_x64_together.py
