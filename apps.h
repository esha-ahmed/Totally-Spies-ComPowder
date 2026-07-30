#ifndef APPS_H
#define APPS_H

#include <Arduino.h>
#include <lvgl.h>

// Ogni app espone: una funzione "create" che costruisce la lv_obj_t schermo
// (chiamata una sola volta in setup), e una funzione "update" chiamata ad
// ogni giro di loop() per animazioni/logica time-based. Le update fanno
// nulla se la loro schermata non è quella attiva, per non sprecare cicli.

lv_obj_t *radarApp_create();
void radarApp_update(unsigned long now);

lv_obj_t *musicApp_create();
void musicApp_update(unsigned long now);

lv_obj_t *netscanApp_create();
void netscanApp_update(unsigned long now);

// Orologio: la lettura I2C dell'RTC resta nel .ino (già gestisce Wire per
// il touch), qui esponiamo solo il "setter" che aggiorna le lancette.
lv_obj_t *clockApp_create();
void clockApp_setTime(int hour, int minute, int second, int day, int month, int year);

#endif
