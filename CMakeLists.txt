cmake_minimum_required(VERSION 2.8)
project( DisplayImage )
find_package( OpenCV REQUIRED )
add_executable( DisplayImage DisplayImage.cpp )
target_link_libraries( DisplayImage ${OpenCV_LIBS} )


set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/kinect/MyBlobDetection )
find_library(BLOBLIB NAMES libblobtree.so )
find_library(BLOBLIBb NAMES libdepthtree.so )
message(STATUS ${BLOBLIB})
message(STATUS ${BLOBLIBb})

include_directories(/opt/kinect/MyBlobDetection)
target_link_libraries( DisplayImage 
	${BLOBLIB}
	#	${BLOBLIBb} 
	)
