cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.15)
endif()

project(midigyver VERSION 0.1
                  DESCRIPTION "midigyver"
                  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads)
find_package(ALSA)
find_package(yaml-cpp REQUIRED)
# find_package(liblo REQUIRED)

file(GLOB ROOT_SOURCE "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB OPS_SOURCE "${PROJECT_SOURCE_DIR}/src/ops/*.cpp")

# add_subdirectory(deps) 
set(DEPS_FOLDER "${PROJECT_SOURCE_DIR}/deps")
file(GLOB DUK_SOURCE "${PROJECT_SOURCE_DIR}/deps/duktape/*.c")
file(GLOB MIDI_SOURCE "${PROJECT_SOURCE_DIR}/deps/rtmidi/*.cpp")
set_source_files_properties(${DUK_SOURCE} PROPERTIES LANGUAGE CXX)
add_definitions(-DDUK_OPT_CPP_EXCEPTIONS)

add_executable (midigyver ${DUK_SOURCE} ${MIDI_SOURCE} ${OPS_SOURCE} ${ROOT_SOURCE})

target_include_directories(midigyver PUBLIC ${DEPS_FOLDER})
target_link_libraries(midigyver Threads::Threads)
target_link_libraries(midigyver ${YAML_CPP_LIBRARIES})

# target_link_libraries (midigyver ${LIBLO_LIBRARY})
target_link_libraries (midigyver "/usr/lib/x86_64-linux-gnu/liblo.a")
if(ALSA_FOUND)
  target_compile_definitions(midigyver PUBLIC __LINUX_ALSA__)
  target_link_libraries (midigyver ${ALSA_LIBRARY})
endif()

install(TARGETS midigyver
        RUNTIME DESTINATION bin)