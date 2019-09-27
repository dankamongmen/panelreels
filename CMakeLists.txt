cmake_minimum_required(VERSION 3.13)
project(Outcurses VERSION 0.0.2
                  DESCRIPTION "High-level UI via ncurses"
		  HOMEPAGE_URL "https://nick-black.com/dankwiki/index.php/Outcurses"
		  LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

include(GNUInstallDirs)

file(GLOB LIBSRCS CONFIGURE_DEPENDS src/*.c)
add_library(outcurses SHARED ${LIBSRCS})
target_include_directories(outcurses PRIVATE include)
set_target_properties(outcurses PROPERTIES
	PUBLIC_HEADER "include/outcurses.h"
	VERSION 0.0.1
	SOVERSION 0
)

set(CURSES_NEED_WIDE TRUE)
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses 6.1 REQUIRED)
target_include_directories(outcurses PRIVATE ${CURSES_INCLUDE_DIR})
target_link_libraries(outcurses INTERFACE ${CURSES_LIBRARIES})
# FIXME ideally this would come from FindCurses.cmake
target_compile_definitions(outcurses PRIVATE _DEFAULT_SOURCE _XOPEN_SOURCE=600)
target_compile_options(outcurses PRIVATE -Wall -W)

file(GLOB TESTSRCS CONFIGURE_DEPENDS tests/*.cpp)
add_executable(outcurses-tester ${TESTSRCS})
find_package(GTest 1.9 REQUIRED)
target_link_libraries(outcurses-tester
	GTest::GTest
	outcurses
)
target_include_directories(outcurses-tester PRIVATE include)

gtest_discover_tests(outcurses-tester)
enable_testing()

install(TARGETS outcurses
	LIBRARY
		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		COMPONENT Libraries
		NAMELINK_COMPONENT Development
	PUBLIC_HEADER
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
		COMPONENT Development
)
