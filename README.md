This package contains programs which analyze the 
motion vections from the h264 coarse motion estimation, see
http://www.raspberrypi.org/vectors-from-coarse-motion-estimation/


Applications:

1. The program DisplayBlobs shows the usage of the BlobDetection
library. Called without any arguments it shows blobs from the
images folder and allows the off line analysis of images
to find good parameter sets for the detection.
Notes:
• Requires OpenCV
• Execute: ./DisplayBlobs [algorithm number] [imagepath] [thresh] [stepwidth]
		- algorithm number should be 0 or 1.
• Adding 1 as sixth argument starts a performance test. (Do not forget
	to compile to activate the relase/optimization flags for this)

2. Raspivid (see apps/raspivid/ ) is a modification of the official
raspivid applications. It was extended by the OpenGL functionalities 
of raspistill. The output of founded blobs is realized as OpenGL texture,
thus be sure to activate the OpenGL output if you start the application.
Notes:
• This app is currently only in an EXPERIMENTAL phase! 
• Requires OpenGLES 2.0
• Example set of arguments:
	./raspivid -o /dev/null -x /dev/null -t 0 --preview '0,0,1280,720' \
		--gl --glwin '0,0,800,600' --glscene motion 


Compiling:

	mkdir bulid
	cd build
	cmake [-DCMAKE_BUILD_TYPE=Release] ..
	make

	If you compile on the RPi: Please note that the compiling 
	of the release version (-O3) requires a big amount of memory.
  Reduce the memory for the gpu at 64MB (config.txt) or enable the
	swapping file (very slow).

	If you compile on other systems: Add the cmake flag
	-DWITH_RPI=0
	to disable the parts which requires RPi dependecies.


