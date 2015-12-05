cmake_minimum_required(VERSION 3.0)
project(broadcast)


# We set compiler flags at toplevel
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Werror -W -g")

set(BC_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")
add_subdirectory(src)
add_subdirectory(examples)
