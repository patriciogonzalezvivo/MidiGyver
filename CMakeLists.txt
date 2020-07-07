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


if(POLICY CMP0002)
cmake_policy(SET CMP0002 OLD)
endif(POLICY CMP0002)

find_package(Threads)
find_package(ALSA)
find_package(yaml-cpp REQUIRED)
# find_package(liblo REQUIRED)

file(GLOB ROOT_HEADER "${PROJECT_SOURCE_DIR}/src/*.h")
file(GLOB ROOT_SOURCE "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable (midigyver "${PROJECT_SOURCE_DIR}/src/main.cpp")

add_library(midigyver ${ROOT_SOURCE})
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