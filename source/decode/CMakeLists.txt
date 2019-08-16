cmake_minimum_required(VERSION 3.4)

project(dec)

link_directories(/opt/intel/mediasdk/lib/)

include_directories (dec /usr/local/include)
include_directories (dec /opt/intel/mediasdk/include/mfx)
add_executable(dec dec.cpp)
target_link_libraries(dec mfx msdkutils X11 va va-drm pthread)

include_directories (dec_vm /usr/local/include)
include_directories (dec_vm /opt/intel/mediasdk/include/mfx)
add_executable(dec_vm dec_vm.cpp)
target_link_libraries(dec_vm mfx msdkutils X11 va va-drm pthread)

include_directories (dec_vpp /usr/local/include)
include_directories (dec_vpp /opt/intel/mediasdk/include/mfx)
add_executable(dec_vpp dec_vpp.cpp)
target_link_libraries(dec_vpp mfx msdkutils X11 va va-drm pthread)
