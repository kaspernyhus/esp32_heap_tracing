#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include "esp_heap_caps.h"
#include "esp_heap_task_info.h"
#include "esp_heap_trace.h"
#include "esp_log.h"

#define LOOP 10
#define SIZE 1000
#define MAX_TASK_NUM 20 // Max number of per tasks info that it can store
#define MAX_BLOCK_NUM 20 // Max number of per block info that it can store

static const char* TAG = "HEAP";
int i = 0;
int leaked = 0;

static size_t s_prepopulated_num = 0;
static heap_task_totals_t s_totals_arr[MAX_TASK_NUM];
static heap_task_block_t s_block_arr[MAX_BLOCK_NUM];

static void esp_dump_per_task_heap_info(void)
{
    heap_task_info_params_t heap_info = { 0 };
    heap_info.caps[0] = MALLOC_CAP_8BIT; // Gets heap with CAP_8BIT capabilities
    heap_info.mask[0] = MALLOC_CAP_8BIT;
    heap_info.caps[1] = MALLOC_CAP_32BIT; // Gets heap info with CAP_32BIT capabilities
    heap_info.mask[1] = MALLOC_CAP_32BIT;
    heap_info.tasks = NULL; // Passing NULL captures heap info for all tasks
    heap_info.num_tasks = 0;
    heap_info.totals = s_totals_arr; // Gets task wise allocation details
    heap_info.num_totals = &s_prepopulated_num;
    heap_info.max_totals = MAX_TASK_NUM; // Maximum length of "s_totals_arr"
    heap_info.blocks = s_block_arr; // Gets block wise allocation details. For each block, gets owner task, address and size
    heap_info.max_blocks = MAX_BLOCK_NUM; // Maximum length of "s_block_arr"
    heap_caps_get_per_task_info(&heap_info);

    printf("\n--------------------------------------------------------------------------\n");
    printf(" Task info for %d tasks", *heap_info.num_totals);
    printf("\n--------------------------------------------------------------------------\n");
    printf("|| Task name\t\t| 8BIT \tHeap allocated  32BIT   | Stack left\t||\n");
    printf("--------------------------------------------------------------------------\n");

    for (int i = 0; i < *heap_info.num_totals; i++) {
        const char* task_name = heap_info.totals[i].task ? pcTaskGetTaskName(heap_info.totals[i].task) : "Pre-Sched allocs";

        // returns the minimum amount of remaining stack space - that is the amount of stack that remained unused when the task stack was at its greatest (deepest) value.
        int task_HWM = uxTaskGetStackHighWaterMark(heap_info.totals[i].task);
        // int task_HWM = 0;

        printf("|| %-16s\t| %-5d\t\t| %-8d\t| %-9d\t||\n", task_name, heap_info.totals[i].size[0], heap_info.totals[i].size[1], task_HWM);
    }

    const size_t free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const int min_free_8bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t total = heap_caps_get_total_size(MALLOC_CAP_8BIT);
    printf("--------------------------------------------------------------------------\n");
    printf("System Memory Utilisation Stats:\n");
    printf("----------------------------------------------------------\n");
    printf("||  Free Memory\t\t\t|  Low Watermark\t||\n");
    printf("||  %-6d/%d %.2f%% used\t|  %-6d %.2f%% left   ||\n", free, total, 100.0 - ((float)free / total * 100), min_free_8bit_cap, (float)min_free_8bit_cap / total * 100);
    printf("----------------------------------------------------------\n\n");
}

static void example_task(void* pvparams)
{
    uint32_t size = 0;
    const char* TAG = "example_task";
    while (1) {
        /*
         * Allocate random amount of memory for demonstration
         */
        size = (esp_random() % 1000);
        void* ptr = malloc(size);
        if (ptr == NULL) {
            ESP_LOGE(TAG, "Could not allocate heap memory");
            abort();
        }
        vTaskDelay(pdMS_TO_TICKS(2000 + size));
    }
}

static void heap_tracking_task(void* pvparams)
{
    while (1) {
        vTaskDelay(2000);
        esp_dump_per_task_heap_info();
        // print_heap_info("tracking task");
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "===== APP_MAIN =====");

    xTaskCreatePinnedToCore(example_task, "example_task1", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(example_task, "example_task2", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(example_task, "example_task3", 8192, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(heap_tracking_task, "heap_tracking_task", 8192, NULL, 5, NULL, 1);

    vTaskSuspend(NULL);
}
