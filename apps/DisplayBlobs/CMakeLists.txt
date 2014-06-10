### Test application for BlobDetection library ###
# The images directory contains a few images 
# as examples.

add_definitions(-DWITH_OCV)
add_definitions(-DWITH_GSL)

#Allow OpenCV commands with QT dependencies. Comment out for non-QT-Version
if(WITH_QT)
add_definitions(-DWITH_QT)
endif(WITH_QT)

include_directories(${CMAKE_SOURCE_DIR})

add_executable( DisplayBlobs 
	DisplayBlobs.cpp
	../../Tracker.cpp ../../Tracker2.cpp
	../../Gestures.cpp ../../gsl_helper.c
	)

target_link_libraries( DisplayBlobs
	${THRESHLIB}
	${DEPTHLIB}
	${OpenCV_LIBS} 
	gsl gslcblas m
	)

set_target_properties( DisplayBlobs PROPERTIES COMPILE_FLAGS -DWITH_GSL)
#set_target_properties( DisplayBlobs PROPERTIES COMPILE_FLAGS -DWITH_OCV)

# Create link to simplify paths
execute_process( COMMAND "ln" "-s" "${CMAKE_HOME_DIRECTORY}/images" "."
	WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)

install(TARGETS DisplayBlobs RUNTIME DESTINATION bin)
