cmake_minimum_required(VERSION 3.15)
project(Outcurses VERSION 0.0.1
                  DESCRIPTION "High-level UI via ncurses"
                  LANGUAGES C)
add_library(outcurses SHARED fade.c)
