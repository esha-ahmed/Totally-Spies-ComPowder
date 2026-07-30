#ifndef APPS_H
#define APPS_H

#include <Arduino.h>
#include <lvgl.h>

// Each app exposes: a "create" function that builds the lv_obj_t screen
// (only a single voltage in setup), and an "update" function can be
// performed using the loop() function for time based animation/logic. Updates do 
// nothing if their screen is not the active one, so as not to waste cycles.

lv_obj_t *radarApp_create();
void radarApp_update(unsigned long now);

lv_obj_t *musicApp_create();
void musicApp_update(unsigned long now);

lv_obj_t *netscanApp_create();
void netscanApp_update(unsigned long now);

// Clock: the I2C reading of the RTC remains in the .ino (it already manages
// Wire for the touch), here we only expose the "setter" that updates the hands.

lv_obj_t *clockApp_create();
void clockApp_setTime(int hour, int minute, int second, int day, int month, int year);

#endif
