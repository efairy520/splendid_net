#ifndef XSOCKET_H
#define XSOCKET_H

#include <stdint.h>
#include "xnet_def.h"

typedef struct _xsocket_t xsocket_t;

typedef enum {
    XSOCKET_TYPE_TCP = 0,
    XSOCKET_TYPE_UDP = 1,
} xsocket_type_t;

// ===== 打开/关闭 =====
// 兼容旧代码：默认打开 TCP
xsocket_t *xsocket_open(void);
// 新接口：明确指定 TCP / UDP
xsocket_t *xsocket_open_ex(xsocket_type_t type);

void xsocket_close(xsocket_t *socket);

// ===== 通用：绑定 =====
xnet_status_t xsocket_bind(xsocket_t *socket, uint16_t port);

// ===== TCP 专用 =====
xnet_status_t xsocket_listen(xsocket_t *socket);
xsocket_t *xsocket_accept(xsocket_t *socket);

int xsocket_write(xsocket_t *socket, const char *data, int len);

int xsocket_try_read(xsocket_t *socket, char *buf, int max_len);
int xsocket_read_timeout(xsocket_t *socket, char *buf, int max_len, int max_polls);
int xsocket_read(xsocket_t *socket, char *buf, int max_len);

// ===== UDP 专用 =====
int xsocket_sendto(xsocket_t *socket, const char *data, int len,
                   const xip_addr_t *dest_ip, uint16_t dest_port);

// 返回：>0 收到的字节数；0 超时；-1 参数/状态错误
int xsocket_recvfrom(xsocket_t *socket, char *buf, int max_len,
                     xip_addr_t *src_ip, uint16_t *src_port, int max_polls);

#endif // XSOCKET_H
