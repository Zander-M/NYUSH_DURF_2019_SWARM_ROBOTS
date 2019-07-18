/* PID (Position) Example
   Using encoder to move the robot forward, backward, left, right


   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "driver/pcnt.h"
#include "esp_log.h"
#include "freertos/task.h"

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

/*
 * ESP32 PCNT PID function
 * 
 * Use PCNT module to monitor how many round does motors rotated.
 * 
 */

/*
 * Motor init
 */
#define L1 11
#define L2 12
#define R1 13
#define R2 15
#define L_SPD 50.0 // speed in percentage. 100 for full speed
#define R_SPD 50.0
volatile int handler_count[2] {0, 0}; // handler counter
volatile int count[2] = {50, 50}; // counter
volatile int spd[2] = {50, 50};   // speed

/*
 * PCNT Module init
 */
#define PCNT_UNIT             PCNT_UNIT_0
#define PCNT_LEFT_IO          16
#define PCNT_RIGHT_IO         17
#define PCNT_H_LIM_VAL        1000
#define PCNT_L_LIM_VAL       -1000
#define PCNT_THRESH_VAL       50 // every 50 round, check speed 

volatile int count = 0; // interrupt will change the value, it must be volatile
xQueueHandle pcnt_evt_queue;
pcnt_isr_handle_t user_isr_handle = NULL;

/*
 * Motor init
 */

static void Motor_init()
{
    pirntf("initializing mcpwm gpio...\n");
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, L1);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, L2);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1A, R1);
    mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM1B, R2);
}

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
/*
 * structure to pass the event from PCNT interrupt handler to main program
 */
typedef struct {
    int unit; // 0 for left, 1 for right
    uint32_t status; // information about the event
} pcnt_evt_t;

static void IRAM_ATTR pcnt_intr_handler(void *arg)
{
    uint32_t intr_status = PCNT.int_st.val;
    int i;
    pcnt_evt_t evt;
    portBASE_TYPE HPTaskAwoken = pdFALSE;

    for (i = 0; i< PCNT_UNIT_MAX; i++) {
        if (intr_status & (BIT(i))) {
            evt.unit = i;
            evt.status = PCNT.status_unit[i].val;
            PCNT.int_clr.val = BIT(i);
            xQueueSendFromISR(pcnt_evt_queue, &evt, &HPTaskAwoken);
            if (HPTaskAwoken == pdTRUE) {
                portYIELD_FROM_ISR();
            }
        }
    }
}

/*
 * Pulse Counter
 */
static void pcnt_init(void)
{
    // initialize pcnt module
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = PCNT_LEFT_IO,
        .channel = PCNT_CHANNEL_0,
        .unit = PCNT_UNIT,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_INC,
        .counter_h_lim = PCNT_H_LIM_VAL,
        .counter_l_lim = PCNT_L_LIM_VAL,
    };
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    /* Configure & enable filter*/
    pcnt_set_filter_value(PCNT_UNIT, 100);
    pcnt_filter_enable(PCNT_UNIT);

    /* Set threshold 0 and 1 values and enable events to watch */
    pcnt_set_event_value(PCNT_UNIT, PCNT_EVT_THRES_1, PCNT_THRESH1_VAL);
    pcnt_event_enable(PCNT_UNIT, PCNT_EVT_THRES_1);
    pcnt_set_event_value(PCNT_UNIT, PCNT_EVT_THRES_0, PCNT_THRESH0_VAL);
    pcnt_event_enable(PCNT_UNIT, PCNT_EVT_THRES_0);
    /* Enable events on zero, maximum and minimum limit values */
    pcnt_event_enable(PCNT_UNIT, PCNT_EVT_ZERO);
    pcnt_event_enable(PCNT_UNIT, PCNT_EVT_H_LIM);
    pcnt_event_enable(PCNT_UNIT, PCNT_EVT_L_LIM);
    
    /* Initialize PCNT's counter */
    pcnt_counter_pause(PCNT_UNIT);
    pcnt_counter_clear(PCNT_UNIT);

    /* Register ISR handler and enable interrupts for PCNT unit */
    pcnt_isr_register(pcnt_intr_handler, NULL, 0 , &user_isr_handle);
    pcnt_intr_enable(PCNT_UNIT);
    
    /* Resume counting */
    pcnt_counter_resume(PCNT_UNIT);
}

int dist2rotate(int dist)
{
    /* 
    * From distance in mm to rounds that the motor rotate
    */
   return (int)dist*100/(34*3.14);
}

void balance_spd__handler()
{
    /*
     * Balance the speed of the motors. If one is too slow, change the speed of the other one
     */
    int motor1 = count[0];
    int motor2 = count[1];
    if (motor1 - motor2 >40) {
        stop_motor(motor1);
    } else if (motor2)
}


float Pcnt(int Encoder, int Target){
    /* 
     * Discrete PID control
     * Encoder for encoder reading, Target for target value 
     * PID formula: pwm = Kp*e(k) + Ki*Σe(k) +Kd[e(k) - e(k-1)]
     * e(k) for current error
     * e(k-1) for previous error
     * Σe(k)  for summation of errors
     * pwm is the output
     */
    float Position_Kp = 80, Position_Ki = 0.1, Position_Kd = 500; // PID coefficients, need to be tuned later.
    static float Bias, Pwm, IntergralBias, LastBias;
    Bias = Encoder - Target;
    IntergralBias += Bias;
    Pwm =  Position_Kp*Bias + Position_Ki*IntergralBias + Position_Kd*(Bias-LastBias);
    LastBias = Bias;
    return Pwm;
}

void app_main()
{
    pcnt_evt_queue = xQueueCreate(10, sizeof(pcnt_evt_t));
    pcnt_init();

    int16_t count = 0;
    pcnt_evt_t evt;
    portBASE_TYPE res;
    while (1) {
        res = xQueueReceive(pcnt_evt_queue, &evt, 1000 / portTICK_PERIOD_MS);
        if (res == pdTRUE) {
            pcnt_get_counter_value(PCNT_UNIT, &count);
            printf("Event PCNT unit [%d]: cnt:%d\n", evt.unit, count);
            if (evt.status & PCNT_STATUS_THRES1_M) {
                printf("THRESH1 EVT\n");
            }
            if (evt.status & PCNT_STATUS_THRES0_M) {
                printf("THRESH0 EVT\n");
            }
            if (evt.status & PCNT_STATUS_L_LIM_M) {
                printf("L_LIM EVT\n");
            }
            if (evt.status & PCNT_STATUS_H_LIM_M) {
                printf("H_LIM EVT\n");
            }
            if (evt.status & PCNT_STATUS_ZERO_M) {
                printf("ZERO EVT\n");
            }
        } else {
            pcnt_get_counter_value(PCNT_UNIT, &count);
            printf("Current counter value :%d\n", count);
        }
    }
    if (user_isr_handle) {
        esp_intr_free(user_isr_handle);
        user_isr_handle = NULL;
    }
}
