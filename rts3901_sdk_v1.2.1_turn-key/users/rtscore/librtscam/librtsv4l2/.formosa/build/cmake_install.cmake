# Install script for directory: /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtscam/librtsv4l2

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtscam/librtsv4l2/.formosa/build")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "1")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FOREACH(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librtsv4l2.so.0.0.2"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librtsv4l2.so.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librtsv4l2.so"
      )
    IF(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      FILE(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    ENDIF()
  ENDFOREACH()
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtscam/librtsv4l2/.formosa/build/librtsv4l2.so.0.0.2"
    "/home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtscam/librtsv4l2/.formosa/build/librtsv4l2.so.0"
    "/home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtscam/librtsv4l2/.formosa/build/librtsv4l2.so"
    )
  FOREACH(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librtsv4l2.so.0.0.2"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librtsv4l2.so.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/librtsv4l2.so"
      )
    IF(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      IF(CMAKE_INSTALL_DO_STRIP)
        EXECUTE_PROCESS(COMMAND "/home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-strip" "${file}")
      ENDIF(CMAKE_INSTALL_DO_STRIP)
    ENDIF()
  ENDFOREACH()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtscam/librtsv4l2/.formosa/build/librtsv4l2.a")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtscam/librtsv4l2/rtsv4l2.h")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
ELSE(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
ENDIF(CMAKE_INSTALL_COMPONENT)

FILE(WRITE "/home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtscam/librtsv4l2/.formosa/build/${CMAKE_INSTALL_MANIFEST}" "")
FOREACH(file ${CMAKE_INSTALL_MANIFEST_FILES})
  FILE(APPEND "/home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtscam/librtsv4l2/.formosa/build/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
ENDFOREACH(file)
