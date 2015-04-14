# modified raspivid

add_definitions(-DWITH_OPENGL)
SET(COMPILE_DEFINITIONS -Werror)

add_executable(raspivid 
	main.cpp 
	Graphics.cpp 
	../../libs/tracker/DrawingOpenGL.cpp
	)

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

include_directories(${CMAKE_SOURCE_DIR}/libs/raspicam/)
include_directories(${CMAKE_SOURCE_DIR}/libs/blobdetection/)
include_directories(${CMAKE_SOURCE_DIR}/libs/tracker/)


target_link_libraries(raspivid
	raspivid_core
	tracker
	depthtree
	#	threshtree
	${MMAL_LIBS} vcos bcm_host GLESv2 EGL m
	)


install(TARGETS raspivid RUNTIME DESTINATION bin)

# Create link to simplify paths
message(STATUS "Add symbolic link to shader directory\n From: ${CMAKE_SOURCE_DIR}/shader \nTo: ${CMAKE_CURRENT_BINARY_DIR}/shader" )
execute_process( COMMAND "ln" "-s" "${CMAKE_SOURCE_DIR}/shader" "."
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)

#Create symbolic link to start script
execute_process( COMMAND "ln" "-s" "${CMAKE_CURRENT_SOURCE_DIR}/start" "."
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)

