#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include "driver/gpio.h"

esp_err_t init_gpio_pins(void);
void handle_gpio_input_change(gpio_num_t gpio, int level);
void set_gpo_state(uint8_t index, bool state);
bool get_gpi_state(uint8_t index);