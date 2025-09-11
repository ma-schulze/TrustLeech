cmake_minimum_required(VERSION 3.0)

project(TrustLeechVMI)


add_executable(vmi_client client.cpp)
add_executable(vmi_client_2 client_2.cpp)
add_executable(vmi_client_3 client_3.cpp)
add_executable(vmi_client_4 client_4.cpp)


target_link_libraries(vmi_client vmi)
target_link_libraries(vmi_client crypto)

target_link_libraries(vmi_client_2 vmi)
target_link_libraries(vmi_client_2 crypto)

target_link_libraries(vmi_client_3 vmi)
target_link_libraries(vmi_client_3 crypto)

target_link_libraries(vmi_client_4 vmi)
target_link_libraries(vmi_client_4 crypto)

include_directories(${CMAKE_SYSROOT}/usr/include/)

install(TARGETS vmi_client DESTINATION bin)
install(TARGETS vmi_client_2 DESTINATION bin)
install(TARGETS vmi_client_3 DESTINATION bin)
install(TARGETS vmi_client_4 DESTINATION bin)

