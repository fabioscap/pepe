#include <drive_servo.h>

void _servo_stop() {
    ESP_ERROR_CHECK(mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A));
}

typedef struct servo_queue_message {
    uint16_t us;
    uint16_t ticks;
} servo_queue_message_t;

// wakes up every SERVO_ROUTINE_SLEEP_TICKS ticks then checks for messages in the queue
// maybe there's a way to wake a task up when there is a new message and have it sleep all
// the time when there's no new message
// (to try: free rtos task notify)
void my_servo_routine(void* params) {
    QueueHandle_t q = (QueueHandle_t) params;
    servo_queue_message_t msg;
    while(1) {
        if (xQueueReceive(q, &msg, 10) == pdTRUE) { // wait 10 ticks
            // there is a new message
            ESP_LOGI(TAG,"new command...");
            ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, msg.us));
            vTaskDelay(msg.ticks);
            _servo_stop();
        }
        else {
            vTaskDelay(SERVO_ROUTINE_SLEEP_TICKS);
        }
    }
}

servo_handle_t my_servo_init() {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_GPIO);
    mcpwm_config_t pwm_config = {
        .frequency = 50,
        .cmpr_a = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    //ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, us));

    // create queue
    QueueHandle_t servo_queue = xQueueCreate(SERVO_QUEUE_BUFFER_SIZE, // queue length
                              sizeof(servo_queue_message_t));

    // create task
    TaskHandle_t servo_task = NULL;
    xTaskCreate(my_servo_routine,"servo_routine",4096,servo_queue,SERVO_TASK_PRIORITY,&servo_task);
    
    servo_handle_t s_h = (servo_handle_t)malloc(sizeof(servo_handle));
    s_h->queue = servo_queue;
    s_h->task = servo_task;
    return s_h;
}
/*
// blocking
void servo_cw(uint16_t ticks) {
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, SERVO_CW));
    vTaskDelay(ticks);
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, SERVO_OFF));
}

void servo_ccw(uint16_t ticks) {
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, SERVO_CCW));
    vTaskDelay(ticks);
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, SERVO_OFF));
}
*/
void servo_set_duty_us_blocking(uint16_t us, uint16_t ticks) {
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, us));
    vTaskDelay(ticks);
    _servo_stop();
}

void servo_enq_duty_us(servo_handle_t servo,uint16_t us, uint16_t ticks) {
    if (us < SERVO_US_MIN || us > SERVO_US_MAX) {
        ESP_LOGE(TAG,"invalid pwm command...");
        return;
    }
    servo_queue_message_t msg = {
        .us = us, .ticks = ticks
    };
    xQueueSendToBack(servo->queue,&msg,portMAX_DELAY);
}