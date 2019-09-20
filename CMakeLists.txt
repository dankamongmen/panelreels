cmake_minimum_required(VERSION 3.15)
project(Outcurses VERSION 0.0.1
                  DESCRIPTION "High-level UI via ncurses"
                  LANGUAGES C)
set(CMAKE_C_STANDARD 11)

add_library(outcurses SHARED fade.c)
set(CURSES_NEED_WIDE TRUE)
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
