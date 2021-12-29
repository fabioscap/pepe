#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
// maximun schedules in a day
#define MAX_FEED_IN_A_DAY 8

typedef struct queue_element {
    uint8_t hour;
    uint8_t minute;
    time_t activation_time;
} queue_element_t;

typedef struct schedule_list {
    queue_element_t list[MAX_FEED_IN_A_DAY];
    uint8_t size;
    SemaphoreHandle_t schedule_semaphore;
} schedule_list_t;

typedef schedule_list_t* schedule_list_handler_t;

void print_feed_schedule(queue_element_t* fs, uint8_t n);
schedule_list_handler_t init_feed_schedule();