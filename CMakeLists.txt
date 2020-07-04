cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.15)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.15)
endif()

project(midiglue VERSION 0.1
                  DESCRIPTION "midiglue"
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

add_executable (midiglue "${PROJECT_SOURCE_DIR}/src/main.cpp")

add_library(midiglue ${ROOT_SOURCE})
target_link_libraries(midiglue Threads::Threads)
target_link_libraries(midiglue ${YAML_CPP_LIBRARIES})
# target_link_libraries (midiglue ${LIBLO_LIBRARY})
target_link_libraries (midiglue "/usr/lib/x86_64-linux-gnu/liblo.a")
if(ALSA_FOUND)
  target_compile_definitions(midiglue PUBLIC __LINUX_ALSA__)
  target_link_libraries (midiglue ${ALSA_LIBRARY})
endif()

install(TARGETS midiglue
        RUNTIME DESTINATION bin)