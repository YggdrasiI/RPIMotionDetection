if(WITH_RPI)
	message(STATUS "Include native apps.")
	set (MMAL_LIBS mmal_core mmal_util mmal_vc_client pthread)
	link_directories(/opt/vc/lib)

	if(WITH_OPENGL)
		add_subdirectory(raspicam)
		add_subdirectory(pong)
		add_subdirectory(gestures)
	endif(WITH_OPENGL)
else(WITH_RPI)
	message(STATUS "Skipping native apps.")
endif(WITH_RPI)

### Test application for BlobDetection library ###
# The images directory contains a few images 
# as examples.
if(WITH_OCV)
	add_subdirectory(displayblobs)
else(WITH_OCV)
	message(STATUS "No OpenCV lib. Skip DisplayBlob application." )
endif(WITH_OCV)

