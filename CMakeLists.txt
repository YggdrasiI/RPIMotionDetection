cmake_minimum_required(VERSION 2.8)
project( DisplayBlobs )
find_package( OpenCV REQUIRED )
#find_package( OpenGLES REQUIRED )

#Disable this option if not compiling on RPi.
option(WITH_RPI "Set this option on if you compiling on the Raspberry Pi." ON)
option(WITH_OCV "Enable this option if OpenCV library is available. Required for 'DisplayBlob' application." ON)

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



### 1. Test application for BlobDetection library ###
# The images directory contains a few images 
# as examples.
if(WITH_OCV)

add_executable( DisplayBlobs DisplayBlobs.cpp Tracker.cpp Tracker2.cpp )
#add_definitions(-DWITH_OCV)
target_link_libraries( DisplayBlobs
	${THRESHLIB}
	${DEPTHLIB}
	${OpenCV_LIBS} 
	)

set_target_properties( DisplayBlobs PROPERTIES
	COMPILE_FLAGS -DWITH_OCV
	)

# Create link to simplify paths
execute_process( COMMAND "ln" "-s" "${CMAKE_HOME_DIRECTORY}/images" "."
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)

# Rebuild BlobDetection library if neccessary.
add_custom_command(TARGET DisplayBlobs
	PRE_BUILD
	COMMAND make
	WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/BlobDetection/build )
else(WITH_OCV)
	message(STATUS "No OpenCV lib. Skip DisplayBlob compiliation." )
endif(WITH_OCV)



### 2. Create some other testing applications ###
add_subdirectory(apps)
