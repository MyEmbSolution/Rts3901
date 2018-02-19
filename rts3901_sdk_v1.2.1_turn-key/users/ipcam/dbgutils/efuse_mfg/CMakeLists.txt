CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(efuse_mfg)

INCLUDE_DIRECTORIES(after ../include)

set(target efuse_mfg)
set(src efuse_mfg.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)
