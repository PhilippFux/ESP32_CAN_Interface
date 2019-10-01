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

#include "CAN_Interface.h"
#include "nvm.h"
#include "cannelloni.h"
#include "websocket_config.h"
#include "can_driver_handler.h"
IPAddress remote_ip;

struct cannelloni_config_t config;

Nvm config_persistor;

/**/
void init_default(void)
{
    set_bitrate(&config, DEFAULT_CAN_BITRATE);
    set_can_mode(&config, DEFAULT_CAN_MODE);
    config.filter = 0;
    config.is_extended = 0;
    config.start_id = 0;
    config.end_id = 2047;

    config_persistor.init(sizeof(config), reinterpret_cast<uint8_t *>(&config));
    config_persistor.read();
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
    setup_can_driver(&config);
    cannelloni_init();
    cannelloni_start();
}

void loop() {
  if ( config_persistor.get_update_flag() )
  {
	  config_persistor.write();
      Serial.print("Nvm written ");      
      config_persistor.reset_update();
  }
  delay(200);
}
