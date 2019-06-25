#ifndef LED_HEADER
#define LED_HEADER

#include <stdio.h>
#include "driver/gpio.h"
void led_init();
void led_state(int state);

#endif
