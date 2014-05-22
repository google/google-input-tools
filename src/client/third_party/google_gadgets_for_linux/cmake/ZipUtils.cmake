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

MACRO(ADD_DIR_TO_ZIP_ABS _source _zip_file)
  FILE(GLOB_RECURSE ADD_DIR_TO_ZIP_files ${_source}/*)
  SET(ADD_DIR_TO_ZIP_relative_files)
  SET(ADD_DIR_TO_ZIP_source_files)
  FOREACH(ADD_DIR_TO_ZIP_filename ${ADD_DIR_TO_ZIP_files})
    IF(NOT ${ADD_DIR_TO_ZIP_filename} MATCHES
      /\\.svn/|/CMakeLists\\.txt|/Makefile\\.)
      FILE(RELATIVE_PATH ADD_DIR_TO_ZIP_relative_name
        ${_source} ${ADD_DIR_TO_ZIP_filename})
      SET(ADD_DIR_TO_ZIP_source_files
        ${ADD_DIR_TO_ZIP_source_files} ${ADD_DIR_TO_ZIP_filename})
      SET(ADD_DIR_TO_ZIP_relative_files
        ${ADD_DIR_TO_ZIP_relative_files} ${ADD_DIR_TO_ZIP_relative_name})
    ENDIF(NOT ${ADD_DIR_TO_ZIP_filename} MATCHES
      /\\.svn/|/CMakeLists\\.txt|/Makefile\\.)
  ENDFOREACH(ADD_DIR_TO_ZIP_filename ${ADD_DIR_TO_ZIP_files})

  STRING(REPLACE / + ADD_DIR_TO_ZIP_target ${_zip_file}_${_source}_zip)
  ADD_CUSTOM_TARGET(${ADD_DIR_TO_ZIP_target} ALL
    ${CMAKE_SOURCE_DIR}/cmake/zip.sh -r -u ${_zip_file}
      ${ADD_DIR_TO_ZIP_relative_files}
    DEPENDS ${ADD_DIR_TO_ZIP_source_files}
    WORKING_DIRECTORY ${_source})
ENDMACRO(ADD_DIR_TO_ZIP_ABS _source _zip_file)

#! Make a zip file containing all the files in the directory.
#! @param _source directory relative to ${CMAKE_CURRENT_SOURCE_DIR}.
#! @param _zip_file zip file relative to ${CMAKE_CURRENT_BINARY_DIR}.
MACRO(ADD_DIR_TO_ZIP _source _zip_file)
  ADD_DIR_TO_ZIP_ABS(${CMAKE_CURRENT_SOURCE_DIR}/${_source}
    ${CMAKE_CURRENT_BINARY_DIR}/${_zip_file})
ENDMACRO(ADD_DIR_TO_ZIP _source _zip_file)

MACRO(ADD_FILE_TO_ZIP_ABS _file _zip_file)
  STRING(REPLACE / + ADD_FILE_TO_ZIP_target ${_zip_file}_${_file}_zip)
  GET_FILENAME_COMPONENT(ADD_FILE_TO_ZIP_name ${_file} NAME)
  GET_FILENAME_COMPONENT(ADD_FILE_TO_ZIP_path ${_file} PATH)
  ADD_CUSTOM_TARGET(${ADD_FILE_TO_ZIP_target} ALL
    ${CMAKE_SOURCE_DIR}/cmake/zip.sh -u ${_zip_file} ${ADD_FILE_TO_ZIP_name}
    DEPENDS ${_file}
    WORKING_DIRECTORY ${ADD_FILE_TO_ZIP_path})
ENDMACRO(ADD_FILE_TO_ZIP_ABS _file _zip_file)

MACRO(ADD_FILE_TO_ZIP _file _zip_file)
  ADD_FILE_TO_ZIP_ABS(${CMAKE_CURRENT_SOURCE_DIR}/${_file}
    ${CMAKE_CURRENT_BINARY_DIR}/${_zip_file})
ENDMACRO(ADD_FILE_TO_ZIP _file _zip_file)

MACRO(ADD_TARGET_TO_ZIP_ABS _target_name _zip_file)
  STRING(REPLACE / + ADD_TARGET_TO_ZIP_target ${_zip_file}_${target_name}_zip)
  GET_TARGET_PROPERTY(ADD_TARGET_TO_ZIP_location ${_target_name} LOCATION)
  GET_FILENAME_COMPONENT(ADD_TARGET_TO_ZIP_name
    ${ADD_TARGET_TO_ZIP_location} NAME)
  GET_FILENAME_COMPONENT(ADD_TARGET_TO_ZIP_path
    ${ADD_TARGET_TO_ZIP_location} PATH)
  ADD_CUSTOM_TARGET(${ADD_TARGET_TO_ZIP_target} ALL
    ${CMAKE_SOURCE_DIR}/cmake/zip.sh -u ${_zip_file} ${ADD_TARGET_TO_ZIP_name}
    DEPENDS ${_target_name}
    WORKING_DIRECTORY ${ADD_TARGET_TO_ZIP_path})
ENDMACRO(ADD_TARGET_TO_ZIP_ABS _target_name _zip_file)

MACRO(ADD_TARGET_TO_ZIP _target_name _zip_file)
  ADD_TARGET_TO_ZIP_ABS(${_target_name}
    ${CMAKE_CURRENT_BINARY_DIR}/${_zip_file})
ENDMACRO(ADD_TARGET_TO_ZIP _target_name _zip_file)
