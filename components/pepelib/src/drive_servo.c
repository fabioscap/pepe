#include <drive_servo.h>


#define SERVO_OFF 0 
#define SERVO_CCW 1600
#define SERVO_CW  1400



void my_servo_init() {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_GPIO);

    mcpwm_config_t pwm_config = {
        .frequency = 50,
        .cmpr_a = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    //ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, us));
}
// blocking
void servo_cw(uint8_t ticks) {
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, SERVO_CW));
    vTaskDelay(ticks);
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, SERVO_OFF));
}
void servo_ccw(uint8_t ticks) {
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, SERVO_CCW));
    vTaskDelay(ticks);
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, SERVO_OFF));
}

void servo_pwm(uint8_t pwm, uint8_t ticks) {
    ESP_ERROR_CHECK(mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, pwm));
}