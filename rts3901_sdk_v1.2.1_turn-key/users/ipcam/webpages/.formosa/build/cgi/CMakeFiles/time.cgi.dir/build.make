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
CMAKE_SOURCE_DIR = /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build

# Include any dependencies generated for this target.
include cgi/CMakeFiles/time.cgi.dir/depend.make

# Include the progress variables for this target.
include cgi/CMakeFiles/time.cgi.dir/progress.make

# Include the compile flags for this target's objects.
include cgi/CMakeFiles/time.cgi.dir/flags.make

cgi/CMakeFiles/time.cgi.dir/time.c.o: cgi/CMakeFiles/time.cgi.dir/flags.make
cgi/CMakeFiles/time.cgi.dir/time.c.o: ../../cgi/time.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object cgi/CMakeFiles/time.cgi.dir/time.c.o"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi && /home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/time.cgi.dir/time.c.o   -c /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/cgi/time.c

cgi/CMakeFiles/time.cgi.dir/time.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/time.cgi.dir/time.c.i"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi && /home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-gcc  $(C_DEFINES) $(C_FLAGS) -E /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/cgi/time.c > CMakeFiles/time.cgi.dir/time.c.i

cgi/CMakeFiles/time.cgi.dir/time.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/time.cgi.dir/time.c.s"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi && /home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-gcc  $(C_DEFINES) $(C_FLAGS) -S /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/cgi/time.c -o CMakeFiles/time.cgi.dir/time.c.s

cgi/CMakeFiles/time.cgi.dir/time.c.o.requires:
.PHONY : cgi/CMakeFiles/time.cgi.dir/time.c.o.requires

cgi/CMakeFiles/time.cgi.dir/time.c.o.provides: cgi/CMakeFiles/time.cgi.dir/time.c.o.requires
	$(MAKE) -f cgi/CMakeFiles/time.cgi.dir/build.make cgi/CMakeFiles/time.cgi.dir/time.c.o.provides.build
.PHONY : cgi/CMakeFiles/time.cgi.dir/time.c.o.provides

cgi/CMakeFiles/time.cgi.dir/time.c.o.provides.build: cgi/CMakeFiles/time.cgi.dir/time.c.o

cgi/CMakeFiles/time.cgi.dir/utils.c.o: cgi/CMakeFiles/time.cgi.dir/flags.make
cgi/CMakeFiles/time.cgi.dir/utils.c.o: ../../cgi/utils.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/CMakeFiles $(CMAKE_PROGRESS_2)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object cgi/CMakeFiles/time.cgi.dir/utils.c.o"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi && /home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/time.cgi.dir/utils.c.o   -c /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/cgi/utils.c

cgi/CMakeFiles/time.cgi.dir/utils.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/time.cgi.dir/utils.c.i"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi && /home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-gcc  $(C_DEFINES) $(C_FLAGS) -E /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/cgi/utils.c > CMakeFiles/time.cgi.dir/utils.c.i

cgi/CMakeFiles/time.cgi.dir/utils.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/time.cgi.dir/utils.c.s"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi && /home/tony_nie/ipcam/4dpocket/release/.build/build/toolchain/rsdk-4.8.5-5281-EL-3.10-u0.9.33-m32ut-160408/bin/rsdk-linux-gcc  $(C_DEFINES) $(C_FLAGS) -S /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/cgi/utils.c -o CMakeFiles/time.cgi.dir/utils.c.s

cgi/CMakeFiles/time.cgi.dir/utils.c.o.requires:
.PHONY : cgi/CMakeFiles/time.cgi.dir/utils.c.o.requires

cgi/CMakeFiles/time.cgi.dir/utils.c.o.provides: cgi/CMakeFiles/time.cgi.dir/utils.c.o.requires
	$(MAKE) -f cgi/CMakeFiles/time.cgi.dir/build.make cgi/CMakeFiles/time.cgi.dir/utils.c.o.provides.build
.PHONY : cgi/CMakeFiles/time.cgi.dir/utils.c.o.provides

cgi/CMakeFiles/time.cgi.dir/utils.c.o.provides.build: cgi/CMakeFiles/time.cgi.dir/utils.c.o

# Object files for target time.cgi
time_cgi_OBJECTS = \
"CMakeFiles/time.cgi.dir/time.c.o" \
"CMakeFiles/time.cgi.dir/utils.c.o"

# External object files for target time.cgi
time_cgi_EXTERNAL_OBJECTS =

cgi/time.cgi: cgi/CMakeFiles/time.cgi.dir/time.c.o
cgi/time.cgi: cgi/CMakeFiles/time.cgi.dir/utils.c.o
cgi/time.cgi: cgi/CMakeFiles/time.cgi.dir/build.make
cgi/time.cgi: cgi/CMakeFiles/time.cgi.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C executable time.cgi"
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/time.cgi.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
cgi/CMakeFiles/time.cgi.dir/build: cgi/time.cgi
.PHONY : cgi/CMakeFiles/time.cgi.dir/build

cgi/CMakeFiles/time.cgi.dir/requires: cgi/CMakeFiles/time.cgi.dir/time.c.o.requires
cgi/CMakeFiles/time.cgi.dir/requires: cgi/CMakeFiles/time.cgi.dir/utils.c.o.requires
.PHONY : cgi/CMakeFiles/time.cgi.dir/requires

cgi/CMakeFiles/time.cgi.dir/clean:
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi && $(CMAKE_COMMAND) -P CMakeFiles/time.cgi.dir/cmake_clean.cmake
.PHONY : cgi/CMakeFiles/time.cgi.dir/clean

cgi/CMakeFiles/time.cgi.dir/depend:
	cd /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/cgi /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi /home/tony_nie/ipcam/4dpocket/release/.build/build/users/ipcam/webpages/.formosa/build/cgi/CMakeFiles/time.cgi.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : cgi/CMakeFiles/time.cgi.dir/depend

