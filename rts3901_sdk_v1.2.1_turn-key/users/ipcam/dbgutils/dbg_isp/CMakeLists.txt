CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(dbg_isp)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -shared-libgcc")

if (TARGET_PLATFORM MATCHES "ipcam")
	if (IPCAM_ROOT)
		INCLUDE_DIRECTORIES(after ${IPCAM_ROOT}/tmpfs/include)
		LINK_DIRECTORIES(${IPCAM_ROOT}/tmpfs/lib)
	endif ()
endif ()

INCLUDE_DIRECTORIES(after .)
INCLUDE_DIRECTORIES(after inc)

set(target dbg_isp)
set(src dbg_isp.c dbg_log.c dbg_ctrl.c dbg_strm.c dbg_opt.c dbg_h264.c)

add_executable(${target} ${src})

target_link_libraries(${target} pthread h1encoder rtsisp rtsutils rtsv4l2 rtsjpeg rtscamkit)

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)

set(target dbg_enc)
set(src dbg_enc.c dbg_log.c dbg_opt.c dbg_h264.c)

add_executable(${target} ${src})

target_link_libraries(${target} pthread h1encoder rtsisp rtscamkit)

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)

set(target rts_isp_cmd)
set(src dbg_isp_cmd.c)

add_executable(${target} ${src})

target_link_libraries(${target} rtsisp rtscamkit)

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)

set(target dbg_reg)
set(src dbg_reg.c)

add_executable(${target} ${src})

target_link_libraries(${target} rtsisp rtscamkit)

install(TARGETS ${target}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)

