# 根目录 CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(splendid_net C)

set(CMAKE_C_STANDARD 99)

# =======================================================
# 全局编译选项
# =======================================================
if(NOT WIN32)
    add_compile_options(-march=native)
endif()

# =======================================================
# 依赖库查找 (环境配置保持不变)
# =======================================================
if(WIN32)
    # Windows Npcap 设置
    set(NPCAP_SDK_DIR "C:/Users/efair/Downloads/npcap-sdk-1.15")
    include_directories(${NPCAP_SDK_DIR}/Include)
    link_directories(${NPCAP_SDK_DIR}/Lib/x64)
else()
    # Linux DPDK 设置
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DPDK REQUIRED libdpdk)

    # 这里用 link_directories 配合后面的 target_link_libraries
    include_directories(${DPDK_INCLUDE_DIRS})
    link_directories(${DPDK_LIBRARY_DIRS})
    add_definitions(-DALLOW_EXPERIMENTAL_API)
endif()

# =======================================================
# 添加子模块
# =======================================================
add_subdirectory(src/xnet_tiny)
add_subdirectory(src/xnet_app)

# =======================================================
# 生成可执行文件
# =======================================================
# 注意：这里不再需要手动加 port_pcap.c 了，因为它已经在 xnet_app 库里了！
add_executable(${PROJECT_NAME} src/app.c)

# =======================================================
# 最终链接
# =======================================================
# 根目录 CMakeLists.txt 的末尾

if(WIN32)
    # Windows 链接逻辑
    target_link_libraries(${PROJECT_NAME} PRIVATE
            # 【核心修改】调整顺序，采用“三明治”写法解决循环依赖
            # 1. 先放 tiny，让 linker 知道我们需要 core 里的函数
            xnet_tiny

            # 2. 再放 app，满足 tiny 对 driver 的依赖
            xnet_app

            # 3. 再放一次 tiny (或者 app)，防止还没解开的依赖
            # 其实对于你的情况，通常 tiny 放在前面，app 放在后面就可以
            # 但为了保险，我们可以写成：
            xnet_app
            xnet_tiny
            xnet_app  # 让 app 兜底，因为 driver 是最底层的

            # 系统库
            wpcap
            Packet
            Ws2_32
    )
else()
    # Linux 链接逻辑
    target_link_libraries(${PROJECT_NAME} PRIVATE
            # 同样采用这种顺序
            xnet_tiny
            xnet_app

            # Linux 特有的解决循环依赖的神器：--whole-archive
            # 但为了简单，我们也用重复链接法
            xnet_tiny

            -Wl,--whole-archive ${DPDK_LIBRARIES} -Wl,--no-whole-archive
            pthread
            numa
            dl
            m
    )
endif()