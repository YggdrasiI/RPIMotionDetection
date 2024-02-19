cmake_minimum_required(VERSION 2.8)
project( RPiMotionDetection )
#find_package( OpenCV )
#find_package( OpenGLES REQUIRED )

option(WITH_RPI "Enable this option if you compiling on the Raspberry Pi." ON)
option(WITH_OPENGL "Required for most apps on the Raspberry Pi." ON)

option(WITH_OCV "Enable this option if OpenCV library is available. Required for 'DisplayBlob' application." OFF)
option(WITH_QT "Allow OpenCV commands with QT dependencies (toggle buttons). Required for 'DisplayBlob' application." OFF)

#Set variables for options because options are not availabe in subdirectories.
set(WITH_RPI ${WITH_RPI} CACHE BOOL "Native libs of rpi available.")
set(WITH_OPENGL ${WITH_OPENGL} CACHE BOOL "OpenGL libs available")
set(WITH_OCV ${WITH_OCV} CACHE BOOL "OpenCV libs available")

option(WITH_GSL "Gnu Sientific Library. Required for gesture recognition" ON)

include_directories(${CMAKE_SOURCE_DIR}/3rdparty/blobdetection/include/blobdetection)
include_directories(${CMAKE_SOURCE_DIR}/3rdparty/blobdetection/src)
link_directories(${CMAKE_SOURCE_DIR}/3rdparty/blobdetection/build/src)
# TODO shift link dir to 'make' target dir of blobdetection library

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/raspicam)

# Use newer c++ standard (for shared pointers)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x" )

#Create symbolic link to assets
execute_process( COMMAND "ln" "-s" "${CMAKE_CURRENT_SOURCE_DIR}/images" "."
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	ERROR_QUIET
	)

### Create libs
add_subdirectory(libs)

### Create applications ###
add_subdirectory(apps)
