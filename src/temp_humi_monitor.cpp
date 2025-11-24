#include "temp_humi_monitor.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);


void temp_humi_monitor(void *pvParameters){
    
    Wire.begin(11, 12);
    dht20.begin();

    while (1){
        /* code */
        //Serial.println("Hello World");
        dht20.read();
        // Reading temperature in Celsius
        float temperature = dht20.getTemperature();
        // Reading humidity
        float humidity = dht20.getHumidity();

        // Check if any reads failed and exit early
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
            //return;
        }

        //Update global variables for temperature and humidity
        glob_temperature = temperature;
        glob_humidity = humidity;

        // Print the results
        String result = "Temperature: " + String(temperature) + " °C, Humidity: " + String(humidity) + " %";
        //Serial.println(result);
        
        vTaskDelay(5000);
    }
    
}