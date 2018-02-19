CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(veridisk)

set(target veridisk)
set(src veridisk.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)
