#include "esp_heap_caps.h"
#include "esp_heap_trace.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

#define LOOP 10
#define SIZE 1000

static const char* TAG = "HEAP-TRACE-EXAMPLE";

int i = 0;
int leaked = 0;

static void print_heap_info(char* tag)
{
    printf("===================== FREE HEAP @ %s ======================\n", tag);
    size_t free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t total = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    size_t low_watermark = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    printf("Total free: %d/%d %.2f%% used\n", free, total, 100.0 - ((float)free / total * 100));
    printf("Low watermark: %d/%d %.2f%% left\n", low_watermark, total, (float)low_watermark / total * 100);
    printf("%s", "===================================================================\n");
}

/**
 * Leaks (LOOP/2)*SIZE bytes before waiting forever so that trace can be stopped
 *
 */
void app_main(void)
{
    ESP_ERROR_CHECK(heap_trace_init_tohost());

    print_heap_info("SYSTEM INIT");

    ESP_ERROR_CHECK(heap_trace_start(HEAP_TRACE_ALL));

    while (i < LOOP) {
        uint8_t* p = (uint8_t*)malloc(SIZE * sizeof(uint8_t));
        memset(p, 0x23, SIZE);
        print_heap_info("STATUS");
        vTaskDelay(pdMS_TO_TICKS(1000));
        i++;
        if (i % 2 == 0) {
            ESP_LOGI(TAG, "free");
            free(p);
        } else {
            leaked += SIZE;
        }
    }

    ESP_LOGE(TAG, "DONE! Leaked: %d bytes", leaked);

    // Wait forever
    while (1) {
        vTaskDelay(1000);
    }
}
