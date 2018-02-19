include_directories(after .)

set(target ft2_tests)

set(src ft2_tests.c ft2_test_fake.c ft2_test_isp.c ft2_test_h264.c ft2_test_sd.c ft2_test_aes.c
	ft2_test_usb.c ft2_test_gpio.c ft2_test_i2c.c ft2_test_uart02.c ft2_test_rtc.c
	ft2_test_audio.c ft2_test_eth.c ft2_test_stop.c ft2_test_disk.c ft2_test_jpeg.c)

add_library(${target} STATIC ${src})

target_link_libraries(${target} ft2protocol)

install(TARGETS ${target}
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
