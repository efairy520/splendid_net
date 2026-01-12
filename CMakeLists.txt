cmake_minimum_required(VERSION 3.16)
project(splendid_net C)

set(CMAKE_C_STANDARD 99)

# =======================================================
# !!! 关键修正：必须放在最前面 !!!
# 这样不仅主程序生效，所有 add_subdirectory 里的 DLL 也会生效
# =======================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# (可选) 如果你想把 .lib 导入库和静态库也放一起，可以加上下面两行：
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(NOT WIN32)
    add_compile_options(-march=native)
endif()

# =======================================================
# 1. 定义外部依赖 (这是 xnet_port 能工作的关键！)
# =======================================================
if(WIN32)
    # --- Windows Npcap 配置 ---
    set(NPCAP_SDK_DIR "C:/Users/efair/Downloads/npcap-sdk-1.15")

    # 定义 Npcap::SDK 目标，供子模块引用
    add_library(Npcap::SDK UNKNOWN IMPORTED)
    set_target_properties(Npcap::SDK PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${NPCAP_SDK_DIR}/Include"
            IMPORTED_LOCATION "${NPCAP_SDK_DIR}/Lib/x64/wpcap.lib"
            INTERFACE_LINK_LIBRARIES "${NPCAP_SDK_DIR}/Lib/x64/Packet.lib;Ws2_32"
    )
else()
    # --- Linux DPDK 配置 ---
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DPDK REQUIRED libdpdk)

    add_library(DPDK::SDK INTERFACE IMPORTED)
    target_include_directories(DPDK::SDK INTERFACE ${DPDK_INCLUDE_DIRS})
    target_link_directories(DPDK::SDK INTERFACE ${DPDK_LIBRARY_DIRS})
    target_link_libraries(DPDK::SDK INTERFACE
            -Wl,--whole-archive ${DPDK_LIBRARIES} -Wl,--no-whole-archive
            pthread numa dl m
    )
    add_compile_definitions(ALLOW_EXPERIMENTAL_API)
endif()

# 2. 添加子模块 (注意顺序：底层 -> 上层)
# A. 驱动层
if(WIN32)
    # 注册 pcap 驱动库！
    add_subdirectory(src/drivers/pcap)
else()
    # 注册 Linux 的驱动库！
    add_subdirectory(src/drivers/dpdk)
endif()
# B. 适配层 & 协议核心
add_subdirectory(src/xnet_port)
add_subdirectory(src/xnet_tiny)

# C. 系统调用层 (Socket)
add_subdirectory(src/xnet_socket)

# D. 应用层
# 它依赖 Socket，所以放在最后
add_subdirectory(src/xnet_app)

# =======================================================
# 3. 生成可执行文件
# =======================================================
add_executable(${PROJECT_NAME}
        src/app.c
        src/xnet_socket/xsocket.c
)

# 头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xnet_tiny/core

        # !!! 关键修正：补上 transport 目录，xnet_tcp.h 就在这里 !!!
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xnet_tiny/transport

        ${CMAKE_CURRENT_SOURCE_DIR}/src/xnet_socket
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xnet_app
)

# =======================================================
# 4. 最终链接
# =======================================================
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            xnet_app      # 链接应用层 (HTTP/Time)

            # !!! 必须链接这两个，否则 EXE 找不到 xnet_init 和 xnet_poll !!!
            xnet_tiny     # 链接核心协议栈
            xnet_port     # 链接驱动适配层

            # !!! 千万不要链接 xnet_socket !!!
            # 因为我们在上面已经直接编译了 xsocket.c
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
            xnet_app
            xnet_tiny
            xnet_port
    )
endif()