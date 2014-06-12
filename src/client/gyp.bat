@echo off
set PROTOC=%~dp0\depends\\protoc.exe
set PROTOBUF=%~dp0\depends\protobuf-2.5.0
set GTEST=%~dp0\depends\gtest-1.7.0
set ZLIB=%~dp0\depends\zlib-1.2.8
set WTL80=%~dp0\depends\wtl80\
set GYP=%~dp0\depends\gyp
%gyp%\gyp.bat --depth %~dp0 -I build\common.gypi -G msvs_version=2012