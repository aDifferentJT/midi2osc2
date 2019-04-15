cmake_minimum_required (VERSION 2.6)

project (midi2osc2)

include_directories(src)
add_subdirectory(src)

install (TARGETS midi2osc2 DESTINATION bin)

