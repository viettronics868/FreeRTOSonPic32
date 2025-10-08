/*
 Definition of an array of function pointers
 */


#include "Lab17_gpio.h"

GetSW_t getSw[SW_COUNT] = {vGetSW0, vGetSW1, vGetSW2, vGetSW3};

LedToggle_t ledToggle[SW_COUNT] = {vLED0Toggle, vLED1Toggle, vLED2Toggle, vLEDRGBToggle};