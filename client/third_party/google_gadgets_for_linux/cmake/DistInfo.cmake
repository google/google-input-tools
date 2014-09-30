#
# Copyright 2007 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Check distribution information, including distribution id, version,
# architecture, etc. String macro GGL_DIST_INFO will be defined.
IF(NOT CMAKE_CROSSCOMPILING)
  # lsb_release and /etc/*-release can only be trusted
  # when it's not cross compiling.
  EXECUTE_PROCESS(COMMAND lsb_release -d -s
    OUTPUT_VARIABLE GGL_DIST_DESC
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  STRING(REGEX REPLACE "\"" "" GGL_DIST_DESC "${GGL_DIST_DESC}")
  IF(NOT GGL_DIST_DESC)
    SET(DIST_RELEASE_FILES 
      /etc/fedora-release
      /etc/redhat-release
      /etc/SuSE-release
      /etc/sles-release
      /etc/gentoo-release
      /etc/mandriva-release
      /etc/debian_version
      /etc/arch-release
      /etc/arklinux-release
      /etc/slaclware-version
      /etc/pld-release
      /etc/knoppix_version
      /etc/lfs-release
      /etc/linuxppc-release
      /etc/yellowdog-release
      /etc/immunix-release
      /etc/sun-release
      /etc/release
    )
    FOREACH(RELEASE_FILE ${DIST_RELEASE_FILES})
      IF(NOT GGL_DIST_DESC)
        IF(EXISTS ${RELEASE_FILE})
          FILE(READ ${RELEASE_FILE} GGL_DIST_DESC)
          STRING(REGEX REPLACE "\n.*" "" GGL_DIST_DESC ${GGL_DIST_DESC})
        ENDIF(EXISTS ${RELEASE_FILE})
      ENDIF(NOT GGL_DIST_DESC)
    ENDFOREACH(RELEASE_FILE ${DIST_RELEASE_FILES})
  ENDIF(NOT GGL_DIST_DESC)
ENDIF(NOT CMAKE_CROSSCOMPILING)

EXECUTE_PROCESS(COMMAND uname -m
  OUTPUT_VARIABLE GGL_DIST_CPU
  OUTPUT_STRIP_TRAILING_WHITESPACE)
IF(NOT GGL_DIST_CPU)
  SET(GGL_DIST_CPU unknown)
ENDIF(NOT GGL_DIST_CPU)
SET(GGL_DIST_INFO "${GGL_HOST_TYPE}-${GGL_DIST_CPU} (${GGL_DIST_DESC})")

IF(GGL_OEM_BRAND)
  SET(GGL_DIST_INFO "${GGL_DIST_INFO} ${GGL_OEM_BRAND}")
ELSE(GGL_OEM_BRAND)
  SET(GGL_DIST_INFO "${GGL_DIST_INFO} (-)")
ENDIF(GGL_OEM_BRAND)
