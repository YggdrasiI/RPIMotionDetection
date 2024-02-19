if(WITH_RPI)
	message(STATUS "Include native apps.")
	set (MMAL_LIBS mmal_core mmal_util mmal_vc_client pthread)
	link_directories(/opt/vc/lib)

	if(WITH_OPENGL)
		add_subdirectory(raspicam)
		add_subdirectory(pong)
		add_subdirectory(estimateDirection)
		if(WITH_GSL)
			add_subdirectory(gestures)
		else(WITH_GSL)
			message(STATUS "Skipping gestures app. WITH_GSL is ${WITH_GSL}.")
		endif(WITH_GSL)
	endif(WITH_OPENGL)
else(WITH_RPI)
	message(STATUS "Skipping native apps. WITH_RPI is ${WITH_RPI}.")
endif(WITH_RPI)

### Test application for BlobDetection library ###
# The images directory contains a few images 
# as examples.
if(WITH_OCV)
	add_subdirectory(displayblobs)
else(WITH_OCV)
	message(STATUS "No OpenCV lib. Skip displayblobs application." )
endif(WITH_OCV)

