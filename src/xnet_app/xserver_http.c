//
// Created by efairy520 on 2025/12/12.
//

#include "xserver_http.h"
#include <stdio.h>
#include "xnet_tcp.h"

static xnet_status_t http_handler(xtcp_socket_t* socket, xtcp_event_type_t event) {
    if (event == XTCP_EVENT_CONNECTED) {
        printf("http connected\n");
    }
    if (event == XTCP_EVENT_CLOSED) {
        printf("http closed\n");
    }
    return XNET_OK;
}

xnet_status_t xserver_http_create(uint16_t port) {
    xtcp_socket_t* socket = xtcp_alloc_socket(http_handler);
    xtcp_bind_socket(socket, port);
    xtcp_listen_socket(socket);
    return XNET_OK;
}
