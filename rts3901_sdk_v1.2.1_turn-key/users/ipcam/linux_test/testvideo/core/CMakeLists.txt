include_directories(after .)

set(target rtsftest)
set(src rtsftest.c)

add_library(${target} SHARED ${src})

set_target_properties(${target} PROPERTIES SOVERSION 0 VERSION 1.0.1)

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)
