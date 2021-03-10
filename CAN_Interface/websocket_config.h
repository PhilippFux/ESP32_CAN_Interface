#ifndef WEBSOCKET_CONFIG_H
#define WEBSOCKET_CONFIG_H

#ifdef __USE_WEBSOCKET_CONFIG__
#include <WebSocketServer.h> // https://github.com/morrissinger/ESP8266-Websocket
#include <ArduinoJson.h>  // Version 5
#endif


void setup_ap();
void setup_socket();
void websocket_start();
void http_start();

#endif
