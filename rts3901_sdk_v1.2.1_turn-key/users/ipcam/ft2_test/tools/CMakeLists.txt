set(target i2c_program)
set(src i2c_program.c)

add_executable(${target} ${src})

install(TARGETS ${target}
	RUNTIME DESTINATION bin)
