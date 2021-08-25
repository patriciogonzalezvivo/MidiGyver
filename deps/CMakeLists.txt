## yaml-cpp ##
##############
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "")
set(YAML_CPP_INSTALL OFF CACHE BOOL "")
add_subdirectory(yaml-cpp)

target_include_directories(yaml-cpp PUBLIC yaml-cpp/include)

## duktape ##
#############
add_subdirectory(duktape)

## RTMidi
##########################
add_subdirectory(rtmidi)

## MidiFile
##########################
add_subdirectory(midifile)

## Liblo
##########################
add_subdirectory(liblo/cmake)