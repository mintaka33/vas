cmake_minimum_required(VERSION 3.4)

project(dec)

set (SOURCE_FILES 
    dec.cpp
)

include_directories (dec /usr/local/include)
include_directories (dec /opt/intel/mediasdk/include/mfx)

link_directories(/opt/intel/mediasdk/lib/)

add_executable(dec ${SOURCE_FILES})
target_link_libraries(dec mfx msdkutils X11 va va-drm pthread)