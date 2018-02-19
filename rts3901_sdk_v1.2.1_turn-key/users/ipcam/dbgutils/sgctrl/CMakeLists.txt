CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(sgctl)

INCLUDE_DIRECTORIES(after ../include)

set(target sg_read10)
set(src read10.c rtcr.c sg_ctl.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)

set(target sg_write10)
set(src write10.c rtcr.c sg_ctl.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)
