//
// Created by efairy520 on 2025/12/9.
//

#ifndef XNET_TCP_H
#define XNET_TCP_H

#include "xnet_tiny.h"

// 1. TCP PCB 最大数量
#define XTCP_PCB_MAX_NUM   40
#define XTCP_FLAG_FIN    (1 << 0)
#define XTCP_FLAG_SYN    (1 << 1)
#define XTCP_FLAG_RST    (1 << 2)
#define XTCP_FLAG_ACK    (1 << 4)

#define XTCP_KIND_END       0
#define XTCP_KIND_MSS       2
#define XTCP_MSS_DEFAULT    1460

#pragma pack(1)
typedef struct _xtcp_hdr_t {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack;
    union {
        uint16_t all;
        struct {
            uint16_t flags : 6;       // 低6位
            uint16_t reserved : 6;    // 中间6位
            uint16_t hdr_len : 4;     // 高4位
        };
    }hdr_flags;

    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
}xtcp_hdr_t;
#pragma pack(0)

// 2. TCP 生命周期状态
typedef enum _xtcp_state_t {
    XTCP_STATE_FREE,
    XTCP_STATE_CLOSED,
    XTCP_STATE_LISTEN,
    XTCP_STATE_SYN_RECVD,
    XTCP_STATE_ESTABLISHED,
} xtcp_state_t;

// 3. 事件类型
typedef enum _xtcp_event_t {
    XTCP_EVENT_CONNECTED,       // 连接成功
    XTCP_EVENT_DATA_RECEIVED,   // 收到数据
    XTCP_EVENT_CLOSED,          // 连接断开
    XTCP_EVENT_ABORTED,         // 连接异常终止 (RST)
} xtcp_event_t;

// 4. TCP 控制块
typedef struct _xtcp_pcb_t xtcp_pcb_t;

// TCP 事件回调函数指针（接口）
typedef xnet_status_t (*xtcp_event_handler_t) (xtcp_pcb_t* pcb, xtcp_event_t event);

// 5. TCP PCB 结构体
struct _xtcp_pcb_t {
    xtcp_state_t           state;
    uint16_t               local_port;
    uint16_t               remote_port;
    xip_addr_t             remote_ip;
    uint32_t               seq;
    uint32_t               ack;
    uint16_t               remote_mss;
    uint16_t               remote_win;
    xtcp_event_handler_t   handler;
};

void xtcp_init(void);
void xtcp_in(xip_addr_t* remote_ip, xnet_packet_t* packet);

xtcp_pcb_t* xtcp_pcb_new(xtcp_event_handler_t handler);
xnet_status_t xtcp_pcb_bind(xtcp_pcb_t* pcb, uint16_t local_port);
xtcp_pcb_t* xtcp_pcb_find(xip_addr_t* remote_ip, uint16_t remote_port, uint16_t local_port);
xnet_status_t xtcp_pcb_listen(xtcp_pcb_t* pcb);
xnet_status_t xtcp_pcb_free(xtcp_pcb_t* pcb);


#endif //XNET_TCP_H