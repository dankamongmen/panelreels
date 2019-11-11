cmake_minimum_required(VERSION 3.13)
project(Outcurses VERSION 0.0.5
  DESCRIPTION "High-level UI via ncurses"
  HOMEPAGE_URL "https://nick-black.com/dankwiki/index.php/Outcurses"
  LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

file(GLOB LIBSRCS CONFIGURE_DEPENDS src/lib/*.c)
add_library(outcurses SHARED ${LIBSRCS})
target_include_directories(outcurses PRIVATE include)
set_target_properties(outcurses PROPERTIES
  PUBLIC_HEADER "include/outcurses.h"
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

# CMake (even 3.16.0) has issues with ncurses wide support. Use pkg-config.
#set(CURSES_NEED_WIDE TRUE)
#set(CURSES_NEED_NCURSES TRUE)
#find_package(Curses 6.1 REQUIRED)
pkg_check_modules(CURSES REQUIRED ncursesw>=6.1)
pkg_check_modules(PANEL REQUIRED panelw>=6.1)
target_include_directories(outcurses
  PRIVATE
    ${CURSES_INCLUDE_DIR}
)
target_link_libraries(outcurses
  INTERFACE
    ${PANEL_LIBRARIES}
    ${CURSES_LIBRARIES}
)
# FIXME ideally this would come from FindCurses.cmake
target_compile_definitions(outcurses PRIVATE
  _DEFAULT_SOURCE _XOPEN_SOURCE=600
)
target_compile_options(outcurses
  PRIVATE
    -Wall -Wextra -W
  INTERFACE
    ${PANEL_CFLAGS} ${PANEL_CFLAGS_OTHER}
    ${CURSES_CFLAGS} ${CURSES_CFLAGS_OTHER}
)

file(GLOB BINSRCS CONFIGURE_DEPENDS src/bin/*.c)
add_executable(outcurses-demo ${BINSRCS})
target_include_directories(outcurses-demo PRIVATE include)
target_link_libraries(outcurses-demo
  PRIVATE
    outcurses
    Threads::Threads
)
target_compile_definitions(outcurses-demo PRIVATE
  _DEFAULT_SOURCE _XOPEN_SOURCE=600
)
target_compile_options(outcurses-demo PRIVATE
  ${CURSES_CFLAGS} ${CURSES_CFLAGS_OTHER}
  -Wall -Wextra
)

file(GLOB TESTSRCS CONFIGURE_DEPENDS tests/*.cpp)
add_executable(outcurses-tester ${TESTSRCS})
find_package(GTest 1.9 REQUIRED)
target_include_directories(outcurses-tester PRIVATE include)
target_link_libraries(outcurses-tester
  GTest::GTest
  outcurses
)
target_compile_definitions(outcurses-tester PRIVATE
  _DEFAULT_SOURCE _XOPEN_SOURCE=600
)
target_compile_options(outcurses-tester PRIVATE
  ${CURSES_CFLAGS} ${CURSES_CFLAGS_OTHER}
  -Wall -Wextra -W
)

gtest_discover_tests(outcurses-tester)
enable_testing()

configure_file(tools/outcurses.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/outcurses.pc
  @ONLY
)

include(CMakePackageConfigHelpers)

configure_package_config_file(tools/OutcursesConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/OutcursesConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/Outcurses/cmake
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/OutcursesConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/OutcursesConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/OutcursesConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Outcurses
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/outcurses.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(TARGETS outcurses-demo DESTINATION bin)
install(TARGETS outcurses
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Development
)
