add_subdirectory(blobdetection)
add_subdirectory(tracker)
if(WITH_RPI)
	add_subdirectory(raspicam)
	add_subdirectory(freetypeGlesRpi)
endif(WITH_RPI)

