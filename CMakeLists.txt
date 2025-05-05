cmake_minimum_required(VERSION 3.31)
project(splendid_net C)

set(CMAKE_C_STANDARD 99)

include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/xnet_app
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny
        ${PROJECT_SOURCE_DIR}/lib/xnet
)

add_executable(${PROJECT_NAME} src/app.c)
add_subdirectory(src/xnet_tiny)
target_link_libraries(${PROJECT_NAME} xnet_tiny)
