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
# Copyright 2013 Google Inc. All Rights Reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

# Project information
PROJECT:=hangul
LDFLAGS:=-lppapi_cpp -lppapi -ljsoncpp -lhangul
CXX_SOURCES:=$(PROJECT).cc url_loader_util.cc hanja.cc unicode_util.cc

# Project Build flags
WARNINGS:=-Wno-long-long -Wall -Wswitch-enum -pedantic -Werror
OPTFLAGS:=-O2
CXXFLAGS:=-pthread -std=gnu++98 $(WARNINGS) $(OPTFLAGS) \
	-I $(NACL_SDK_ROOT)/include

# Compute tool paths
OSNAME:=$(shell python $(NACL_SDK_ROOT)/tools/getos.py)
TC_PATH:=$(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_newlib)
CC:=$(TC_PATH)/bin/i686-nacl-gcc
CXX:=$(TC_PATH)/bin/i686-nacl-g++

TC_PATH_ARM:=$(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_arm_newlib)
CC_ARM:=$(TC_PATH_ARM)/bin/arm-nacl-gcc
CXX_ARM:=$(TC_PATH_ARM)/bin/arm-nacl-g++

# Declare the ALL target first, to make the 'all' target the default build
all: $(PROJECT)_x86_64.nexe $(PROJECT)_x86_32.nexe $(PROJECT)_arm.nexe

# Define 32 bit compile and link rules for main application
x86_32_OBJS:=$(patsubst %.cc,%_32.o,$(CXX_SOURCES))
$(x86_32_OBJS) : %_32.o : %.cc $(THIS_MAKE)
	$(CXX) -o $@ -c $< -m32 $(CXXFLAGS)

$(PROJECT)_x86_32.nexe : $(x86_32_OBJS)
	$(CXX) -o $@ $^ -m32 $(CXXFLAGS) $(LDFLAGS)

# Define 64 bit compile and link rules for C++ sources
x86_64_OBJS:=$(patsubst %.cc,%_64.o,$(CXX_SOURCES))
$(x86_64_OBJS) : %_64.o : %.cc $(THIS_MAKE)
	$(CXX) -o $@ -c $< -m64 $(CXXFLAGS)

$(PROJECT)_x86_64.nexe : $(x86_64_OBJS)
	$(CXX) -o $@ $^ -m64 $(CXXFLAGS) $(LDFLAGS)

# Define arm compile and link rules for C++ sources
ARM_OBJS:=$(patsubst %.cc,%_arm.o,$(CXX_SOURCES))
$(ARM_OBJS) : %_arm.o : %.cc $(THIS_MAKE)
	$(CXX_ARM) -o $@ -c $< $(CXXFLAGS)

$(PROJECT)_arm.nexe : $(ARM_OBJS)
	$(CXX_ARM) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f *.o *.nexe *.nmf *.txt

test: all
	@cp ../misc/*.nmf ../misc/*.txt .
	@echo "Open your web browser and see http://127.0.0.1:8000/test.html"
	python -m SimpleHTTPServer
