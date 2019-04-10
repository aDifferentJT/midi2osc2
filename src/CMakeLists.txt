cmake_minimum_required (VERSION 3.13)

set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(${TARGET_NAME} -Wall -Wextra -pedantic)

add_executable(midi2osc2 main.cpp mappings.cpp midi.cpp osc.cpp)

target_link_libraries(midi2osc2 pthread rtmidi)

