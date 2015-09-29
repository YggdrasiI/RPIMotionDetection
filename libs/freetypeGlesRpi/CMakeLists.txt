
include_directories(/opt/vc/include)
include_directories(/opt/vc/include/interface/vcos)
include_directories(/opt/vc/include/interface/vcos/pthreads)
include_directories(/opt/vc/include/interface/vmcs_host/linux)
link_directories(/opt/vc/lib)

include_directories(/usr/include/freetype2)

set(FREETYPE_GLES_SOURCES texture-atlas.c texture-font.c vector.c )

add_library(freetypeGlesRpi SHARED ${FREETYPE_GLES_SOURCES} )
target_link_libraries(freetypeGlesRpi bcm_host GLESv2 EGL m freetype)
#target_link_libraries(freetypeGlesRpi png)

install(TARGETS freetypeGlesRpi LIBRARY DESTINATION lib)

# Extraction of original Makefile:
#FLAGS=-D__FOR_RPi_noX__ -c  `pkg-config libpng --cflags` -I/usr/include/freetype2 -Isrc -Itest
#FLAGS+= -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads/
#LIBS=-lGLESv2 -lEGL -lm -lbcm_host -lfreetype -L/opt/vc/lib -L/usr/lib/arm-linux-gnueabihf `pkg-co    nfig libpng --libs`

