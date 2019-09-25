#include <WiFi.h>
#include <WiFiUdp.h>
#include <driver/can.h>
#include <driver/gpio.h>
#include <FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <driver/can.h>
#include <driver/gpio.h>
/* ------------ Config ------------ */

#define SERIAL_BAUDRATE 115200
#define SSID "Tagliatelle"
#define PSWD "123456789"
#define DEFAULT_CAN_BITRATE 500000
#define DEFAULT_CAN_MODE 0

#define __USE_HTTP_CONFIG__
#define __USE_WEBSOCKET_CONFIG__

#define LOCAL_PORT 3333u
#define REMOTE_PORT 3333u

#define HTTP_PORT 80u

#define WEBSOCKET_PORT 81u

#ifdef __USE_WEBSOCKET_CONFIG__
#include <WebSocketServer.h> // https://github.com/morrissinger/ESP8266-Websocket
#include <ArduinoJson.h>  // Version 5
#endif


int udp_socket;
IPAddress ap_ip;
IPAddress remote_ip;
static struct sockaddr_in remote_address;
static struct sockaddr_in local_address;
static uint16_t remote_port = REMOTE_PORT;  // TODO: needed?

/* CAN Config Datatype */
struct cannelloni_config_t {
    can_timing_config_t bitrate;
    can_mode_t can_mode;
    int filter;
    int is_extended;
    int start_id;
    int end_id;
} config;

bool can_started = false;

/* ------------ TIMER ------------ */
volatile int interrupt_counter;
volatile bool send_event = false;
int total_interrupt_counter;

hw_timer_t *send_udp_timer = NULL;
portMUX_TYPE timer_mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
    portENTER_CRITICAL_ISR(&timer_mux);
    send_event = true;
    //interrupt_counter++;
    portEXIT_CRITICAL_ISR(&timer_mux);
}

bool event_is_present() {
    return send_event;
}

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
/* ------------ HTTP ------------- */
#ifdef __USE_HTTP_CONFIG__
const char* html =
#include "html.h"
  ;

WiFiServer webui_server(HTTP_PORT);
static TaskHandle_t http_task_handle;

void http() {
    WiFiClient client;
    webui_server.begin();
    String header;
    while (1) {
        client = webui_server.available();
        if (client.connected()) {
            Serial.println("Client connected...");
            String current_line = "";
            while (client.connected()) {
                if (client.available()) {
                    char c = client.read();
                    header += c;
                    if (c == '\n') {
                        if (current_line.length() == 0) {
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:text/html");
                            client.println("Connection: close");
                            client.println();

                            client.println(html);
                            break;
                        }
                        else {
                            current_line = "";
                        }
                    }
                    else if (c != '\r') {
                        current_line += c;
                    }
                }
            }
            Serial.println(header);

            header = "";
            client.stop();
            Serial.println("Client disconnected...");
            Serial.println("");
        }
        else {
            delay(200);
        }
    }
}

void http_task(void *pv_parm) {
    http();
    vTaskDelete(NULL);
}

void http_start() {
    xTaskCreate(http_task, "http_task", 15000, NULL, 1, &http_task_handle); // 2
}

void http_stop() {
    vTaskDelete(http_task_handle);
}

#endif //  __USE_HTTP_CONFIG__


/* ------------ Websocket ---------------*/
#ifdef __USE_WEBSOCKET_CONFIG__
WiFiServer web_server(WEBSOCKET_PORT);
WebSocketServer websocket_server;
static TaskHandle_t websocket_recv_task_handle;


void handle_recv_message(String message) {
    StaticJsonBuffer<500> json_buffer;

    JsonObject &parsed = json_buffer.parseObject(message);
    if (!parsed.success()) {
        Serial.println("Json parsing failed");
        return;
    }

    int bitrate = parsed["bitrate"];
    settings_write("bitrate", &config, &bitrate);

    int can_mode = parsed["can_mode"];
    settings_write("can_mode", &config, &can_mode);

    int filter = parsed["filter"];
    settings_write("filter", &config, &filter);

    int is_extended = parsed["is_extended"];
    settings_write("is_extended", &config, &is_extended);

    int start_id = parsed["start_id"];
    settings_write("start_id", &config, &start_id);

    int end_id = parsed["end_id"];
    settings_write("end_id", &config, &end_id);

    Serial.println();
    Serial.println("----- NEW DATA FROM CLIENT ----");

    Serial.print("bitrate: ");
    Serial.println(bitrate);

    Serial.print("can_mode: ");
    Serial.println(config.can_mode);

    Serial.print("filter: ");
    Serial.println(config.filter);

    Serial.print("is extended: ");
    Serial.println(config.is_extended);

    Serial.print("start_id: ");
    Serial.println(config.start_id);

    Serial.print("end_id: ");
    Serial.println(config.end_id);

    Serial.println("------------------------------");
    setup_can_driver(&config);

}

void websocket_recv() {
    web_server.begin();
    WiFiClient client;
    while (1) {
        client = web_server.available();
        if (client.connected() && websocket_server.handshake(client)) {
            String data;

            while (client.connected()) {
                data = websocket_server.getData();
                if (data.length() > 0) {
                    handle_recv_message(data);
                }
                delay(10);
            }
            Serial.println("The client disconnected");
            delay(100);
        }
        delay(100);
    }
}

void websocket_recv_task(void *pv_parm) {
    websocket_recv();
    vTaskDelete(NULL);
}

void websocket_start() {
    xTaskCreate(websocket_recv_task, "websocket_recv_task", 15000, NULL, 1, &websocket_recv_task_handle);
}

void websocket_stop() {
    vTaskDelete(websocket_recv_task_handle);
}
#endif // __USE_WEBSOCKET_CONFIG__

/* ------------ WIFI ------------ */
void setup_socket() {
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        Serial.println("count not create socket");
        return;
    }

    int yes = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        Serial.printf("could not set socket option: %d", errno);
        Serial.println();
        return;
    }

    memset((char *) &local_address, 0, sizeof(local_address));
    local_address.sin_family = AF_INET;
    local_address.sin_len = sizeof(local_address);
    local_address.sin_port = htons(LOCAL_PORT);
    local_address.sin_addr.s_addr = (in_addr_t)ap_ip;

    if (bind(udp_socket, (struct sockaddr*)&local_address, sizeof(local_address)) == -1) {
        Serial.printf("could not bind socket: %d", errno);
        Serial.println();
        return;
    }
}

void setup_ap() {
    const char * ssid = SSID;
    const char * pswd = PSWD;

    // delete the old configuration
    WiFi.disconnect(true);

    Serial.println("Setting AP [Access Point]...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid, pswd);
    ap_ip = WiFi.softAPIP();
    Serial.println("AP IP-Address: ");
    Serial.println(ap_ip);
}

/* ------------ CAN driver ------------ */





void set_bitrate(cannelloni_config_t *config, int bitrate) {
    switch (bitrate) {
    case 1000:
        config->bitrate = CAN_TIMING_CONFIG_1MBITS();
        break;
    case 800:
        config->bitrate = CAN_TIMING_CONFIG_800KBITS();
        break;
    case 500:
        config->bitrate = CAN_TIMING_CONFIG_500KBITS();
        break;
    case 250:
        config->bitrate = CAN_TIMING_CONFIG_250KBITS();
        break;
    case 125:
        config->bitrate = CAN_TIMING_CONFIG_125KBITS();
        break;
    case 100:
        config->bitrate = CAN_TIMING_CONFIG_100KBITS();
        break;
    case 50:
        config->bitrate = CAN_TIMING_CONFIG_50KBITS();
        break;
    default:  // TODO: Invalid Input print error in log or Serial Port
        config->bitrate = CAN_TIMING_CONFIG_500KBITS();
        break;
    }
}

void set_can_mode(cannelloni_config_t *config, int can_mode) {
    switch (can_mode) {
    case 0:
        config->can_mode = CAN_MODE_NORMAL;
        break;
    case 1:
        config->can_mode = CAN_MODE_NO_ACK;
        break;
    case 2:
        config->can_mode = CAN_MODE_LISTEN_ONLY;
        break;
    default:  // TODO: Invalid Input print error in log or Serial Port
        config->can_mode = CAN_MODE_NORMAL;
        break;
    }
}

void settings_write(String key, cannelloni_config_t *config, void *value) {
    if (key == "bitrate") {
        set_bitrate(config, *((int *) value));
    }
    else if (key == "can_mode") {
        set_can_mode(config, *((int *) value));
    }
    else if (key == "filter") {
        config->filter = *((int *) value);
    }
    else if (key == "is_extended") {
        config->is_extended = *((int *) value);
    }
    else if (key == "start_id") {
        config->start_id = *((int *) value);
    }
    else if (key == "end_id") {
        config->end_id = *((int *) value);
    }
    else {
        Serial.println("Error! Keyword is not defined!");
        return;
    }
}

#define CAN_GENERAL_CONFIG(tx_io_num, rx_io_num, op_mode) { .mode = op_mode, .tx_io = tx_io_num, .rx_io = rx_io_num,   \
                                                            .clkout_io = (gpio_num_t)CAN_IO_UNUSED, .bus_off_io = (gpio_num_t)CAN_IO_UNUSED,      \
                                                            .tx_queue_len = 100, .rx_queue_len = 65,                   \
                                                            .alerts_enabled = CAN_ALERT_NONE, .clkout_divider = 0,         }
                                   
#define CAN_FILTER_CONFIG(accept_code, accept_mask) {.acceptance_code = accept_code, .acceptance_mask = accept_mask, .single_filter = true}

uint32_t calc_acceptance_mask(uint32_t start_id, uint32_t end_id, bool is_extended) {
    uint32_t acceptance_mask = 0x0;
    const uint8_t offset_standard = 0x15;
    const uint8_t offset_extended = 0x3;
    const uint32_t stuffing_standard = 0x1FFFFF;
    const uint32_t stuffing_extended = 0x7;

    if (start_id > end_id || end_id >= (is_extended ? 0x1FFFFFFF : 0x7FF)) {  // invalid start id and end id
        // TODO: Error Handling
        return 0xFFFFFFFF;  // accept all arbitration ID's
    }

    for (uint32_t id = end_id; id >= start_id; id--) {
        acceptance_mask |= (end_id ^ id);
    }

    acceptance_mask <<= (is_extended ? offset_extended : offset_standard);
    acceptance_mask |= (is_extended ? stuffing_extended : stuffing_standard);
    return acceptance_mask;
}

void setup_can_driver(cannelloni_config_t *config) {
  esp_err_t error;
  Serial.println("A1\n");
  if (can_started)
  {
    can_started = false;  
    error = can_stop();
    
    Serial.println("A1.2\n");
    delay(100);
    error = can_driver_uninstall();
    delay(100);
    Serial.println("A1.3\n");
    
  }
  can_filter_config_t filter_config;
  Serial.println("A2\n");
  
  can_general_config_t general_config = CAN_GENERAL_CONFIG(GPIO_NUM_5, GPIO_NUM_4, config->can_mode);
  can_timing_config_t timing_config = config->bitrate;
  Serial.println("A3\n");

  if (config->filter) {
    unsigned int acceptance_code = config->start_id;
    unsigned int acceptance_mask = calc_acceptance_mask(config->start_id, config->end_id, config->is_extended);
    filter_config = CAN_FILTER_CONFIG(acceptance_code, acceptance_mask);
  }
  else {
    filter_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
  }
  
  error = can_driver_install(&general_config, &timing_config, &filter_config);
  Serial.println("A4\n");
  if (error == ESP_OK) {
    ESP_LOGI(TAG, "Diver install was successful");
  }
  else {
    ESP_LOGE(TAG, "Driver install failed");
    Serial.println("Driver install failed");
    Serial.println(error);
    return;
  }
  Serial.println("A5\n");

  // start CAN driver
  error = can_start();
  Serial.println("A6\n");

  if (error == ESP_OK) {
    Serial.println("Driver start...");
    ESP_LOGI(TAG, "Driver start was successful");
    can_started = true;
  }
  else {
    ESP_LOGE(TAG, "Driver start failed");
    return;
  }
}

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

static char ESP_TAG[] = "CAN";

static uint8_t seq_no;
static uint8_t udp_rx_packet_buf[CANNELLONI_UDP_RX_PACKET_BUF_LEN];
static uint8_t udp_tx_packet_buf[CANNELLONI_UDP_TX_PACKET_BUF_LEN]; // CANNELLONI_DATA_PACKET_BASE_SIZE + CANNELLONI_FRAME_BASE_SIZE + 8
static TaskHandle_t cannelloni_can_task_handle;
static TaskHandle_t cannelloni_udp_task_handle;
static TaskHandle_t cannelloni_udp_queue_task_handle;
static TaskHandle_t cannelloni_can_queue_task_handle;
static QueueHandle_t udp_rx_queue_handle;
static QueueHandle_t can_rx_queue_handle;

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

size_t cannelloni_build_packet(uint8_t *buffer, can_message_t *frames, int frame_count) {     //POSIX alternative ssize_t or Standard C size_t
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
            while (queue_size > 0 && can_started) {
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
      if(can_started)
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


/**/
void init_default(void)
{
    set_bitrate(&config, DEFAULT_CAN_BITRATE);
    set_can_mode(&config, DEFAULT_CAN_MODE);
    config.filter = 0;
    config.is_extended = 0;
    config.start_id = 0;
    config.end_id = 2047;
}

/* ------------ MAIN ------------ */
void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    init_default();
    setup_ap();
    setup_socket();
#ifdef __USE_HTTP_CONFIG__
    http_start();
#endif 

#ifdef __USE_WEBSOCKET_CONFIG__
    websocket_start();
#endif

    setup_timer();
    cannelloni_init();
    cannelloni_start();
}

void loop() {
    delay(200);
}
