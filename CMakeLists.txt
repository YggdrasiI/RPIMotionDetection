cmake_minimum_required(VERSION 2.8)
project( DisplayBlobs )
find_package( OpenCV REQUIRED )
add_executable( DisplayBlobs DisplayBlobs.cpp Tracker.cpp Tracker2.cpp )
target_link_libraries( DisplayBlobs ${OpenCV_LIBS} )

# Create link to simplify paths
execute_process( COMMAND "ln" "-s" "${CMAKE_HOME_DIRECTORY}/images" "images"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)

# Compile blobdetection library in subdirectory
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

add_custom_command(TARGET DisplayBlobs
	PRE_BUILD
	COMMAND make
	WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY}/BlobDetection/build)


include_directories(${CMAKE_HOME_DIRECTORY}/BlobDetection)
target_link_libraries( DisplayBlobs
	${THRESHLIB}
	${DEPTHLIB}
	)

