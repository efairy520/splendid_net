cmake_minimum_required(VERSION 3.16)
project(splendid_net C)

set(CMAKE_C_STANDARD 99)

# 设置 Npcap 的路径（请根据你实际安装路径修改）
set(PCAP_INCLUDE_DIR "C:/Users/efair/Downloads/npcap-sdk-1.15/Include")
set(PCAP_LIBRARY_DIR "C:/Users/efair/Downloads/npcap-sdk-1.15/Lib")

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/xnet_app
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny
        ${PROJECT_SOURCE_DIR}/lib/xnet
        ${PCAP_INCLUDE_DIR}
)

link_directories(${PCAP_LIBRARY_DIR})

add_executable(${PROJECT_NAME} lib/xnet/pcap_device.c src/app.c)
add_subdirectory(src/xnet_tiny)
add_subdirectory(src/xnet_app)
target_link_libraries(${PROJECT_NAME} PRIVATE xnet_tiny xnet_app ${PCAP_LIBRARY_DIR}/x64/wpcap.lib Ws2_32)