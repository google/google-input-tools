@echo off
set PROTOC=%~dp0\third_party\protobuf\bin\protoc.exe
set PROTOBUF=%~dp0\third_party\protobuf
set GTEST=%~dp0\third_party\gtest
set ZLIB=%~dp0\third_party\zlib
set WTL80=%~dp0\third_party\wtl_80
..\..\third_party\gyp\files\gyp.bat --depth %~dp0 -I build\common.gypi -G msvs_version=2012