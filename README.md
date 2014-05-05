Program to experiment offline with images created by
the h264 motion estimation, see
http://www.raspberrypi.org/vectors-from-coarse-motion-estimation/


Requires:
• OpenCV
• Own Bloblib (See Subfolder ( libthreshtree.so, libdepthtree.so ))

Compiling:

	mkdir bulid
	cd build
	cmake ..
	make

Execute:
	./DisplayBlobs [path] [thresh]

