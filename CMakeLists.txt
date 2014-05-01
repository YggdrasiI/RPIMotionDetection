cmake_minimum_required(VERSION 2.8)
project( DisplayImage )
find_package( OpenCV REQUIRED )
add_executable( DisplayImage DisplayImage.cpp )
target_link_libraries( DisplayImage ${OpenCV_LIBS} )


set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/kinect/MyBlobDetection/build )
find_library(THRESHLIB NAMES libthreshtree.so )
find_library(DEPTHLIB NAMES libdepthtree.so )
message(STATUS ${THRESHLIB})
message(STATUS ${DEPTHLIB})

include_directories(/opt/kinect/MyBlobDetection)
target_link_libraries( DisplayImage 
	${THRESHLIB}
	${DEPTHLIB} 
	)
