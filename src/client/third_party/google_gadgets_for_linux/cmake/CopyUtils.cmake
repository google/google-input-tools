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

MACRO(COPY_FILE_INTERNAL _source _dest _depends)
  STRING(REPLACE / + COPY_FILE_INTERNAL_target ${_dest})
  ADD_CUSTOM_TARGET(${COPY_FILE_INTERNAL_target}_copy ALL
    DEPENDS ${_dest})
  FILE(TO_NATIVE_PATH ${_source} COPY_FILE_INTERNAL_native_source)
  FILE(TO_NATIVE_PATH ${_dest} COPY_FILE_INTERNAL_native_dest)
  ADD_CUSTOM_COMMAND(OUTPUT ${_dest}
    COMMAND rm -f ${COPY_FILE_INTERNAL_native_dest} 
    COMMAND cp -Pp
      ${COPY_FILE_INTERNAL_native_source} ${COPY_FILE_INTERNAL_native_dest}
    DEPENDS ${_depends})
ENDMACRO(COPY_FILE_INTERNAL _source _dest)

#! Copy the file to the specified location.
#! @param _source the full path of the file.
#! @param _dest_dir the full path of the destination directory.
#! @paramoptional _new_name copy to the new directory with the new name.
MACRO(COPY_FILE _source _dest_dir)
  FILE(MAKE_DIRECTORY ${_dest_dir})
  IF(${ARGC} GREATER 2)
    SET(COPY_FILE_dest_name ${ARGV2})
  ELSE(${ARGC} GREATER 2)
    GET_FILENAME_COMPONENT(COPY_FILE_dest_name ${_source} NAME)
  ENDIF(${ARGC} GREATER 2)
  COPY_FILE_INTERNAL(${_source} ${_dest_dir}/${COPY_FILE_dest_name} ${_source})
ENDMACRO(COPY_FILE _source _dest_dir)

#! Copy the directory to the specified location.
#! @param _source the full path of the directory.
#! @param _dest the full path of the destination directory.
MACRO(COPY_DIR _source _dest)
  FILE(GLOB_RECURSE COPY_DIR_files ${_source} ${_source}/*)
  GET_FILENAME_COMPONENT(COPY_DIR_source_dir_name ${_source} NAME)
  FOREACH(COPY_DIR_filename ${COPY_DIR_files})
    FILE(RELATIVE_PATH COPY_DIR_relative_name ${_source} ${COPY_DIR_filename})
    GET_FILENAME_COMPONENT(COPY_DIR_dest_file_dir
      ${_dest}/${COPY_DIR_relative_name} PATH)
    IF(NOT ${COPY_DIR_filename} MATCHES /\\.svn/)
      COPY_FILE(${COPY_DIR_filename} ${COPY_DIR_dest_file_dir})
    ENDIF(NOT ${COPY_DIR_filename} MATCHES /\\.svn/)
  ENDFOREACH(COPY_DIR_filename ${COPY_DIR_files})
ENDMACRO(COPY_DIR _source _dest_dir)

#! Copy the target to the specified location.
#! @param _target_name the name of the target.
#! @param _dest_dir the full path of the destination directory.
#! @paramoptional _new_name copy to the new directory with the new name.
MACRO(COPY_TARGET _target_name _dest_dir)
  FILE(MAKE_DIRECTORY ${_dest_dir})
  GET_TARGET_PROPERTY(COPY_TARGET_location ${_target_name} LOCATION)
  GET_FILENAME_COMPONENT(COPY_TARGET_name ${COPY_TARGET_location} NAME)
  IF(${ARGC} GREATER 2)
    COPY_FILE_INTERNAL(${COPY_TARGET_location} ${_dest_dir}/${ARGV2}
      ${_target_name})
  ELSE(${ARGC} GREATER 2)
    COPY_FILE_INTERNAL(${COPY_TARGET_location} ${_dest_dir}/${COPY_TARGET_name}
      ${_target_name})
    # Linux requires the symbolic link mechanism for .so files.
    IF(NOT WIN32)
      GET_TARGET_PROPERTY(COPY_TARGET_soversion ${_target_name} SOVERSION)
      GET_TARGET_PROPERTY(COPY_TARGET_version ${_target_name} VERSION)
      GET_TARGET_PROPERTY(COPY_TARGET_suffix ${_target_name} PREFIX)
      GET_FILENAME_COMPONENT(COPY_TARGET_path ${COPY_TARGET_location} PATH)
      # SOVERSION symbolic link.
      IF(NOT COPY_TARGET_soversion MATCHES "NOTFOUND")
        # Filename of shared libraries in Mac OS X are <libname>.<soversion>.dylib
        # instead of <libname>.dylib.<soversion>
        IF(APPLE)
          COPY_FILE_INTERNAL(${COPY_TARGET_path}/lib${_target_name}.${COPY_TARGET_soversion}${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${_dest_dir}/lib${_target_name}.${COPY_TARGET_soversion}${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${_target_name})
        ELSE(APPLE)
          COPY_FILE_INTERNAL(${COPY_TARGET_location}.${COPY_TARGET_soversion}
            ${_dest_dir}/${COPY_TARGET_name}.${COPY_TARGET_soversion}
            ${_target_name})
        ENDIF(APPLE)
      ENDIF(NOT COPY_TARGET_soversion MATCHES "NOTFOUND")
      # VERSION symbolic link.
      IF(NOT COPY_TARGET_version MATCHES "NOTFOUND")
        IF(APPLE)
          COPY_FILE_INTERNAL(${COPY_TARGET_path}/lib${_target_name}.${COPY_TARGET_version}${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${_dest_dir}/lib${_target_name}.${COPY_TARGET_version}${CMAKE_SHARED_LIBRARY_SUFFIX}
            ${_target_name})
        ELSE(APPLE)
          COPY_FILE_INTERNAL(${COPY_TARGET_location}.${COPY_TARGET_version}
            ${_dest_dir}/${COPY_TARGET_name}.${COPY_TARGET_version}
            ${_target_name})
        ENDIF(APPLE)
      ENDIF(NOT COPY_TARGET_version MATCHES "NOTFOUND")
    ENDIF(NOT WIN32)
  ENDIF(${ARGC} GREATER 2)
ENDMACRO(COPY_TARGET)

#! Copy the file to the specified location under ${CMAKE_BINARY_DIR}/output.
#! @param _source the full path of the file.
#! @param _dest_dir the full path of the destination directory.
#! @paramoptional _new_name copy to the new directory with the new name.
MACRO(OUTPUT_FILE _source _dest_dir)
  IF(${ARGC} GREATER 2)
    COPY_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${_source}
      ${CMAKE_BINARY_DIR}/output/${_dest_dir} ${ARGV2})
  ELSE(${ARGC} GREATER 2)
    COPY_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${_source}
      ${CMAKE_BINARY_DIR}/output/${_dest_dir})
  ENDIF(${ARGC} GREATER 2)
ENDMACRO(OUTPUT_FILE _source _dest_dir)

#! Copy the directory to the specified location under ${CMAKE_BINARY_DIR}/output.
#! @param _source the full path of the directory.
#! @param _dest_dir the full path of the destination directory.
MACRO(OUTPUT_DIR _source _dest_dir)
  COPY_DIR(${CMAKE_CURRENT_SOURCE_DIR}/${_source}
    ${CMAKE_BINARY_DIR}/output/${_dest_dir} ${ARGV2})
ENDMACRO(OUTPUT_DIR _source _dest_dir)

#! Copy the target to the specified location under ${CMAKE_BINARY_DIR}/output.
#! @param _target_name the name of the target.
#! @param _dest_dir the full path of the destination directory.
#! @paramoptional _new_name copy to the new directory with the new name.
MACRO(OUTPUT_TARGET _target_name _dest_dir)
  IF(${ARGC} GREATER 2)
    COPY_TARGET(${_target_name} ${CMAKE_BINARY_DIR}/output/${_dest_dir} ${ARGV2})
  ELSE(${ARGC} GREATER 2)
    COPY_TARGET(${_target_name} ${CMAKE_BINARY_DIR}/output/${_dest_dir})
  ENDIF(${ARGC} GREATER 2)
ENDMACRO(OUTPUT_TARGET _target_name _dest_dir)
