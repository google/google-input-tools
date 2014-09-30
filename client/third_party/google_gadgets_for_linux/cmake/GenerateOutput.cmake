#
# Copyright 2008 Google Inc.
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

IF(WIN32)
  MACRO(OUTPUT_EXECUTABLE _target_name)
    OUTPUT_TARGET(${_target_name} /)
  ENDMACRO(OUTPUT_EXECUTABLE _target_name)
  MACRO(OUTPUT_LIBRARY _target_name)
    OUTPUT_TARGET(${_target_name} /)
  ENDMACRO(OUTPUT_LIBRARY _target_name)
ELSE(WIN32)
  MACRO(OUTPUT_EXECUTABLE _target_name)
    OUTPUT_TARGET(${_target_name} bin ${_target_name}.bin)
    COPY_FILE(${CMAKE_SOURCE_DIR}/cmake/bin_wrapper.sh
      ${CMAKE_BINARY_DIR}/output/bin ${_target_name})
  ENDMACRO(OUTPUT_EXECUTABLE _target_name)
  MACRO(OUTPUT_LIBRARY _target_name)
    SET_TARGET_PROPERTIES(${_target_name} PROPERTIES
      VERSION ${GGL_LIB_VERSION}
      SOVERSION ${GGL_LIB_SOVERSION})
    OUTPUT_TARGET(${_target_name} lib)
    INSTALL(TARGETS ${_target_name}
      RUNTIME DESTINATION ${BIN_INSTALL_DIR}
      LIBRARY DESTINATION ${LIB_INSTALL_DIR}
      ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
  ENDMACRO(OUTPUT_LIBRARY _target_name)
ENDIF(WIN32)

MACRO(ADD_MODULE _target_name)
  ADD_LIBRARY(${_target_name} MODULE ${ARGN})
  SET_TARGET_PROPERTIES(${_target_name} PROPERTIES PREFIX "")
ENDMACRO(ADD_MODULE _target_name)

MACRO(OUTPUT_MODULE _target_name)
  OUTPUT_TARGET(${_target_name} modules)
  INSTALL(TARGETS ${_target_name}
    LIBRARY DESTINATION "${LIB_INSTALL_DIR}/google-gadgets/modules/")
ENDMACRO(OUTPUT_MODULE _target_name)

MACRO(INSTALL_GADGET _gadget_name)
  INSTALL(FILES "${CMAKE_BINARY_DIR}/output/bin/${_gadget_name}"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/share/google-gadgets")
ENDMACRO(INSTALL_GADGET _gadget_name)

MACRO(INSTALL_PKG_CONFIG _name)
  IF(NOT WIN32)
    CONFIGURE_FILE("${_name}${GGL_EPOCH}.pc.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${_name}${GGL_EPOCH}.pc"
      @ONLY)

    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_name}${GGL_EPOCH}.pc
      DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
  ENDIF(NOT WIN32)
ENDMACRO(INSTALL_PKG_CONFIG _name)

MACRO(INSTALL_BINARY_DESKTOP _name)
  SET(bindir ${CMAKE_INSTALL_PREFIX}/bin)
  SET(datadir ${CMAKE_INSTALL_PREFIX}/share)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${_name}.desktop.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_name}.desktop.in"
    @ONLY)
  #  ADD_CUSTOM_TARGET(GenAppDesktop ALL
  #  ./intl_desktop_file ${CMAKE_CURRENT_BINARY_DIR}/ggl-qt.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/ggl-qt.desktop
  #  DEPENDS intl_desktop_file
  #  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/output/bin
  #  )
  INSTALL(CODE
    "EXECUTE_PROCESS(COMMAND ./intl_desktop_file ${CMAKE_CURRENT_BINARY_DIR}/${_name}.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/${_name}.desktop WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/output/bin)"
    )

  INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${_name}.desktop
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/share/applications
    )
ENDMACRO(INSTALL_BINARY_DESKTOP _name)
