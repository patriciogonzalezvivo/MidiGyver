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

add_subdirectory(deps)

# find_package(Threads)
# find_package(ALSA)

file(GLOB ROOT_SOURCE "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB OPS_SOURCE "${PROJECT_SOURCE_DIR}/src/ops/*.cpp")

add_executable(midigyver
    ${OPS_SOURCE}
    ${ROOT_SOURCE}
)

target_include_directories(midigyver 
    PUBLIC 
    deps
)

target_link_libraries(midigyver 
    # Threads::Threads
    yaml-cpp
    duktape
    rtmidi
    lo_static
)

install(TARGETS midigyver
        RUNTIME DESTINATION bin)