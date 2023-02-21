#include "esp_heap_caps.h"
#include "esp_heap_trace.h"
#include "esp_heap_task_info.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

#define LOOP 10
#define SIZE 1000
#define MAX_TASK_NUM 20                         // Max number of per tasks info that it can store
#define MAX_BLOCK_NUM 20                        // Max number of per block info that it can store

static const char* TAG = "HEAP-TRACE-EXAMPLE";
int i = 0;
int leaked = 0;

static size_t s_prepopulated_num = 0;
static heap_task_totals_t s_totals_arr[MAX_TASK_NUM];
static heap_task_block_t s_block_arr[MAX_BLOCK_NUM];

static void esp_dump_per_task_heap_info(void)
{
    heap_task_info_params_t heap_info = {0};
    heap_info.caps[0] = MALLOC_CAP_8BIT;        // Gets heap with CAP_8BIT capabilities
    heap_info.mask[0] = MALLOC_CAP_8BIT;
    heap_info.caps[1] = MALLOC_CAP_32BIT;       // Gets heap info with CAP_32BIT capabilities
    heap_info.mask[1] = MALLOC_CAP_32BIT;
    heap_info.tasks = NULL;                     // Passing NULL captures heap info for all tasks
    heap_info.num_tasks = 0;
    heap_info.totals = s_totals_arr;            // Gets task wise allocation details
    heap_info.num_totals = &s_prepopulated_num;
    heap_info.max_totals = MAX_TASK_NUM;        // Maximum length of "s_totals_arr"
    heap_info.blocks = s_block_arr;             // Gets block wise allocation details. For each block, gets owner task, address and size
    heap_info.max_blocks = MAX_BLOCK_NUM;       // Maximum length of "s_block_arr"

    heap_caps_get_per_task_info(&heap_info);

    for (int i = 0 ; i < *heap_info.num_totals; i++) {
        printf("Task: %s -> CAP_8BIT: %d CAP_32BIT: %d\n",
                heap_info.totals[i].task ? pcTaskGetTaskName(heap_info.totals[i].task) : "Pre-Scheduler allocs" ,
                heap_info.totals[i].size[0],    // Heap size with CAP_8BIT capabilities
                heap_info.totals[i].size[1]);   // Heap size with CAP32_BIT capabilities
    }

    printf("\n\n");
}

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
