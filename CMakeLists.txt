cmake_minimum_required(VERSION 3.25)
message("ascend.")
project(
	ascend 
	VERSION 0.0.1
	DESCRIPTION "ascend."
	LANGUAGES CXX)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
add_subdirectory(src)
add_subdirectory(test)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ascend)