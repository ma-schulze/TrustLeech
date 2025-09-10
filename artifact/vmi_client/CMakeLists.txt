cmake_minimum_required(VERSION 3.0)

project(TrustLeechVMI)


add_executable(vmi_client client.cpp)

target_link_libraries(vmi_client vmi)
target_link_libraries(vmi_client crypto)


include_directories(${CMAKE_SYSROOT}/usr/include/)

install(TARGETS vmi_client DESTINATION bin)
