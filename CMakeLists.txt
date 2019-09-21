cmake_minimum_required(VERSION 3.15)
project(Outcurses VERSION 0.0.1
                  DESCRIPTION "High-level UI via ncurses"
		  HOMEPAGE_URL "https://nick-black.com/dankwiki/index.php/Outcurses"
		  LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

add_library(outcurses SHARED fade.c)

set(CURSES_NEED_WIDE TRUE)
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
target_include_directories(outcurses PRIVATE ${CURSES_INCLUDE_DIR})

file(GLOB TESTSRCS tests/*.cpp)
add_executable(outcurses-tester ${TESTSRCS})
find_package(GTest)
target_link_libraries(outcurses-tester
	GTest::GTest
	outcurses
)
target_include_directories(outcurses-tester PRIVATE include)

gtest_discover_tests(outcurses-tester)

install(TARGETS outcurses DESTINATION lib)

add_custom_command(TARGET outcurses-tester POST_BUILD
	COMMAND outcurses-tester --output-on-failure
)
