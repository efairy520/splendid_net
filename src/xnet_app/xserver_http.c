//
// Created by efairy520 on 2025/12/12.
//

#include "xserver_http.h"
#include <stdio.h>
#include "xnet_tcp.h"

static xnet_status_t http_handler(xtcp_pcb_t* pcb, xtcp_event_t event) {
    switch (event) {
        case XTCP_EVENT_CONNECTED:
            printf("http: new client connected\n");
            xtcp_pcb_close(pcb);
            break;

        case XTCP_EVENT_CLOSED:
            printf("http: connection closed\n");
            break;

        default:
            break;
    }
    return XNET_OK;
}

xnet_status_t xserver_http_create(uint16_t port) {
    xtcp_pcb_t* pcb = xtcp_pcb_new(http_handler);
    xtcp_pcb_bind(pcb, port);
    xtcp_pcb_listen(pcb);
    return XNET_OK;
}
