CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(fephy)

INCLUDE_DIRECTORIES(after ../include)

set(target fephy)
set(src fephy.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)
