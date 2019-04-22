cmake_minimum_required (VERSION 3.13)

set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "-std=c++2a")

add_compile_options(${TARGET_NAME} -Wall -Wextra -pedantic)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)
include_directories(websocketpp)
add_executable(midi2osc2 main.cpp mappings.cpp midi.cpp osc.cpp gui.cpp config.cpp)
target_link_libraries(midi2osc2 pthread rtmidi)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)
add_executable(midi_console midi_console.cpp)
target_link_libraries(midi_console pthread rtmidi)

