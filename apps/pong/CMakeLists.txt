# modified raspivid

add_definitions(-DWITH_OPENGL)
SET(COMPILE_DEFINITIONS -Werror)

SET(RELDIR "../raspicam")

#(we dont need opencv in this app)
#target_link_libraries( pong ${OpenCV_LIBS} )
remove_definitions(-DWITH_OPENCV)

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

include_directories(${CMAKE_SOURCE_DIR}/apps/pong/)
include_directories(${CMAKE_SOURCE_DIR}/apps/raspicam/)
include_directories(${CMAKE_SOURCE_DIR})

link_directories(/opt/vc/lib)

set (GL_SCENE_SOURCES
	${RELDIR}/gl_scenes/models.c
	${RELDIR}/gl_scenes/mirror.c
	${RELDIR}/gl_scenes/yuv.c
	${RELDIR}/gl_scenes/sobel.c
	${RELDIR}/gl_scenes/square.c
	${RELDIR}/gl_scenes/teapot.c
	${RELDIR}/gl_scenes/motion.c
	${RELDIR}/gl_scenes/pong.c
	)

set (COMMON_SOURCES
	${RELDIR}/RaspiCamControl.c
	${RELDIR}/RaspiCLI.c
	${RELDIR}/RaspiPreview.c
	${RELDIR}/RaspiVid.c
	${RELDIR}/RaspiTex.c
	${RELDIR}/RaspiTexUtil.c
	${RELDIR}/tga.c
	${RELDIR}/RaspiImv.c
	${RELDIR}/lodepng.cpp
	${RELDIR}/norm2.c
	${RELDIR}/DrawingFunctions.cpp
	${RELDIR}/GfxProgram.cpp
	../../Tracker2.cpp ../../Tracker.cpp 
	)

 add_executable(pong
	 ${COMMON_SOURCES} 
	 ${GL_SCENE_SOURCES} 
	 main.cpp 
	 Pong.cpp
	 Graphics.cpp 
	 )

set (MMAL_LIBS mmal_core mmal_util mmal_vc_client)

target_link_libraries(pong   ${MMAL_LIBS} vcos bcm_host GLESv2 EGL m
	${DEPTHLIB}
	)

# Create link to simplify paths
message(STATUS "Add symbolic link to shader directory\n From: ${CMAKE_CURRENT_SOURCE_DIR}/../raspicam/shader \nTo: ${CMAKE_CURRENT_BINARY_DIR}/shader" )
execute_process( COMMAND "ln" "-s" "${CMAKE_CURRENT_SOURCE_DIR}/../raspicam/shader" "."
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)

#Create symbolic link to start script
execute_process( COMMAND "ln" "-s" "${CMAKE_CURRENT_SOURCE_DIR}/start" "."
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
	)

install(TARGETS pong RUNTIME DESTINATION bin)
