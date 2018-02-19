CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(uphymdio)

INCLUDE_DIRECTORIES(after ../include)

set(target uphymdio)
set(src usb_phy_mdio.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)
