cmake_minimum_required(VERSION 2.8)
project( DisplayBlobs )
find_package( OpenCV REQUIRED )
add_executable( DisplayBlobs DisplayBlobs.cpp )
target_link_libraries( DisplayBlobs ${OpenCV_LIBS} )

# Create link to simplify paths
execute_process( COMMAND "ln" "-s" "${CMAKE_HOME_DIRECTORY}/images" "images"
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)

# Compile blobdetection library in subdirectory
execute_process( COMMAND "mkdir" "BlobDetection/build"
	WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"	
	)
execute_process( COMMAND "${CMAKE_COMMAND}" "-DCMAKE_BUILD_TYPE=Release" ".."
	COMMAND make
	WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/BlobDetection/build"
#	OUTPUT QUIET
)

# Add blobdetection library
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_HOME_DIRECTORY}/BlobDetection/build )
find_library(THRESHLIB NAMES libthreshtree.so )
find_library(DEPTHLIB NAMES libdepthtree.so )
message(STATUS ${THRESHLIB})
message(STATUS ${DEPTHLIB})

include_directories(${CMAKE_HOME_DIRECTORY}/BlobDetection)
target_link_libraries( DisplayBlobs 
	${THRESHLIB}
	${DEPTHLIB} 
	)

