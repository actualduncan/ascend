# This file is part of the AMD & HSC Work Graph Playground.
#
# Copyright (C) 2024 Advanced Micro Devices, Inc. and Coburg University of Applied Sciences and Arts.
# All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

function(fetch_nuget_package)
    set(options)
    set(oneValueArgs PACKAGE VERSION)
    set(multiValueArgs)
	set(OUT_BINARY_PATH)
    cmake_parse_arguments(FETCH_NUGET_PACKAGE  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT DEFINED FETCH_NUGET_PACKAGE_PACKAGE)
        message(FATAL_ERROR "Missing PACKAGE argument")
    endif()
    if (NOT DEFINED FETCH_NUGET_PACKAGE_VERSION)
        message(FATAL_ERROR "Missing VERSION argument")
    endif()

    set(DOWNLOAD_URL "https://www.nuget.org/api/v2/package/${FETCH_NUGET_PACKAGE_PACKAGE}/${FETCH_NUGET_PACKAGE_VERSION}")
    set(DOWNLOAD_FILE ${CMAKE_CURRENT_BINARY_DIR}/nuget/${FETCH_NUGET_PACKAGE_PACKAGE}.zip)
    set(PACKAGE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/nuget/${FETCH_NUGET_PACKAGE_PACKAGE})

    if (NOT EXISTS ${DOWNLOAD_FILE})
        message(STATUS "Downloading NuGet package \"${FETCH_NUGET_PACKAGE_PACKAGE}\" from \"${DOWNLOAD_URL}\".")

        file(DOWNLOAD "${DOWNLOAD_URL}" ${DOWNLOAD_FILE} STATUS DOWNLOAD_RESULT)

        list(GET DOWNLOAD_RESULT 0 DOWNLOAD_RESULT_CODE)
        if(NOT DOWNLOAD_RESULT_CODE EQUAL 0)
            message(FATAL_ERROR "Failed to download NuGet package \"${FETCH_NUGET_PACKAGE_PACKAGE}\" from \"${DOWNLOAD_URL}\". Error: ${DOWNLOAD_RESULT}.")
        endif()
    endif()

    file(ARCHIVE_EXTRACT
         INPUT ${DOWNLOAD_FILE}
         DESTINATION ${PACKAGE_DIRECTORY})

    message(STATUS "Adding NuGet package \"${FETCH_NUGET_PACKAGE_PACKAGE}\".")

    add_library(${FETCH_NUGET_PACKAGE_PACKAGE} INTERFACE)
    target_include_directories(${FETCH_NUGET_PACKAGE_PACKAGE} INTERFACE ${PACKAGE_DIRECTORY}/build/native/include)

    set(${OUT_BINARY_PATH} "${PACKAGE_DIRECTORY}/build/native/bin/x64")
	
    file(GLOB PACKAGE_BIN_FILES 
        ${PACKAGE_DIRECTORY}/build/native/bin/x64/*.exe
        ${PACKAGE_DIRECTORY}/build/native/bin/x64/*.dll
        ${PACKAGE_DIRECTORY}/build/native/bin/x64/*.pdb)

    foreach(PACKAGE_BIN_FILE ${PACKAGE_BIN_FILES})
        get_filename_component(PACKAGE_BIN_FILE_NAME ${PACKAGE_BIN_FILE} NAME)
        message("Generating custom command for ${PACKAGE_BIN_FILE_NAME}")
        add_custom_command(
            OUTPUT   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/${PACKAGE_BIN_FILE_NAME}
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PACKAGE_BIN_FILE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
            MAIN_DEPENDENCY  ${PACKAGE_BIN_FILE}
            COMMENT "Updating ${PACKAGE_BIN_FILE} into bin folder"
        )
        list(APPEND COPY_FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/${PACKAGE_BIN_FILE_NAME})
    endforeach()

    add_custom_target(${FETCH_NUGET_PACKAGE_PACKAGE}_copy DEPENDS "${COPY_FILES}")
    set_target_properties(${FETCH_NUGET_PACKAGE_PACKAGE}_copy PROPERTIES FOLDER CopyTargets)

    add_dependencies(${FETCH_NUGET_PACKAGE_PACKAGE} ${FETCH_NUGET_PACKAGE_PACKAGE}_copy)
    
endfunction(fetch_nuget_package)
