# set(CMAKE_C_FLAGS "")
# set(CMAKE_C_FLAGS_RELEASE "")
# set(CMAKE_C_FLAGS_DEBUG "")
# set(CMAKE_C_COMPILE_OBJECT "${target_compiler} -c <FLAGS> -o <OBJECT>")
# SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_C_COMPILER> ${SJR_LINUX_C_FLAGS} ${SJR_LINUX_LINK} -shared -fPIC -Wl,-soname=<TARGET> <OBJECTS> -o <TARGET> <LINK_LIBRARIES> <LINK_FLAGS>")
# set(CMAKE_C_ARCHIVE_FINISH ${CMAKE_C_ARCHIVE_FINISH} PARENT_SCOPE)
# set(CMAKE_CXX_ARCHIVE_CREATE ${CMAKE_CXX_ARCHIVE_CREATE} PARENT_SCOPE)
# set(CMAKE_C_ARCHIVE_CREATE ${CMAKE_C_ARCHIVE_CREATE} PARENT_SCOPE)

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

## Liblo
##########################
add_subdirectory(liblo/cmake)