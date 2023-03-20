cmake_minimum_required(VERSION 3.0)
project(RSP3_tcp CXX)
########################################################################
# Setup library
########################################################################

########################################################################
# Setup shared library variant
########################################################################

########################################################################
# Setup static library variant
########################################################################

########################################################################
# Set up Windows DLL resource files
########################################################################

########################################################################
# Set link library
########################################################################

########################################################################
# Setup sources used in executable
########################################################################
########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include_sdrplay
)

message(STATUS "CMAKE_SOURCE_DIR- ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR- ${CMAKE_CURRENT_BINARY_DIR}")

########################################################################
# Build utility
########################################################################
add_executable(RSP3_tcp
    RSP3_tcp.cpp
    common.cpp
    controlThread.cpp
    crc32.cpp
    devices.cpp
    IPAddress.cpp
    MeasTimeDiff.cpp
    receiveThread.cpp
    rsp_cmdLineArgs.cpp
    sdrplay_device.cpp
    sendThread.cpp
    sdrGainTable.cpp
)

if(UNIX)
target_link_libraries(RSP3_tcp ${LIBSDRPLAY_LIBRARIES} Threads::Threads)
endif()

if(WIN32)
if(MSVC)
target_link_libraries(RSP3_tcp ws2_32
    ${LIB64_DIR}/sdrplay_api.lib
    ${LIB64_DIR}/pthreadVC2.lib
)
endif()
endif()

########################################################################
# Install built library files & utilities
########################################################################
#install(TARGETS ${INSTALL_TARGETS}
#    LIBRARY DESTINATION ${LIB_INSTALL_DIR} # .so/.dylib file
#    ARCHIVE DESTINATION ${LIB_INSTALL_DIR} # .lib file
#     RUNTIME DESTINATION bin              # .dll file
#)
#INSTALL(
#    FILES ${CMAKE_CURRENT_BINARY_DIR}
#    DESTINATION ${RELEASE_INSTALL_DIR}
#)
