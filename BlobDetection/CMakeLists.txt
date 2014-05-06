cmake_minimum_required(VERSION 2.8)
project(Bloblib)

set(THRESH_SOURCES blob.c threshtree.c tree.c )
add_library(threshtree SHARED ${THRESH_SOURCES} )

set(DEPTH_SOURCES blob.c depthtree.c tree.c )
add_library(depthtree SHARED ${DEPTH_SOURCES} )

add_executable(example1 example1.c )
target_link_libraries(example1 threshtree ) 

add_executable( example2 example2.c )
target_link_libraries(example2 depthtree ) 


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fpic -O3" )
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -g -O0 -fmax-errors=3 -w" )

install(TARGETS threshtree depthtree example1
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	)
