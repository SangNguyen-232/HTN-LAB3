#include "global.h"
float glob_temperature = 0;
float glob_humidity = 0;
float glob_anomaly_score = 0;
String glob_anomaly_message = "Nomal";

bool web_led1_control_enabled = false;
bool web_led2_control_enabled = false;

String ssid = "ESP32-TESTLAB3!!!";
String password = "12345678";
String wifi_ssid = "ACLAB";
String wifi_password = "ACLAB2023";
String location_label = "Thành phố Hồ Chí Minh";
//String location_label = "Thành phố Hà Nội";
boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();