#include <Wire.h>
#include "TemperatureSensor.h"
#include "MultiSensor.h"

#define SDA_PIN 8
#define SCK_PIN 9
#define SCD41_ADDRESS 0x62  // I2C address of the SCD41 sensor
#define TEMP_DATA_PIN 0


TemperatureSensor* temp_sensor = nullptr;
MultiSensor* multi_sensor = nullptr;

void setup() {
    Serial.begin(115200);
    delay(1000);  // Wait for Serial to stabilize
    Serial.println("Starting...");
    
    temp_sensor = new TemperatureSensor(TEMP_DATA_PIN);
    Serial.println("Initializing multi sensor");
    multi_sensor = new MultiSensor(SCD41_ADDRESS);


    Wire.begin(SDA_PIN, SCK_PIN);  // Initialize I2C with your pins
    delay(2000);  // Give sensor time to power up

    temp_sensor->init();
    multi_sensor->init();

    multi_sensor->perform_self_check();
}



void loop() {
    MeasurementResult sensor_result = multi_sensor->read_measurement();
    float temp = temp_sensor->get_temperature();
    printf("Temp Control %f\n", temp);
    printf("result->temperature: %f\n", sensor_result.temperature);
    printf("result->relative_humidity: %d\n", sensor_result.relative_humidity);
    printf("result->co2: %f\n", sensor_result.co2);
}
