//
// Created by efairy520 on 2025/12/9.
//

#include "xnet_tcp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xnet_ip.h"

// 静态资源池
static xtcp_socket_t tcp_socket_pool[XTCP_MAX_SOCKET_COUNT];

static xnet_status_t tcp_send_reset(uint32_t remote_ack, uint16_t local_port, xip_addr_t* remote_ip, uint16_t remote_port) {
    xnet_packet_t* packet = xnet_alloc_tx_packet(sizeof(xtcp_hdr_t));
    xtcp_hdr_t* tcp_hdr = (xtcp_hdr_t*) packet->data;

    tcp_hdr->src_port = swap_order16(local_port);
    tcp_hdr->dest_port = swap_order16(remote_port);
    tcp_hdr->seq = 0;
    tcp_hdr->ack = swap_order32(remote_ack);
    tcp_hdr->hdr_flags.all = 0;
    tcp_hdr->hdr_flags.hdr_len = sizeof(xtcp_hdr_t) / 4;
    tcp_hdr->hdr_flags.flags = XTCP_FLAG_RST | XTCP_FLAG_ACK;
    tcp_hdr->hdr_flags.all = swap_order16(tcp_hdr->hdr_flags.all);
    tcp_hdr->window = 0;
    tcp_hdr->checksum = 0;
    tcp_hdr->urgent_ptr = 0;

    tcp_hdr->checksum = checksum_peso(&xnet_local_ip, remote_ip, XNET_PROTOCOL_TCP, (uint16_t*)packet->data, packet->length);
    tcp_hdr->checksum = tcp_hdr->checksum ? tcp_hdr->checksum : 0xFFFF;
    return xip_out(XNET_PROTOCOL_TCP, remote_ip, packet);
}

static void tcp_read_mss(xtcp_socket_t * tcp, xtcp_hdr_t * tcp_hdr)
{
    uint16_t opt_len = tcp_hdr->hdr_flags.hdr_len * 4 - sizeof(xtcp_hdr_t);

    if (opt_len == 0) {
        tcp->remote_mss = XTCP_MSS_DEFAULT;
    }
    else {
        uint8_t* opt_data = (uint8_t*)tcp_hdr + sizeof(xtcp_hdr_t);
        uint8_t* opt_end = opt_data + opt_len;

        while ((*opt_data != XTCP_KIND_END) && (opt_data < opt_end)) {
            if ((*opt_data++ == XTCP_KIND_MSS) && (*opt_data++ == 4)) {
                tcp->remote_mss = swap_order16(*(uint16_t *)opt_data);
                return;
            }
        }
    }
}

static xnet_status_t tcp_send(xtcp_socket_t* tcp, uint8_t flags) {
    xnet_packet_t* packet;
    xtcp_hdr_t* tcp_hdr;
    xnet_status_t status;

    packet = xnet_alloc_tx_packet(sizeof(xtcp_hdr_t));
    tcp_hdr = (xtcp_hdr_t*) packet->data;

    tcp_hdr->src_port = swap_order16(tcp->local_port);
    tcp_hdr->dest_port = swap_order16(tcp->remote_port);
    tcp_hdr->seq = tcp->next_seq;
    tcp_hdr->ack = swap_order32(tcp->ack);
    tcp_hdr->hdr_flags.all = 0;
    tcp_hdr->hdr_flags.hdr_len = sizeof(xtcp_hdr_t) / 4;
    tcp_hdr->hdr_flags.flags = flags;
    tcp_hdr->hdr_flags.all = swap_order16(tcp_hdr->hdr_flags.all);
    tcp_hdr->window = 1024;
    tcp_hdr->checksum = 0;
    tcp_hdr->urgent_ptr = 0;

    tcp_hdr->checksum = checksum_peso(xnet_local_ip.addr, &tcp->remote_ip, XNET_PROTOCOL_TCP,
                                     (uint16_t*)packet->data, packet->length);
    tcp_hdr->checksum = tcp_hdr->checksum ? tcp_hdr->checksum : 0xFFFF;

    status = xip_out(XNET_PROTOCOL_TCP, &tcp->remote_ip, packet);
    if (status < 0) return status;

    if (flags & (XTCP_FLAG_SYN | XTCP_FLAG_FIN)) {
        tcp->next_seq++;
    }
    return XNET_OK;
}

static void tcp_process_accept(xtcp_socket_t* listen_tcp, xip_addr_t* remote_ip, xtcp_hdr_t* tcp_hdr) {
    uint16_t hdr_flags = tcp_hdr->hdr_flags.all;

    if (hdr_flags & XTCP_FLAG_SYN) {
        xnet_status_t err;
        uint32_t ack = tcp_hdr->seq + 1;

        xtcp_socket_t* new_tcp = xtcp_alloc_socket(listen_tcp->handler);
        if (!new_tcp) return;

        new_tcp->state = XTCP_STATE_SYN_RECVD;
        new_tcp->local_port = listen_tcp->local_port;
        new_tcp->handler = listen_tcp->handler;
        new_tcp->remote_port = tcp_hdr->src_port;
        new_tcp->remote_ip = *remote_ip;
        new_tcp->ack = ack;
        new_tcp->next_seq = tcp_get_init_seq();
        new_tcp->remote_win = tcp_hdr->window;

        tcp_read_mss(new_tcp, tcp_hdr);

        err = tcp_send(new_tcp, XTCP_FLAG_SYN | XTCP_FLAG_ACK);
        if (err < 0) {
            xtcp_close_socket(new_tcp);
        }
    } else {
        tcp_send_reset(tcp_hdr->seq, listen_tcp->local_port, remote_ip, tcp_hdr->src_port);
    }
}

void xtcp_init(void) {
    // 整体清零，确保所有状态为 XTCP_STATE_FREE (0)
    memset(tcp_socket_pool, 0, sizeof(tcp_socket_pool));
}

void xtcp_in(xip_addr_t* remote_ip, xnet_packet_t* packet) {
    xtcp_hdr_t* tcp_hdr = (xtcp_hdr_t*) packet->data;
    uint16_t pre_checksum;
    xtcp_socket_t* socket;

    if (packet->length < sizeof(xtcp_hdr_t)) {
        return;
    }

    pre_checksum = tcp_hdr->checksum;
    tcp_hdr->checksum = 0;
    if (pre_checksum != 0) {
        uint16_t checksum = checksum_peso(remote_ip, &xnet_local_ip, XNET_PROTOCOL_TCP, (uint16_t*) tcp_hdr, packet->length);
        checksum = (checksum == 0) ? 0xFFFF : checksum;
        if (checksum != pre_checksum) {
            return;
        }
    }

    tcp_hdr->src_port = swap_order16(tcp_hdr->src_port);
    tcp_hdr->dest_port = swap_order16(tcp_hdr->dest_port);
    tcp_hdr->hdr_flags.all = swap_order16(tcp_hdr->hdr_flags.all);
    tcp_hdr->seq = swap_order32(tcp_hdr->seq);
    tcp_hdr->ack = swap_order32(tcp_hdr->ack);
    tcp_hdr->window = swap_order16(tcp_hdr->window);

    socket = xtcp_find_socket(remote_ip, tcp_hdr->src_port, tcp_hdr->dest_port);
    if (socket == NULL) {
        tcp_send_reset(tcp_hdr->seq + 1, tcp_hdr->dest_port, remote_ip, tcp_hdr->src_port);
        return;
    }
    socket->remote_win = tcp_hdr->window;

    if (socket->state == XTCP_STATE_LISTEN) {
        tcp_process_accept(socket, remote_ip, tcp_hdr);
        return;
    }

    if (tcp_hdr->seq != socket->ack) {
        tcp_send_reset(tcp_hdr->seq + 1, tcp_hdr->dest_port, remote_ip, tcp_hdr->src_port);
        return;
    }

    remove_header(packet, tcp_hdr->hdr_flags.hdr_len);
    switch (socket->state) {
        case XTCP_STATE_SYN_RECVD:
            if (tcp_hdr->hdr_flags.flags & XTCP_FLAG_ACK) {
                socket->state = XTCP_STATE_ESTABLISHED;
                socket->handler(socket, XTCP_EVENT_CONNECTED);
            }
            break;
        case XTCP_STATE_ESTABLISHED:
            printf("connection ok");
            break;
    }
}

xtcp_socket_t* xtcp_alloc_socket(xtcp_event_handler_t handler) {
    for (xtcp_socket_t* curr = tcp_socket_pool; curr < &tcp_socket_pool[XTCP_MAX_SOCKET_COUNT]; curr++) {
        // 找到一个空闲的 Socket
        if (curr->state == XTCP_STATE_FREE) {
            // 【优化点1】：必须清空旧数据！
            // Socket被回收后，里面可能残留着上一次连接的 IP、端口等脏数据。
            //如果不清零，下次 bind 可能出错，或者调试时看到奇怪的数值。
            memset(curr, 0, sizeof(xtcp_socket_t));

            // 初始化状态和回调
            curr->state = XTCP_STATE_CLOSED;
            curr->handler = handler;
            curr->remote_win = XTCP_MSS_DEFAULT;
            curr->remote_mss = XTCP_MSS_DEFAULT;
            curr->next_seq = tcp_get_init_seq();
            return curr;
        }
    }
    return NULL;
}

xnet_status_t xtcp_bind_socket(xtcp_socket_t* socket, uint16_t local_port) {
    if (socket == NULL || local_port == 0) {
        return XNET_ERR_PARAM;
    }

    // 1. 检查端口是否已占用
    for (xtcp_socket_t* curr = tcp_socket_pool; curr < &tcp_socket_pool[XTCP_MAX_SOCKET_COUNT]; curr++) {
        // 如果 curr 是 FREE 的，即使它残留的 local_port 也是这个值，也不算冲突。
        if (curr != socket && curr->state != XTCP_STATE_FREE && curr->local_port == local_port) {
            return XNET_ERR_BINDED;
        }
    }

    // 2. 绑定
    socket->local_port = local_port;
    return XNET_OK;
}

// 接收到请求后，找到匹配的 socket
// 优先级：已建立连接的五元组匹配 > 监听端口匹配
xtcp_socket_t* xtcp_find_socket(xip_addr_t* remote_ip, uint16_t remote_port, uint16_t local_port) {
    xtcp_socket_t* listen_socket = NULL;

    for (xtcp_socket_t* curr = tcp_socket_pool; curr < &tcp_socket_pool[XTCP_MAX_SOCKET_COUNT]; curr++) {
        // 0. FREE 的直接跳过
        if (curr->state == XTCP_STATE_FREE) {
            continue;
        }

        // 1. 本地端口必须匹配
        if (curr->local_port != local_port) {
            continue;
        }

        // 2. 检查五元组精确匹配（针对 ESTABLISHED, SYN_SENT 等活动连接）
        // 只有远程 IP 和端口都匹配，才是真正的“当前连接”
        // 【逻辑调整】：建议先判断精确匹配，这样逻辑流更顺畅，虽然结果一样。
        if (xip_addr_eq(remote_ip, &curr->remote_ip) && (remote_port == curr->remote_port)) {
            return curr; // 找到唯一活动连接，直接返回
        }

        // 3. 检查 LISTEN 状态 (作为备选)
        // 如果没找到上面的精确匹配，但找到了一个监听该端口的 Server Socket
        if (curr->state == XTCP_STATE_LISTEN) {
            listen_socket = curr;
            // 注意：不要 break，继续往后找。万一后面有一个精确匹配的呢？
            // 比如 socket[0] 是 LISTEN 80，socket[1] 是 ESTABLISHED 80 (conn 1)...
        }
    }

    // 4. 如果没找到活动连接，返回监听 Socket（如果有的话）用来建立新连接
    return listen_socket;
}

xnet_status_t xtcp_listen_socket(xtcp_socket_t* socket) {
    if (socket == NULL) return XNET_ERR_PARAM;

    // 只有处于 CLOSED 状态且已绑定端口的 Socket 才能开始 Listen
    if (socket->state != XTCP_STATE_CLOSED) {
         return XNET_ERR_STATE;
    }
    if (socket->local_port == 0) {
        return XNET_ERR_PARAM;
    }

    socket->state = XTCP_STATE_LISTEN;
    return XNET_OK;
}

xnet_status_t xtcp_close_socket(xtcp_socket_t* socket) {
    if (socket == NULL) return XNET_ERR_PARAM;

    // 【优化点4】：资源回收
    // 这是一个强制销毁函数（对应 alloc）。
    // 在完整的 TCP 实现中，这里可能需要判断状态，如果是 ESTABLISHED，可能要发 RST。
    // 但作为资源释放函数，简单置为 FREE 是可以的。
    socket->state = XTCP_STATE_FREE;
    // 建议清空 handler 以防野指针调用
    socket->handler = NULL;
    return XNET_OK;
}