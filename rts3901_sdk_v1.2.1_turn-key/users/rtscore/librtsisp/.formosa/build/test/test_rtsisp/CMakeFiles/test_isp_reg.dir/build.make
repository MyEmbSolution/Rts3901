# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build

# Include any dependencies generated for this target.
include test/test_rtsisp/CMakeFiles/test_isp_reg.dir/depend.make

# Include the progress variables for this target.
include test/test_rtsisp/CMakeFiles/test_isp_reg.dir/progress.make

# Include the compile flags for this target's objects.
include test/test_rtsisp/CMakeFiles/test_isp_reg.dir/flags.make

test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o: test/test_rtsisp/CMakeFiles/test_isp_reg.dir/flags.make
test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o: ../../test/test_rtsisp/rts_test_reg.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build/test/test_rtsisp && /home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o   -c /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/test/test_rtsisp/rts_test_reg.c

test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/test_isp_reg.dir/rts_test_reg.c.i"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build/test/test_rtsisp && /home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-gcc  $(C_DEFINES) $(C_FLAGS) -E /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/test/test_rtsisp/rts_test_reg.c > CMakeFiles/test_isp_reg.dir/rts_test_reg.c.i

test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/test_isp_reg.dir/rts_test_reg.c.s"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build/test/test_rtsisp && /home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-gcc  $(C_DEFINES) $(C_FLAGS) -S /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/test/test_rtsisp/rts_test_reg.c -o CMakeFiles/test_isp_reg.dir/rts_test_reg.c.s

test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o.requires:
.PHONY : test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o.requires

test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o.provides: test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o.requires
	$(MAKE) -f test/test_rtsisp/CMakeFiles/test_isp_reg.dir/build.make test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o.provides.build
.PHONY : test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o.provides

test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o.provides.build: test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o

# Object files for target test_isp_reg
test_isp_reg_OBJECTS = \
"CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o"

# External object files for target test_isp_reg
test_isp_reg_EXTERNAL_OBJECTS =

test/test_rtsisp/test_isp_reg: test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o
test/test_rtsisp/test_isp_reg: test/test_rtsisp/CMakeFiles/test_isp_reg.dir/build.make
test/test_rtsisp/test_isp_reg: rtsisp/librtsisp.so.0.0.11
test/test_rtsisp/test_isp_reg: test/test_rtsisp/CMakeFiles/test_isp_reg.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C executable test_isp_reg"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build/test/test_rtsisp && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_isp_reg.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/test_rtsisp/CMakeFiles/test_isp_reg.dir/build: test/test_rtsisp/test_isp_reg
.PHONY : test/test_rtsisp/CMakeFiles/test_isp_reg.dir/build

test/test_rtsisp/CMakeFiles/test_isp_reg.dir/requires: test/test_rtsisp/CMakeFiles/test_isp_reg.dir/rts_test_reg.c.o.requires
.PHONY : test/test_rtsisp/CMakeFiles/test_isp_reg.dir/requires

test/test_rtsisp/CMakeFiles/test_isp_reg.dir/clean:
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build/test/test_rtsisp && $(CMAKE_COMMAND) -P CMakeFiles/test_isp_reg.dir/cmake_clean.cmake
.PHONY : test/test_rtsisp/CMakeFiles/test_isp_reg.dir/clean

test/test_rtsisp/CMakeFiles/test_isp_reg.dir/depend:
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/test/test_rtsisp /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build/test/test_rtsisp /home/tony_nie/ipcam/4dpocket/release/.build/build/users/rtscore/librtsisp/.formosa/build/test/test_rtsisp/CMakeFiles/test_isp_reg.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/test_rtsisp/CMakeFiles/test_isp_reg.dir/depend

