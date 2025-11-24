#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


extern float glob_temperature;
extern float glob_humidity;
extern float glob_anomaly_score;
extern String glob_anomaly_message;

extern bool web_led1_control_enabled; 
extern bool web_led2_control_enabled;

extern float glob_anomaly_score;
extern String glob_anomaly_message;

extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;
extern String location_label;
extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
#endif