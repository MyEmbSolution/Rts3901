CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(ipcam_ft2)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

INCLUDE_DIRECTORIES(after inc)
INCLUDE_DIRECTORIES(after tests)
INCLUDE_DIRECTORIES(after core)

add_subdirectory(core)
add_subdirectory(tests)
add_subdirectory(ap)
add_subdirectory(tools)
