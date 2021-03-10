#ifndef CAN_INTERFACE_H
#define CAN_INTERFACE_H

#include "can_driver_handler.h"

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


extern cannelloni_config_t config;

#endif
