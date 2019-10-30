#include "can_driver_handler.h"

bool can_started = false;

bool is_can_started() {
  return can_started;
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
  if (can_started)
  {
    can_started = false;  
    error = can_stop();
    delay(100);
    error = can_driver_uninstall();
    delay(100);
  }
  can_filter_config_t filter_config;
  
  can_general_config_t general_config = CAN_GENERAL_CONFIG(GPIO_NUM_5, GPIO_NUM_4, config->can_mode);
  can_timing_config_t timing_config = config->bitrate;

  if (config->filter) {
    unsigned int acceptance_code = config->start_id;
    unsigned int acceptance_mask = calc_acceptance_mask(config->start_id, config->end_id, config->is_extended);
    filter_config = CAN_FILTER_CONFIG(acceptance_code, acceptance_mask);
  }
  else {
    filter_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
  }
  
  error = can_driver_install(&general_config, &timing_config, &filter_config);

  if (error == ESP_OK) {
    ESP_LOGI(TAG, "Diver install was successful");
  }
  else {
    ESP_LOGE(TAG, "Driver install failed");
    Serial.println("Driver install failed");
    Serial.println(error);
    return;
  }

  // start CAN driver
  error = can_start();

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
