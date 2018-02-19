set(target ft2_test)
set(src ft2test.c)

add_executable(${target} ${src})

target_link_libraries(${target} tester ft2_tests pthread h1encoder rtsisp rtscamkit asound
rtstream rtsv4l2 rtsjpeg rtsosd rtsamixer)

install(TARGETS ${target}
	RUNTIME DESTINATION bin)
