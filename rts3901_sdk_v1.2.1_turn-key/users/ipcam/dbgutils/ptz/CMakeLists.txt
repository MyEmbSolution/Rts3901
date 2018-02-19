CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(ptz)

INCLUDE_DIRECTORIES(after ../include)

set(target ptz)
set(src ptz.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)
