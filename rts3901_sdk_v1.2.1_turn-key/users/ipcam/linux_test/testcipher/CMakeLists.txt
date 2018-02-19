project(aplayer)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Iinclude -Wall")

include_directories(before include)

add_executable(testcipher testcipher.c)

install(TARGETS testcipher DESTINATION bin)
