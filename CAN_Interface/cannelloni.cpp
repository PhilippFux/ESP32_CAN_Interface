#include <FreeRTOS.h>
#include <driver/can.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "cannelloni.h"

static uint8_t seq_no;
static uint8_t udp_rx_packet_buf[CANNELLONI_UDP_RX_PACKET_BUF_LEN];
static uint8_t udp_tx_packet_buf[CANNELLONI_UDP_TX_PACKET_BUF_LEN]; // CANNELLONI_DATA_PACKET_BASE_SIZE + CANNELLONI_FRAME_BASE_SIZE + 8
static TaskHandle_t cannelloni_can_task_handle;
static TaskHandle_t cannelloni_udp_task_handle;
static TaskHandle_t cannelloni_udp_queue_task_handle;
static TaskHandle_t cannelloni_can_queue_task_handle;
static QueueHandle_t udp_rx_queue_handle;
static QueueHandle_t can_rx_queue_handle;
static struct sockaddr_in remote_address;
portMUX_TYPE timer_mux = portMUX_INITIALIZER_UNLOCKED;
volatile int interrupt_counter;
volatile bool send_event = false;
int total_interrupt_counter;

void IRAM_ATTR onTimer() {
    portENTER_CRITICAL_ISR(&timer_mux);
    send_event = true;
    //interrupt_counter++;
    portEXIT_CRITICAL_ISR(&timer_mux);
} hw_timer_t *send_udp_timer = NULL;



void reset_timer(hw_timer_t *hw_timer) {
    timerWrite(hw_timer, 0);
    timerAlarmEnable(hw_timer);
}

void clear_event() {
    portENTER_CRITICAL(&timer_mux);
    send_event = false;
    portEXIT_CRITICAL(&timer_mux);
}

void setup_timer() {
    send_udp_timer = timerBegin(0, 80, true);
    timerAttachInterrupt(send_udp_timer, &onTimer, true);
    timerAlarmWrite(send_udp_timer, 30000, false);
    timerAlarmEnable(send_udp_timer);
}

bool event_is_present() {
    return send_event;
}

/* ------------ TIMER ------------ */





bool is_can_started();
extern int udp_socket;

struct udp_rx_queue_message {
    uint8_t udp_rx_packet_buf[CANNELLONI_UDP_RX_PACKET_BUF_LEN];
    size_t len;
};

struct __attribute__((__packed__)) CannelloniDataPacket {
    uint8_t version;
    uint8_t op_code;
    uint8_t seq_no;
    uint16_t count;
};

enum op_codes {
    DATA = 0,
    ACK = 1,
    NACK = 2
};

size_t cannelloni_build_packet(uint8_t *buffer, can_message_t *frames, int frame_count) 
{     //POSIX alternative ssize_t or Standard C size_t
    struct CannelloniDataPacket *snd_hdr;
    uint8_t *payload;
    uint32_t can_id;
    int i;
    size_t packet_size = 0;

    snd_hdr = (struct CannelloniDataPacket *) buffer;
    snd_hdr->count = 0;
    snd_hdr->op_code = DATA;
    snd_hdr->seq_no = seq_no++;
    snd_hdr->version = CANNELLONI_FRAME_VERSION;
    snd_hdr->count = htons(frame_count);       // normal use with htons(): converts u_short to TCP/IP network byte order

    payload = buffer + CANNELLONI_DATA_PACKET_BASE_SIZE;

    for (i = 0; i < frame_count; i++) {
        can_id = frames[i].identifier;
        if (frames[i].flags & CAN_MSG_FLAG_EXTD) {    // extended frame format
            can_id |= CAN_EFF_FLAG;
        }
        if (frames[i].flags & CAN_MSG_FLAG_RTR) {     // remote transmission request
            can_id |= CAN_RTR_FLAG;
        }

        can_id = htonl(can_id);       // converts u_long to TCP/IP network byte order
        memcpy(payload, &can_id, sizeof(can_id));
        payload += sizeof(can_id);

        *payload = frames[i].data_length_code;
        payload += sizeof(frames[i].data_length_code);

        if ((frames[i].flags & CAN_MSG_FLAG_RTR) == 0) {
            memcpy(payload, frames[i].data, frames[i].data_length_code);
            payload += frames[i].data_length_code;
        }
    }
    packet_size = payload - buffer;
    return packet_size;
}

void cannelloni_can_queue() {
    int packet_size;
    can_message_t can_messages[20];
    can_message_t can_message;
    int queue_size;
    int sent;
    int i;
    const TickType_t xDelay = 3 / portTICK_PERIOD_MS;    

    while (true) {
        queue_size = uxQueueMessagesWaiting(can_rx_queue_handle);
        if ((queue_size >= 12) || (event_is_present() && queue_size > 0)) {  //TODO: best order for more performance 
            i = 0;
            while (queue_size > 0 && is_can_started()) {
                xQueueReceive(can_rx_queue_handle, &can_message, portMAX_DELAY);
                can_messages[i] = can_message;

                queue_size--;
                i++;
            }
            packet_size = cannelloni_build_packet(udp_tx_packet_buf, can_messages, i);
            if (packet_size < 0) {
                Serial.println("cannelloni_build_packet failed");
                ESP_LOGW(ESP_TAG, "cannelloni_build_packet failed");
            }
            sent = sendto(udp_socket, udp_tx_packet_buf, packet_size, 0, (struct sockaddr*) &remote_address, sizeof(remote_address));
            if (sent < 0) {
                ESP_LOGW(ESP_TAG, "sendto: %s", strerror(errno));
            }
            else {
                //ESP_LOGI(ESP_TAG, ("UDP Message (%d) sent to %08x:%d", packet_size,
                //ntohl(udp.client.sin_addr.s_addr), ntohs(udp.sin_port)));
            }
            clear_event();
            reset_timer(send_udp_timer);
        }
        else {
            vTaskDelay(xDelay);
        }
    }
}

void cannelloni_can_main() {
    can_message_t can_message;
    esp_err_t error;
    int packet_size;

    while (true) {
      if(is_can_started())
      {
        error = can_receive(&can_message, (1000 / portTICK_PERIOD_MS));
        if (error == ESP_OK) {
            ESP_LOGI(ESP_TAG, "CAN Message received");
            xQueueSend(can_rx_queue_handle, (void *) &can_message, portMAX_DELAY);
        }
        else if (error == ESP_ERR_TIMEOUT) {
        }
        else {
            ESP_LOGE(ESP_TAG, "CAN message received failed");
            return;
        }
    }
    }
}

int cannelloni_udp_onrecv(uint8_t *buffer, size_t len) {
    struct CannelloniDataPacket *rcv_hdr;
    const uint8_t* raw_data = buffer + CANNELLONI_DATA_PACKET_BASE_SIZE;
    int received_frames;
    can_message_t can_frame;
    esp_err_t error;

    if (len < CANNELLONI_DATA_PACKET_BASE_SIZE) {
        ESP_LOGW(ESP_TAG, "Not enough data received");
        return -1;
    }

    rcv_hdr = (struct CannelloniDataPacket *) buffer;

    if (rcv_hdr->version != CANNELLONI_FRAME_VERSION) {
        ESP_LOGE(ESP_TAG, "Received wrong frame version");
        return -1;
    }

    if (rcv_hdr->op_code != DATA) {
        ESP_LOGE(ESP_TAG, "Received wrong OP code");
        return -1;
    }

    received_frames = ntohs(rcv_hdr->count);
    if (received_frames == 0) {
        return 0;
    }

    for (uint16_t i = 0; i < received_frames; i++) {
        if ((raw_data - buffer + CANNELLONI_FRAME_BASE_SIZE) > len) {
            ESP_LOGE(ESP_TAG, "Received incomplete UDP packet");
            return -1;
        }

        uint32_t can_id;
        memcpy(&can_id, raw_data, sizeof(can_id));
        can_id = ntohl(can_id);
        raw_data += sizeof(can_id);

        can_frame.flags = CAN_MSG_FLAG_NONE;
        if (can_id & CAN_ERR_FLAG) {
            // TODO: what?
        }
        if (can_id & CAN_EFF_FLAG) {
            can_frame.flags |= CAN_MSG_FLAG_EXTD;
        }
        if (can_id & CAN_RTR_FLAG) {
            can_frame.flags |= CAN_MSG_FLAG_RTR;
        }

        if (can_frame.flags & CAN_MSG_FLAG_EXTD) {
            can_frame.identifier = can_id & CAN_EFF_MASK;
        }
        else {
            can_frame.identifier = can_id & CAN_SFF_MASK;
        }

        can_frame.data_length_code = *raw_data;
        raw_data += sizeof(can_frame.data_length_code);

        if ((can_frame.flags & CAN_MSG_FLAG_RTR) == 0) {
            if ((raw_data - buffer + can_frame.data_length_code) > len) {
                ESP_LOGE(ESP_TAG, "Received an incomplete packet or the can header is corrupt");
                return -1;
            }
            memcpy(can_frame.data, raw_data, can_frame.data_length_code);
            raw_data += can_frame.data_length_code;
        }

        //ESP_LOGI(ESP_TAG, "CAN message received over UDP");
        error = can_transmit(&can_frame, 0);
        if (error == ESP_ERR_TIMEOUT) {
            Serial.println("Waiting for space in TX queue triggered a time out");
            ESP_LOGW(ESP_TAG, "Waiting for space in TX queue triggered a time out");
        }
        else if (error == ESP_FAIL) {
            Serial.println("TX queue is disabled (transmitting another message)");
            ESP_LOGW(ESP_TAG, "TX queue is disabled (transmitting another message)");
        }
        else if (error != ESP_OK) {
            Serial.println("Transmission was unsuccessful");
            ESP_LOGE(ESP_TAG, "Transmission was unsuccessful: %d", error);
            return -1;
        }
    }
    return received_frames;
}

void cannelloni_udp_queue() {
    struct udp_rx_queue_message udp_rx_message;
    while (true) {
        xQueueReceive(udp_rx_queue_handle, &udp_rx_message, portMAX_DELAY);
        cannelloni_udp_onrecv(udp_rx_message.udp_rx_packet_buf, udp_rx_message.len);
    }
}

void cannelloni_udp_main() {
    struct sockaddr_in rcvAddress;
    socklen_t rcv_address_len;
    struct udp_rx_queue_message udp_rx_message;
    int rcv;

    while (true) {
        rcv = recvfrom(udp_socket, udp_rx_packet_buf, CANNELLONI_UDP_RX_PACKET_BUF_LEN, 0, (struct sockaddr*)&rcvAddress, &rcv_address_len);
        if (rcv < 0) {
            ESP_LOGE(ESP_TAG, "Receive UDP message failed");
        }
        if (rcv > 0) {
            //ESP_LOGI(ESP_TAG, "UDP message received");
            memcpy(udp_rx_message.udp_rx_packet_buf, udp_rx_packet_buf, sizeof(udp_rx_packet_buf));
            udp_rx_message.len = rcv;
            xQueueSend(udp_rx_queue_handle, (void *) &udp_rx_message, portMAX_DELAY);
            //cannelloni_udp_onrecv(udp_rx_packet_buf, rcv);
        }
    }
}

void cannelloni_can_task(void *pv_parm) {
    Serial.println("cannelloni_can_task");
    cannelloni_can_main();
    vTaskDelete(NULL);
}

void cannelloni_udp_task(void *pv_parm) {
    Serial.println("cannelloni_udp_task");
    cannelloni_udp_main();
    vTaskDelete(NULL);
}

void cannelloni_udp_queue_task(void *pv_parm) {
    Serial.println("cannelloni_udp_queue_task");
    cannelloni_udp_queue();
    vTaskDelete(NULL);
}

void cannelloni_can_queue_task(void *pv_parm) {
    Serial.println("cannelloni_can_queue_task...");
    cannelloni_can_queue();
    vTaskDelete(NULL);
}

void create_udp_rx_queue() {
    udp_rx_queue_handle = xQueueCreate(60, sizeof(struct udp_rx_queue_message));
}

void create_can_rx_queue() {
    can_rx_queue_handle = xQueueCreate(60, sizeof(can_message_t));
}

void cannelloni_start() {
    Serial.println("cannelloni_start...");
    create_udp_rx_queue();
    xTaskCreate(&cannelloni_udp_task, "cannelloni_udp_task", 5048, NULL, 5, &cannelloni_udp_task_handle);
    xTaskCreate(&cannelloni_udp_queue_task, "cannelloni_udp_queue_task", 5048, NULL, 5, &cannelloni_udp_queue_task_handle);
}

void cannelloni_stop() {
    vTaskDelete(cannelloni_udp_task_handle);
}

void cannelloni_init() {
    //  TODO: pair with the client app
    uint8_t buffer[50];
    socklen_t remote_address_len;
    setup_timer();

    int rcv = recvfrom(udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote_address, &remote_address_len);

    if (rcv <= 0) {
        Serial.println("Init Message not rcv");
    }
    Serial.println("Cannelloni init...");
    memset(buffer, 0, 50);

    create_can_rx_queue();
    xTaskCreate(&cannelloni_can_task, "cannelloni_can_task", 4048, NULL, 5, &cannelloni_can_task_handle);   //2048
    xTaskCreate(&cannelloni_can_queue_task, "cannelloni_can_queue_task", 4048, NULL, 5, &cannelloni_can_queue_task_handle);
}
