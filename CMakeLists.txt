cmake_minimum_required(VERSION 3.23.2)
project(cppchallenger)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fexceptions -g -Wall")

include_directories(3rdparty/rapidxml)
include_directories(3rdparty/nlohmannjson)
include_directories(3rdparty/sqlite)
include_directories(3rdparty/sqlite_modern_cpp/hdr)


add_executable(math math.cc)
add_executable(lang lang.cc)
add_executable(string string.cc)
add_executable(stream_fs stream_fs.cc)
add_executable(time_date time_date.cc)
add_executable(algorithm algorithm.cc)
add_executable(concurrent concurrent.cc)
add_executable(design_pattern design_pattern.cc)
add_executable(serialization serialization.cc)
# add_executables(archive archive.cc)
add_executable(database 3rdparty/sqlite/sqlite3.c database.cc)
add_executable(crypto crypto.cc)

add_custom_command(
  TARGET database PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${PROJECT_SOURCE_DIR}/cppchallenger85.db ${CMAKE_CURRENT_BINARY_DIR})
