#ifndef CANNELLONI_H
#define CANNELLONI_H

/* ------------ Cannelloni ------------ */

#define CANNELLONI_FRAME_VERSION 2
#define CANNELLONI_DATA_PACKET_BASE_SIZE 5
#define CANNELLONI_FRAME_BASE_SIZE 5
#define CANNELLONI_UDP_RX_PACKET_BUF_LEN 1600
#define CANNELLONI_UDP_TX_PACKET_BUF_LEN 1600

/* flags for CAN ID*/
#define CAN_EFF_FLAG 0x80000000U
#define CAN_RTR_FLAG 0x40000000U
#define CAN_ERR_FLAG 0x20000000U

/* frame formats for CAN ID */
#define CAN_SFF_MASK 0x000007FFU
#define CAN_EFF_MASK 0x1FFFFFFFU
#define CAN_ERR_MASK 0x1FFFFFFFU

// static char ESP_TAG[] = "CAN";



// size_t cannelloni_build_packet(uint8_t *buffer, can_message_t *frames, int frame_count);
// void cannelloni_can_queue();
// void cannelloni_can_main();
// int cannelloni_udp_onrecv(uint8_t *buffer, size_t len);
// void cannelloni_udp_queue();
// void cannelloni_udp_main();
// void cannelloni_can_task(void *pv_parm);
// void cannelloni_udp_task(void *pv_parm);
// void cannelloni_udp_queue_task(void *pv_parm);
// void cannelloni_can_queue_task(void *pv_parm);
// void create_udp_rx_queue();
// void create_can_rx_queue();

void cannelloni_start();
void cannelloni_stop();
void cannelloni_init();

#endif
