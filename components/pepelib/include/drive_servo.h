#pragma once

#include "stdint.h" // uint16_t
#include <driver/mcpwm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>

#define SERVO_GPIO 4
#define MAX_PWM_DURATION_MS 100000 // 100 seconds max
#define SERVO_OFF 0
#define SERVO_QUEUE_BUFFER_SIZE 10
#define SERVO_TASK_PRIORITY 11 // 
#define SERVO_US_MAX 2000
#define SERVO_US_MIN 1000
#define SERVO_ROUTINE_SLEEP_TICKS 1000/portTICK_RATE_MS
#define TAG "my_servo"

typedef struct servo_handle {
    TaskHandle_t task;
    QueueHandle_t queue;
} servo_handle;

typedef servo_handle* servo_handle_t;

servo_handle_t my_servo_init();
void servo_set_duty_us_blocking(uint16_t us, uint16_t ticks);
void servo_enq_duty_us_ms(servo_handle_t servo,uint16_t _us, uint16_t ms);