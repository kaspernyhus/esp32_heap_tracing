#include <stdint.h>
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
static TaskStatus_t task_status[MAX_TASK_NUM];

static void esp_dump_per_task_stack_info(void)
{
    uint32_t runtime = 0;
    UBaseType_t num_tasks = uxTaskGetSystemState(task_status, MAX_TASK_NUM, &runtime);

    printf("\n----------------------------------------------------------\n");
    printf(" Task info for %d tasks\n", num_tasks);
    printf("----------------------------------------------------------\n");
    printf("||  Task name\t\t| Core | Priority | Stack left  ||\n");
    printf("----------------------------------------------------------\n");
    for (int i = 0; i < num_tasks; i++) {
        printf("|| %-16s\t| %-4d | %-8d | %-5d\t||\n", task_status[i].pcTaskName, task_status[i].xCoreID, task_status[i].uxBasePriority, task_status[i].usStackHighWaterMark);
    }
    printf("----------------------------------------------------------\n\n");
}

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

    printf("\n----------------------------------------------------------\n");
    printf(" Heap allocation info for %d tasks", *heap_info.num_totals);
    printf("\n----------------------------------------------------------\n");
    printf("|| Task name\t\t| CAP_8BIT \t| CAP_32BIT\t||\n");
    printf("----------------------------------------------------------\n");
    for (int i = 0; i < *heap_info.num_totals; i++) {
        const char* task_name = heap_info.totals[i].task ? pcTaskGetTaskName(heap_info.totals[i].task) : "Pre-Sched allocs";
        printf("|| %-16s\t| %-5d\t\t| %-8d\t||\n", task_name, heap_info.totals[i].size[0], heap_info.totals[i].size[1]);
    }
    printf("----------------------------------------------------------\n\n");
}

static void esp_dump_system_heap_info(void)
{
    const size_t total_free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const int min_free_8bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    const size_t total_available = heap_caps_get_total_size(MALLOC_CAP_8BIT);

    printf("\n----------------------------------------------------------\n");
    printf("||  Free Memory\t\t\t|  Low Watermark\t||\n");
    printf("----------------------------------------------------------\n");
    printf("||  %-6d/%d %.2f%% used\t|  %-6d %.2f%% left   ||\n", total_free, total_available, 100.0 - ((float)total_free / total_available * 100), min_free_8bit_cap, (float)min_free_8bit_cap / total_available * 100);
    printf("----------------------------------------------------------\n\n");
}

static void example_task(void* pvparams)
{
    uint32_t size = 0;
    while (1) {
        /*
         * Allocate random amount of memory for demonstration
         */
        size = (esp_random() % 1000);
        void* ptr = malloc(size);
        if (ptr != NULL) {
            if (size < 500) {
                free(ptr);
            }
        } else {
            ESP_LOGE("example_task", "Could not allocate heap memory");
        }

        vTaskDelay(pdMS_TO_TICKS(2000 + size));
    }
}

static void waiting_task(void* pvParam)
{
    ESP_LOGI(TAG, "Just waiting...");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static void heap_tracking_task(void* pvparams)
{
    while (1) {
        vTaskDelay(5000);
        esp_dump_per_task_stack_info();
        esp_dump_per_task_heap_info();
        esp_dump_system_heap_info();
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "===== APP_MAIN =====");

    xTaskCreatePinnedToCore(example_task, "example_task1", 926, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(example_task, "example_task2", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(example_task, "example_task3", 8192, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(example_task, "example_task4", 4096, NULL, 7, NULL, 0);
    xTaskCreatePinnedToCore(waiting_task, "waiting_task", 4096, NULL, 5, NULL, 0);

    xTaskCreatePinnedToCore(heap_tracking_task, "heap_tracking_task", 8192, NULL, 5, NULL, 1);

    vTaskSuspend(NULL);
}
