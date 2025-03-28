#include "gpio_handler.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "app_config.h"
#include "message_builder.h"
#include "tcp_client.h"
#include "http_client.h"
#include <string.h>
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"  // Needed for esp_timer_get_time()

#define TAG "GPIO_HANDLER"
#define DEBOUNCE_DELAY_MS 50

static int last_stable_state[8] = {0};
static int last_debounce_state[8] = {0};
static int64_t last_debounce_time[8] = {0};

static QueueHandle_t gpio_evt_queue = NULL;

static void gpio_task(void *arg);

// GPIO allocations
static const gpio_num_t gpi_pins[8] = {
    GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_25, GPIO_NUM_26,
    GPIO_NUM_27, GPIO_NUM_14, GPIO_NUM_12, GPIO_NUM_13
};

// GPO reduced to 5 safe pins (excluding GPIO 2, 5, 12)
static const gpio_num_t gpo_pins[5] = {GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_4};

// Stores last known GPI states
static bool gpi_states[8] = {0};

// ISR handler for GPI pin change -triggered by esp each time the pin state changed 
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    int index = (int) arg;
    xQueueSendFromISR(gpio_evt_queue, &index, NULL);
}

esp_err_t init_gpio_pins(void) {
    ESP_LOGI(TAG, "Initializing GPIO pins...");
    
    gpio_evt_queue = xQueueCreate(10, sizeof(int));
    xTaskCreate(gpio_task, "gpio_task", 4096, NULL, 10, NULL);
    
    // Enable ISR service
    gpio_install_isr_service(0);
    
    // Configure GPI pins
    for (int i = 0; i < 8; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << gpi_pins[i],
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_ANYEDGE
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_isr_handler_add(gpi_pins[i], gpio_isr_handler, (void*) i);
        gpi_states[i] = gpio_get_level(gpi_pins[i]);
    }

    // Configure GPO pins
    for (int i = 0; i < 5; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = 1ULL << gpo_pins[i],
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_set_level(gpo_pins[i], 0);
    }

    return ESP_OK;
}

void handle_gpio_input_change(gpio_num_t gpio, int level) {
    char msg[256];
    const char* state = level ? "HIGH" : "LOW";

    // Determine GPI index for name (e.g., "GPI01")
    int index = -1;
    for (int i = 0; i < 8; i++) {
        if (gpio == gpi_pins[i]) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        ESP_LOGW(TAG, "Unknown GPI pin triggered");
        return;
    }

    char event_name[8];
    snprintf(event_name, sizeof(event_name), "GPI%02d", index + 1);  // e.g., GPI01
    ESP_LOGI(TAG, "Trigger:%s",event_name);
    if (globalConfig.companionMode) {
        construct_message(event_name, state, "", "", msg, sizeof(msg));
        tcp_client_send(msg);
        return;
    }

    if (globalConfig.serialEnabled) {
        construct_message(event_name, state, "", "", msg, sizeof(msg));
        printf("%s", msg);
    }

    if (globalConfig.tcpEnabled) {
        construct_message(event_name, state, globalConfig.tcpUser, globalConfig.tcpPassword, msg, sizeof(msg));
        tcp_client_send(msg);
    }

    if (globalConfig.httpEnabled) {
        construct_message(event_name, state, globalConfig.httpUser, globalConfig.httpPassword, msg, sizeof(msg));
        send_http_post(msg);
    }
}

void set_gpo_state(uint8_t index, bool state) {
    if (index < 5) {
        gpio_set_level(gpo_pins[index], state);
    }
}

bool get_gpi_state(uint8_t index) {
    return (index < 8) ? gpi_states[index] : false;
}

static void gpio_task(void *arg) {
    int index;
    while (1) {
        // Wait for an event to know which pin to check
        if (xQueueReceive(gpio_evt_queue, &index, portMAX_DELAY)) {
            while (1) {
                int level = gpio_get_level(gpi_pins[index]); // Read current level
                int64_t now = esp_timer_get_time() / 1000;   // Current time in ms

                if (level != last_debounce_state[index]) {
                    // Level changed: start (or restart) debounce timer
                    last_debounce_state[index] = level;
                    last_debounce_time[index] = now;
                } else {
                    // Same level as before, check if debounce period has passed
                    if ((now - last_debounce_time[index]) >= DEBOUNCE_DELAY_MS) {
                        if (level != last_stable_state[index]) {
                            last_stable_state[index] = level;
                            gpi_states[index] = level;
                            handle_gpio_input_change(gpi_pins[index], level);
                        }
                        break; // Exit inner loop once state is stable
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to avoid busy-waiting
            }
        }
    }
}