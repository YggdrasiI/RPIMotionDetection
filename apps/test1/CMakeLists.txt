cmake_minimum_required(VERSION 2.8)

include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host/linux)
include_directories(/home/olaf/software/userland/host_applications/linux/apps/raspicam)
link_directories(/opt/vc/lib)

add_executable(picamdemo picam.cpp camera.cpp cameracontrol.cpp util.cpp helper.cpp graphics.cpp )

target_link_libraries(picamdemo libmmal_core.so libmmal_util.so libmmal_vc_client.so libvcos.so libbcm_host.so GLESv2 EGL
	${OpenCV_LIBS}
	${THRESHLIB}
	)

