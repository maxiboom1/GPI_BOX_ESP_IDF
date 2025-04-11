#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include "driver/gpio.h"

esp_err_t init_gpio_pins(void);
void handle_gpio_input_change(gpio_num_t gpio, int level);
void trigger_gpo(uint8_t gpo_num, bool state);

// GPIO state getters
bool get_gpi_state(uint8_t index);
bool get_gpo_state(uint8_t index); 

// Number of configured GPI and GPO getters. Those is used to keep the response dynamic , if we decide to change GPO pins from 8 it total to 5
// Our response will always send only configured pins.
uint8_t get_gpi_count(void);
uint8_t get_gpo_count(void);
