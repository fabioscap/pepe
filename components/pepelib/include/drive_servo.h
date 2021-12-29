#pragma once

#ifndef SERVO_GPIO
#define SERVO_GPIO 4
#endif

#include "stdint.h" // uint8_t
#include <driver/mcpwm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void my_servo_init();
// blocking
void servo_cw(uint8_t ticks);
void servo_ccw(uint8_t ticks);
void servo_pwm(uint8_t pwm, uint8_t ticks);


