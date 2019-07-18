/* brushed dc motor control example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * This example will show you how to use MCPWM module to control brushed dc motor.
 * This code is tested with L298 motor driver.
 * User may need to make changes according to the motor driver they use.
*/

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"


#define L1    13
#define L2    12
#define R1    14
#define R2    27
#define L_SPD 50
#define R_SPD 50
// int counter[] = {};

static void MOTOR_INIT()
{
    printf("initializing mcpwm gpio...\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, L1);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, L2);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, R1);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1B, R2);
}

/**
 * @brief motor moves in forward direction, with duty cycle = duty %
 */

static void motor_forward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, float duty_cycle){
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
    mcpwm_set_duty(mcpwm_num, timer_num,MCPWM_OPR_A, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
}

static void motor_backward(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, float duty_cycle){
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    mcpwm_set_duty(mcpwm_num, timer_num,MCPWM_OPR_B, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
}

/**
 * @brief motor moves in backward direction, with duty cycle = duty %
 */

/**
 * @brief motor stop
 */
static void motor_stop(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num)
{
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_A);
    mcpwm_set_signal_low(mcpwm_num, timer_num, MCPWM_OPR_B);
}

static void vForward(){
    motor_forward(MCPWM_UNIT_0, MCPWM_TIMER_0, L_SPD);
    motor_forward(MCPWM_UNIT_1, MCPWM_TIMER_1, R_SPD);
}

static void vBackward(){
    motor_backward(MCPWM_UNIT_0, MCPWM_TIMER_0, L_SPD);
    motor_backward(MCPWM_UNIT_1, MCPWM_TIMER_1, R_SPD);
}

static void vTurnLeft(){
    motor_forward(MCPWM_UNIT_0, MCPWM_TIMER_0, L_SPD);
    motor_backward(MCPWM_UNIT_1, MCPWM_TIMER_1, R_SPD);
}

static void vTurnRight(){
    motor_backward(MCPWM_UNIT_0, MCPWM_TIMER_0, L_SPD);
    motor_forward(MCPWM_UNIT_1, MCPWM_TIMER_1, R_SPD);
}

static void vStop(){
    motor_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
    motor_stop(MCPWM_UNIT_1, MCPWM_TIMER_1);
}
/**
 * @brief Configure MCPWM module for brushed dc motor
 */
static void motor_test(void *arg)
{
    //1. mcpwm gpio initialization
    MOTOR_INIT();
    //2. initial mcpwm configuration
    printf("Configuring Initial Parameters of mcpwm...\n");
    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;    //frequency = 500Hz,
    pwm_config.cmpr_a = 0;    //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings
    mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_1, &pwm_config);    //Configure PWM0A & PWM0B with above settings
    while (1) {
        vForward();
        vTaskDelay(2000 / portTICK_RATE_MS);
        printf("forward\n");
        vBackward();
        vTaskDelay(2000 / portTICK_RATE_MS);
        printf("backward\n");
        vTurnRight();
        vTaskDelay(2000 / portTICK_RATE_MS);
        printf("right\n");
        vTurnLeft();
        vTaskDelay(2000 / portTICK_RATE_MS);
        printf("left\n");
        vStop();
        vTaskDelay(2000 / portTICK_RATE_MS);
        printf("stop\n");
    }
}

void app_main()
{
    printf("Testing brushed motor...\n");
    xTaskCreate(&motor_test, "Motor_Control", 4096, NULL, 5, NULL);
}
