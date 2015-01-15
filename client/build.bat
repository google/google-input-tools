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

if exist env.bat call env.bat

set build_config=Debug

:parse_cmdline_options
if "%1" == "" goto end_parsing_cmdline_options
if /I "%1" == "debug" set build_config=Debug
if /I "%1" == "release" set build_config=Release
shift
goto parse_cmdline_options
:end_parsing_cmdline_options

call gyp.bat
msbuild.exe all.sln /p:Configuration="%build_config%" /p:TargetPlatform="Mixed Platforms"
