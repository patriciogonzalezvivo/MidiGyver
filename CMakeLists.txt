cmake_minimum_required(VERSION 3.1)

set(VERSION 0.4)
project(midigyver VERSION ${VERSION}
                  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(deps)

file(GLOB ROOT_SOURCE "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB OPS_SOURCE "${PROJECT_SOURCE_DIR}/src/ops/*.cpp")

add_executable(midigyver
    ${OPS_SOURCE}
    ${ROOT_SOURCE}
)

target_include_directories(midigyver PRIVATE
    deps
)


target_link_libraries(midigyver PRIVATE
    yaml-cpp
    duktape
    rtmidi
    lo_static
)


install(TARGETS midigyver
        RUNTIME DESTINATION bin)

# set(CPACK_GENERATOR "DEB")
# set(CPACK_PACKAGE_CONTACT "Patricio Gonzalez Vivo <patriciogonzalezvivo@gmail.com>")
# set(CPACK_PACKAGE_NAME "midigyver")
# set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A bridge between MIDI to OSC")
# set(CPACK_PACKAGE_VENDOR https://github.com/patriciogonzalezvivo/midigyver)
# SET(CPACK_PACKAGE_VERSION ${VERSION})

# set(DEBSRC_BUILD_DEPENDS debhelper git cmake)
# set(DEBSRC_PACKAGE_DEPENDS git cmake)

# set(CPACK_DEBIAN_PACKAGE_DEPENDS)
# foreach(DEP ${DEBSRC_PACKAGE_DEPENDS})
# set(CPACK_DEBIAN_PACKAGE_DEPEND "${CPACK_DEBIAN_PACKAGE_DEPENDS}, ${DEP}")
# endforeach(DEP ${DEBSRC_PACKAGE_DEPENDS})  

# set(CPACK_DEBIAN_BUILD_DEPENDS)
# foreach(DEP ${DEBSRC_BUILD_DEPENDS})
# set(CPACK_DEBIAN_BUILD_DEPENDS "${CPACK_DEBIAN_BUILD_DEPENDS}, ${DEP}")
# endforeach(DEP ${DEBSRC_BUILD_DEPENDS})

# execute_process(
#     COMMAND dpkg --print-architecture
#     OUTPUT_VARIABLE DEB_ARCHITECTURE 
#     OUTPUT_STRIP_TRAILING_WHITESPACE
#     )

# message(STATUS "architecture is: ${DEB_ARCHITECTURE}")
# set(DEB_ARCH ${DEB_ARCHITECTURE})
# set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${DEB_ARCH})
# message(STATUS "architecture is: ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
# set(CPACK_DEBIAN_PACKAGE_PRIORITY optional)
# SET(CPACK_PACKAGE_VERSION ${VERSION})
# set(CPACK_DEBIAN_DISTRIBUTION_NAME ubuntu)
# set(CPACK_DEBIAN_PACKAGE_SECTION "multimedia")
    
# set(CPACK_DEBIAN_DISTRIBUTION_RELEASES Groovy Focal Eoan Bionic) 

# message(STATUS " CPACK_DEBIAN_DISTRIBUTION_RELEASES : ${CPACK_DEBIAN_DISTRIBUTION_RELEASES}")

# if(${SRC_DIR} MATCHES "")
#     set(CPACK_RESOURCE_FILE_LICENSE ${SRC_DIR}/deb/LICENSE)
#     set(DEB_SRC_DIR ${SRC_DIR})
# else(${SRC_DIR} MATCHES "")
#     set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
#     set(DEB_SRC_DIR ${CMAKE_SOURCE_DIR})
# endif(${SRC_DIR} MATCHES "")

# set(CPACK_DEBIAN_CHANGELOG "  * Latest development version.")

# INCLUDE(CPack)
