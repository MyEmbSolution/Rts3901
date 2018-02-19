CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(testvideo)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -shared-libgcc -std=gnu99")

INCLUDE_DIRECTORIES(after include)

add_subdirectory(core)
add_subdirectory(tests)

install (FILES include/rtsftest.h DESTINATION include)
