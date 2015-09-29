# modified raspivid

add_definitions(-DWITH_OPENGL)
SET(COMPILE_DEFINITIONS -Werror)

add_executable(pong
	Pong.cpp
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
#include_directories(${CMAKE_SOURCE_DIR}/apps/pong/)
#include_directories(${CMAKE_SOURCE_DIR}/apps/raspicam/)

include_directories(${CMAKE_SOURCE_DIR}/libs/raspicam/)
include_directories(${CMAKE_SOURCE_DIR}/libs/blobdetection/)
include_directories(${CMAKE_SOURCE_DIR}/libs/tracker/)
include_directories(${CMAKE_SOURCE_DIR}/libs/freetypeGlesRpi/)

link_directories(/opt/vc/lib)

target_link_libraries(pong
	raspivid_core
	tracker
	depthtree
  freetypeGlesRpi
	${MMAL_LIBS} vcos bcm_host GLESv2 EGL m
	)

install(TARGETS pong RUNTIME DESTINATION bin)

# Create link to simplify paths
message(STATUS "Add symbolic link to shader directory\n From: ${CMAKE_SOURCE_DIR}/shader \nTo: ${CMAKE_CURRENT_BINARY_DIR}/shader" )
execute_process( COMMAND "ln" "-s" "${CMAKE_SOURCE_DIR}/shader" "."
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)

#Create symbolic link to start script
execute_process( COMMAND "ln" "-s" "${CMAKE_CURRENT_SOURCE_DIR}/start" "."
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)

