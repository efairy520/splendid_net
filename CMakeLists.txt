cmake_minimum_required(VERSION 3.16)
project(splendid_net C)

set(CMAKE_C_STANDARD 99)

# 设置 npcap sdk 的路径（请根据你实际安装路径修改）
set(NPCAP_SDK_DIR "C:/Users/efair/Downloads/npcap-sdk-1.15")

# 添加头文件搜索路径
include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/xnet_app
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny
        ${PROJECT_SOURCE_DIR}/lib/xnet
        ${NPCAP_SDK_DIR}/Include
)

# 生成一个可执行文件，名字是 splendid_net
add_executable(${PROJECT_NAME} lib/xnet/pcap_device.c src/app.c)

# 添加子模块
add_subdirectory(src/xnet_tiny)
add_subdirectory(src/xnet_app)

# 把一些库链接到最终的可执行文件中
target_link_libraries(${PROJECT_NAME} PRIVATE xnet_tiny xnet_app ${NPCAP_SDK_DIR}/Lib/x64/wpcap.lib Ws2_32)