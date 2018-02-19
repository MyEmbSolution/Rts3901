CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(bus_monitor)

INCLUDE_DIRECTORIES(after ../include)

set(target monitor)
set(src monitor.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)
