cmake_minimum_required(VERSION 2.8)

project(dec)

set (SOURCE_FILES 
    dec.cpp
)

include_directories (dec /usr/local/include)
include_directories (dec /opt/intel/mediasdk/include/mfx)

link_directories(/opt/intel/mediasdk/lib/)

add_executable(dec ${SOURCE_FILES})
target_link_libraries(dec mfx)