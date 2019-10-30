#ifndef CAN_DRIVER_HANDLER_H
#define CAN_DRIVER_HANDLER_H

#include <Arduino.h>
#include <FreeRTOS.h>
#include <driver/can.h>

/* CAN Config Datatype */
struct cannelloni_config_t {
    can_timing_config_t bitrate;
    can_mode_t can_mode;
    int filter;
    int is_extended;
    int start_id;
    int end_id;
};

void setup_can_driver(cannelloni_config_t *config);
void set_bitrate(cannelloni_config_t *config, int bitrate);
void set_can_mode(cannelloni_config_t *config, int can_mode);
void settings_write(String key, cannelloni_config_t *config, void *value);

#endif
