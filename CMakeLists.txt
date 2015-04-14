cmake_minimum_required(VERSION 2.8)
project( RPiMotionDetection )
find_package( OpenCV REQUIRED )
#find_package( OpenGLES REQUIRED )

#Disable this option if not compiling on RPi.
option(WITH_RPI "Set this option on if you compiling on the Raspberry Pi." ON)
option(WITH_OPENGL "Required for most apps on the Raspberry Pi." ON)
option(WITH_OCV "Enable this option if OpenCV library is available. Required for 'DisplayBlob' application." ON)
option(WITH_QT "Allow OpenCV commands with QT dependencies (toggle buttons). Required for 'DisplayBlob' application." OFF)

#Set variables for options because options are not availabe in subdirectories.
set(WITH_RPI ${WITH_RPI} CACHE BOOL "Native libs of rpi available.")
set(WITH_OPENGL ${WITH_OPENGL} CACHE BOOL "OpenGL libs available")
set(WITH_OCV ${WITH_OCV} CACHE BOOL "OpenCV libs available")

include_directories(${CMAKE_HOME_DIRECTORY}/include)
include_directories(${CMAKE_HOME_DIRECTORY}/libs/blobdetection)
include_directories(${CMAKE_HOME_DIRECTORY}/libs/raspicam)

# Use newer c++ standard (for shared pointers)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x" )

### Create libs
add_subdirectory(libs)

### Test application for BlobDetection library ###
# The images directory contains a few images 
# as examples.
if(WITH_OCV)
	add_subdirectory(apps/displayblobs)
else(WITH_OCV)
	message(STATUS "No OpenCV lib. Skip DisplayBlob application." )
endif(WITH_OCV)


### Create other testing applications ###
add_subdirectory(apps)
