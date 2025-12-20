# src/xnet_tiny/CMakeLists.txt

# --- 修改前 (我们手动列出的列表漏了文件) ---
# set(TINY_SRCS
#     xnet_ethernet.c
#     xnet_arp.c
#     ...
# )

# --- 修改后 (直接自动抓取该目录下所有 .c 文件) ---
file(GLOB TINY_SRCS "*.c")

# 2. 生成库
add_library(xnet_tiny STATIC ${TINY_SRCS})

# 3. 【关键】告诉别的模块：如果要用我，去哪里找头文件
# 这样父目录就不用写 include_directories(...) 了
target_include_directories(xnet_tiny
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        # 【修复1】这里加上 app 的路径，因为 xnet_ethernet.c 需要 xnet_driver.h
        ${PROJECT_SOURCE_DIR}/src/xnet_app
)