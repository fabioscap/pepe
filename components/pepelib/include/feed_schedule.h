#pragma once

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <time.h>
#include <freertos/semphr.h>
#include <string.h>
#include <esp_log.h>

// maximun schedules in a day
#define MAX_FEED_IN_A_DAY 8
#define TIME_TOLERANCE_SEC 120
#define SCHEDULE_ROUTINE_SLEEP_TIME 10000/portTICK_RATE_MS
#define SCHEDULE_TASK_PRIORITY 10

typedef struct queue_element {
    uint8_t hour;
    uint8_t minute;
    time_t activation_time;
} queue_element_t;

typedef struct schedule_list {
    queue_element_t list[MAX_FEED_IN_A_DAY];
    uint8_t size;
    void (*cb_function)(void*);
    SemaphoreHandle_t smph;
} schedule_list_t;

typedef schedule_list_t* schedule_list_handle_t;

schedule_list_handle_t init_feed_schedule(void (*cb_function)(void*));

void print_feed_schedule(queue_element_t* fs, uint8_t n);

void update_feed_schedule(schedule_list_handle_t sch, queue_element_t* new, uint8_t size);