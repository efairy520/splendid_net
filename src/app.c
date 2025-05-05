#include <stdio.h>
#include "xnet_tiny.h"

int main (void) {
    setvbuf(stdout, NULL, _IONBF, 0);  // 禁用缓冲，保证 printf 立刻显示

    xnet_init();

    printf("xnet running\n");

    while (1) {
        xnet_poll();
    }

    return 0;
}