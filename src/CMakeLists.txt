cmake_minimum_required(VERSION 2.6)

#find_package(LibEvent REQUIRED)
#include_directories(${LIBEVENT_INCLUDE_DIR})

include_directories(include)

file (GLOB
     source_files
     *.c)

add_library(evaio SHARED ${source_files})

target_link_libraries(evaio event rt)

set_target_properties(evaio PROPERTIES VERSION 0.1 SOVERSION 1)

install(TARGETS evaio
                 LIBRARY DESTINATION lib)

install(FILES evaio.h
                 DESTINATION include)
