# modified raspivid

add_definitions(-DWITH_OPENGL)
SET(COMPILE_DEFINITIONS -Werror)

#(we dont need opencv in this app)
#target_link_libraries( raspivid ${OpenCV_LIBS} )
remove_definitions(-DWITH_OPENCV)

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/apps/raspicam/)

link_directories(/opt/vc/lib)

set (GL_SCENE_SOURCES
   gl_scenes/models.c
   gl_scenes/mirror.c
   gl_scenes/yuv.c
   gl_scenes/sobel.c
   gl_scenes/square.c
   gl_scenes/teapot.c
   gl_scenes/motion.c
   gl_scenes/pong.c
	 )

set (COMMON_SOURCES
   RaspiCamControl.c
   RaspiCLI.c
   RaspiPreview.c)

 add_executable(raspivid   ${COMMON_SOURCES} RaspiVid.c RaspiTex.c RaspiTexUtil.c tga.c
	 ${GL_SCENE_SOURCES} RaspiImv.c
	 main.cpp 
	 ../../Tracker2.cpp ../../Tracker.cpp 
	 norm2.c
	 DrawingFunctions.cpp
	 GfxProgram.cpp
	 Graphics.cpp 
	 lodepng.cpp
	 )

set (MMAL_LIBS mmal_core mmal_util mmal_vc_client pthread)

target_link_libraries(raspivid   ${MMAL_LIBS} vcos bcm_host GLESv2 EGL m
	${DEPTHLIB}
	)

# Create link to simplify paths
message(STATUS "Add symbolic link to shader directory\n From: ${CMAKE_CURRENT_SOURCE_DIR}/shader \nTo: ${CMAKE_CURRENT_BINARY_DIR}/shader" )
execute_process( COMMAND "ln" "-s" "${CMAKE_CURRENT_SOURCE_DIR}/shader" "."
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)

#Create symbolic link to start script
execute_process( COMMAND "ln" "-s" "${CMAKE_CURRENT_SOURCE_DIR}/start" "."
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)

install(TARGETS raspivid RUNTIME DESTINATION bin)
