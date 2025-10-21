/**
 * 手写 TCP/IP 协议栈
 */
#ifndef XNET_TINY_H
#define XNET_TINY_H

#include <stdint.h>

#define XNET_CFG_PACKET_MAX_SIZE        1514                // 收发数据包的最大大小
#define XNET_CFG_NETIF_IP               {192, 168, 254, 2}  // 协议栈的IP地址

#define xipaddr_is_equal_buf(addr, buf)      (memcmp((addr)->array, buf, XNET_IPV4_ADDR_SIZE) == 0)   // 相等比较

/**
 * IP地址长度
 */
#define XNET_IPV4_ADDR_SIZE             4

/**
 * MAC地址长度
 */
#define XNET_MAC_ADDR_SIZE              6

/**
 * 错误码枚举
 */
typedef enum _xnet_err_t {
    XNET_ERR_OK = 0,
    XNET_ERR_IO = -1,
} xnet_err_t;

/**
 * 网络数据包
 */
typedef struct _xnet_packet_t {
    uint16_t size; // 包中有效数据大小（因为并不一定会占满载荷）
    uint8_t *data; // 包的数据起始地址
    uint8_t payload[XNET_CFG_PACKET_MAX_SIZE]; // 载荷字节数组
} xnet_packet_t;

typedef uint32_t xnet_time_t;           // 时间类型
const xnet_time_t xsys_get_time(void);

/**
 * 分配一个发送包
 * @param size
 * @return
 */
xnet_packet_t *xnet_alloc_for_send(uint16_t size);

/**
 * 分配一个读取包
 * @param size
 * @return
 */
xnet_packet_t *xnet_alloc_for_read(uint16_t size);

/**
 * 打开驱动
 * @param mac_addr
 * @return
 */
xnet_err_t xnet_driver_open(uint8_t *mac_addr);

/**
 * 发送一个数据包
 * @param packet
 * @return
 */
xnet_err_t xnet_driver_send(xnet_packet_t *packet);

/**
 * 读取一个数据包
 * @param packet
 * @return
 */
xnet_err_t xnet_driver_read(xnet_packet_t **packet);

void add_header(xnet_packet_t *packet, uint16_t header_size);
void remove_header(xnet_packet_t *packet, uint16_t header_size);
void truncate_packet(xnet_packet_t *packet, uint16_t size);

/**
 * IP地址，使用共用体，节省空间
 */
typedef union _xipaddr_t {
    uint8_t array[XNET_IPV4_ADDR_SIZE]; // 以字节形式存储的ip
    uint32_t addr; // 32位的ip地址
} xipaddr_t;

/**
 * 协议栈的初始化
 */
void xnet_init(void);

/**
 * 轮询数据包
 */
void xnet_poll(void);

#endif // XNET_TINY_H
