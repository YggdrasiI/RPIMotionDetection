Program to experiment offline with images created by
the h264 motion estimation, see
http://www.raspberrypi.org/vectors-from-coarse-motion-estimation/


Requires:
• OpenCV
• Own Bloblib (See Subfolder ( libthreshtree.so, libdepthtree.so ))

Compiling:

	mkdir bulid
	cd build
	cmake [-DCMAKE_BUILD_TYPE=Release] ..
	make

	If you compile on the RPi: Please note that the compiling 
	of the release version (-O3) requires a big amount of memory.
  Reduce the memory for the gpu at 64MB (config.txt) or enable the
	swapping file (very slow).


Execute:
	./DisplayBlobs [algorithm number] [path] [thresh] [stepwidth]

		- algorithm number should be 0 or 1.

