
if(WITH_RPI)
 message(STATUS "Include native apps.")
	#add_subdirectory(test1)
add_subdirectory(raspicam)
else(WITH_RPI)
	message(STATUS "Skipping native apps.")
endif(WITH_RPI)
