// 文件位置: src/xnet_socket/xnet_java_bridge.c
#include <stdio.h>
#include <windows.h> // Windows 线程专用头文件
#include "xnet_tiny.h"

// 这是一个标志位，控制线程运行
static volatile int is_running = 0;

// === 后台线程：协议栈的心脏 ===
// 这个函数会在后台一直跑，专门负责收包、发包、处理 TCP 握手
DWORD WINAPI xnet_poll_thread(LPVOID lpParam) {
    printf("[C-Bridge] Network Stack Thread Started (TID=%d).\n", GetCurrentThreadId());

    while (is_running) {
        // 核心驱动函数
        xnet_poll();

        // 稍微休眠一点点，避免 CPU 占用率 100% 导致电脑卡顿
        // 如果你需要极限性能，可以把这行注释掉
        Sleep(1);
    }
    return 0;
}

// === 暴露给 Java 的启动函数 ===
__declspec(dllexport) void xnet_startup_global(void) {
    if (is_running) {
        printf("[C-Bridge] Stack is already running!\n");
        return;
    }

    // 1. 禁用缓冲，确保 Java 控制台能立刻看到日志
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("[C-Bridge] Initializing xnet stack...\n");

    // 2. 初始化核心协议栈
    xnet_init();

    // 3. 启动后台线程
    is_running = 1;
    HANDLE hThread = CreateThread(
        NULL,                   // 默认安全属性
        0,                      // 默认堆栈大小
        xnet_poll_thread,       // 线程函数
        NULL,                   // 参数
        0,                      // 默认启动标志
        NULL                    // 不需要线程ID
    );

    if (hThread == NULL) {
        printf("[C-Bridge] Error: Failed to create background thread!\n");
    } else {
        printf("[C-Bridge] Background thread created successfully.\n");
        CloseHandle(hThread); // 句柄用完关闭，线程会继续跑
    }
}