cmake_minimum_required(VERSION 2.8)
project( DisplayBlobs )
find_package( OpenCV REQUIRED )
#find_package( OpenGLES REQUIRED )

#Disable this option if not compiling on RPi.
option(WITH_RPI ON)
option(WITH_OPENCV ON)




### 0. Compile BlobDetection Lib ###
# Compile blobdetection library in subdirectory
execute_process( COMMAND "mkdir" "build"
	WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/BlobDetection"
	)
execute_process( COMMAND "${CMAKE_COMMAND}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" ".."
	WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/BlobDetection/build"
)
execute_process( COMMAND "${CMAKE_COMMAND}" "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}" ".."
	WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/BlobDetection/build"
)

# Add blobdetection library
include_directories(${CMAKE_HOME_DIRECTORY}/BlobDetection)
#set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_HOME_DIRECTORY}/BlobDetection/build )
find_library(THRESHLIB NAMES libthreshtree.so )
find_library(DEPTHLIB NAMES libdepthtree.so )
message(STATUS ${THRESHLIB})
message(STATUS ${DEPTHLIB})



### 1. Test application for BlobDetection library ###
# The images directory contains a few images 
# as examples.
if(WITH_OPENCV)

add_executable( DisplayBlobs DisplayBlobs.cpp Tracker.cpp Tracker2.cpp )
add_definitions(-DWITH_OPENCV)
target_link_libraries( DisplayBlobs
	${THRESHLIB}
	${DEPTHLIB}
	${OpenCV_LIBS} 
	)

# Create link to simplify paths
execute_process( COMMAND "ln" "-s" "${CMAKE_HOME_DIRECTORY}/images" "images"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)

# Rebuild BlobDetection library if neccessary.
add_custom_command(TARGET DisplayBlobs
	PRE_BUILD
	COMMAND make
	WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/BlobDetection/build )

endif(WITH_OPENCV)


### 2. Create some other testing applications ###
add_subdirectory(apps)
