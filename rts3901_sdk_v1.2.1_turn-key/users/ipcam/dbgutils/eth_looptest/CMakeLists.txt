CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(eth_looptest)

INCLUDE_DIRECTORIES(after ../include)

set(target eth_looptest)
set(src eth_looptest.c)

add_executable(${target} ${src})

target_link_libraries(${target} pthread)

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)
