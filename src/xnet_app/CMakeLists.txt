set(APP_SRCS
        xserver_datetime.c
        xserver_http.c
)

if(WIN32)
    # 【修复2】加入 pcap_device.c，因为它和 port_pcap.c 是一伙的
    list(APPEND APP_SRCS
            port_pcap.c
            ${PROJECT_SOURCE_DIR}/lib/xnet/pcap_device.c
    )
else()
    list(APPEND APP_SRCS port_dpdk.c)
endif()

add_library(xnet_app STATIC ${APP_SRCS})

# 【修复3】改为 PUBLIC
# 这样链接了 xnet_app 的人（比如 main），也能自动看到 xnet_tiny 的头文件
target_link_libraries(xnet_app PUBLIC xnet_tiny)

target_include_directories(xnet_app
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        # 【修复4】告诉它去 lib/xnet 找 pcap_device.h
        ${PROJECT_SOURCE_DIR}/lib/xnet
)