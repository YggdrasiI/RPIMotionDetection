# modified raspivid

add_definitions(-DWITH_OPENGL)
SET(COMPILE_DEFINITIONS -Werror)

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host/linux)

include_directories(${CMAKE_SOURCE_DIR})
#include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/raspicam/)

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
	RaspiPreview.c
	)

add_library(raspivid_core
	${COMMON_SOURCES} 
	${GL_SCENE_SOURCES}
	RaspiImv.c
	RaspiVid.c RaspiTex.c RaspiTexUtil.c 
	tga.c lodepng.cpp
	norm2.c
	DrawingFunctions.cpp
	GfxProgram.cpp
	)

set (MMAL_LIBS mmal_core mmal_util mmal_vc_client pthread)

target_link_libraries(raspivid_core ${MMAL_LIBS} vcos bcm_host GLESv2 EGL m)

#install(TARGETS raspivid_core LIBRARY DESTINATION lib)
