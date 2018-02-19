include_directories(after .)

set(target testvideo)
set(src run_test.c)
set(src ${src} test_video.c)
set(src ${src} test_isp_preview.c)
set(src ${src} test_isp_h264.c)
set(src ${src} test_isp_mjpeg.c)
set(src ${src} test_isp_mixture.c)

add_executable(${target} ${src})

target_link_libraries(${target}
	rtsftest
	rtsisp
	rtscamkit
	rtsutils
	rtsjpeg
	rtsv4l2
	rtsosd
	rtstream
	asound
	rtsamixer
	h1encoder
	pthread)

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)

