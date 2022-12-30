cmake_minimum_required(VERSION 3.20)

string(FIND ${CMAKE_CURRENT_SOURCE_DIR} "/" SrcDirNamePos REVERSE)
string(LENGTH ${CMAKE_CURRENT_SOURCE_DIR} SrcDirNameLength)
string(SUBSTRING ${CMAKE_CURRENT_SOURCE_DIR} ${SrcDirNamePos} ${SrcDirNameLength} BinaryName)
string(REPLACE "/" "" BinaryName ${BinaryName})

project(${BinaryName}) #Project name

set(CMAKE_CXX_STANDARD 14) #setting C++ 14 standard
find_package(PkgConfig) #finding pkg-config is a helper tool
find_package(OpenCV REQUIRED)

#using pkg-config to getting Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)

#including GStreamer header files directory
include_directories(
        ${GLIB_INCLUDE_DIRS}
        ${GSTREAMER_INCLUDE_DIRS}
)

#linking GStreamer library directory
link_directories(
        ${GLIB_LIBRARY_DIRS}
        ${GSTREAMER_LIBRARY_DIRS}
)

#building target executable
add_executable(${PROJECT_NAME} main.cpp)

#linking Gstreamer library with target executable
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${GSTREAMER_LIBRARIES} gstapp-1.0)


