/*  MUX test

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   Test if CD4051 is working properly by switching pins to light up a LED
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "sdkconfig.h"
#include "esp_log.h"

/*
 * Using multiplexer CD4051, reading resistor ~220Ω.
 * A simple test.
 */

#define DEFAUT_VREF     1100
#define NO_OF_SAMPLES   64
#define IR_GPIO 0
#define MUX1    5
#define MUX2    18
#define MUX3    19
#define MUX_OUT ((1ULL<<MUX1) | (1ULL<<MUX2) | (1ULL<<MUX3))

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

// static void check_efuse()
// {
//     //Check TP is burned into eFuse
//     if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
//         printf("eFuse Two Point: Supported\n");
//     } else {
//         printf("eFuse Two Point: NOT supported\n");
//     }

//     //Check Vref is burned into eFuse
//     if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
//         printf("eFuse Vref: Supported\n");
//     } else {
//         printf("eFuse Vref: NOT supported\n");
//     }
// }

// static void print_char_val_type(esp_adc_cal_value_t val_type)
// {
//     if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
//         printf("Characterized using Two Point Value\n");
//     } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
//         printf("Characterized using eFuse Vref\n");
//     } else {
//         printf("Characterized using Default Vref\n");
//     }
// }


void app_main()
{
    // config mux
    // check_efuse();
    // if (unit == ADC_UNIT_1) {
    //     adc1_config_width(ADC_WIDTH_BIT_12);
    //     adc1_config_channel_atten(channel, atten);
    // } else {
    //     adc2_config_channel_atten((adc2_channel_t)channel, atten);
    // }

    //Characterize ADC
    // adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    // esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAUT_VREF, adc_chars);
    // print_char_val_type(val_type);
    printf("working\n");
    gpio_config_t mux_conf;
    mux_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    mux_conf.mode = GPIO_MODE_OUTPUT;
    mux_conf.pin_bit_mask = MUX_OUT;
    mux_conf.pull_down_en = 0;
    mux_conf.pull_up_en = 0;
    gpio_config(&mux_conf);

    int cnt = 0;
    printf("initialized\n");

    while (1) {
        if (cnt == 8) cnt = 0;
        printf("%d\n", cnt);
        gpio_set_level(MUX1, 1 & cnt);
        gpio_set_level(MUX2, 1 & cnt>>1);
        gpio_set_level(MUX3, 1 & cnt>>2);
        vTaskDelay(1000/ portTICK_RATE_MS);
        cnt++;
        printf("change\n");
    }
    

    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    /* Set the GPIO as a push/pull output */
}
