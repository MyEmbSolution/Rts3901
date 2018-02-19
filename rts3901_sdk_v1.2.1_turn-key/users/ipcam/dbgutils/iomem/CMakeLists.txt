CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(iomem)

INCLUDE_DIRECTORIES(after ../include)

set(target iomem)
set(src iomem.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)

if (DBGUTILS_IODMA_FLAG MATCHES "yes")
	set(target iodma)
	set(src iodma.c)
	add_executable(${target} ${src})
	install(TARGETS ${target}
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		RUNTIME DESTINATION bin)
endif()
