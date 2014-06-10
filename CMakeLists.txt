cmake_minimum_required(VERSION 2.8)
project( RPIMotionDetection )

find_package( OpenCV COMPONENTS )
#find_package( OpenGLES REQUIRED )

#Disable this option if not compiling on RPi.
option(WITH_RPI "Set this option on if you compiling on the Raspberry Pi." ON)
option(WITH_OCV "Enable this option if OpenCV library is available. Required for 'DisplayBlob' application." ON)
option(WITH_QT "Allow OpenCV commands with QT dependencies (toggle buttons). Required for 'DisplayBlob' application." OFF)

#Set variables for options because options are not availabe in subdirectories.
set(WITH_RPI ${WITH_RPI} CACHE BOOL "Native libs of rpi available.")
set(WITH_OCV ${WITH_OCV} CACHE BOOL "OpenCV libs available")

### 0. Compile BlobDetection Lib ###
# Compile blobdetection library in subdirectory
include_directories(${CMAKE_HOME_DIRECTORY}/BlobDetection)
execute_process( COMMAND "mkdir" "build"
	WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/BlobDetection"
	)
execute_process( COMMAND "${CMAKE_COMMAND}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" ".."
	WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/BlobDetection/build"
)
execute_process( COMMAND make
	WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/BlobDetection/build"
)

# Add blobdetection library
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_HOME_DIRECTORY}/BlobDetection/build )
find_library(THRESHLIB NAMES libthreshtree.so )
find_library(DEPTHLIB NAMES libdepthtree.so )
message(STATUS ${THRESHLIB})
message(STATUS ${DEPTHLIB})

# Use newer c++ standard (for shared pointers)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x" )


### 1. Create testing applications ###
add_subdirectory(apps)
