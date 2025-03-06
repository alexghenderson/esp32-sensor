
#include <OneWire.h>
#include <DallasTemperature.h>

class TemperatureSensor {
private:
  uint pin;
  OneWire* oneWire;
  DallasTemperature* sensors;
  byte addr[8];
  bool sensorFound;

public:
  TemperatureSensor(uint pin) : pin(pin), sensorFound(false) {
    Serial.println("Allocating OneWire...");
    this->oneWire = new OneWire(pin);
    if (!this->oneWire) {
      Serial.println("ERROR: OneWire allocation failed");
      return;
    }
    Serial.println("Allocating DallasTemperature...");
    this->sensors = new DallasTemperature(this->oneWire);
    if (!this->sensors) {
      Serial.println("ERROR: DallasTemperature allocation failed");
      return;
    }
    Serial.println("Initializing sensors...");
    this->sensors->begin();
    Serial.println("Constructor complete");
  }

  ~TemperatureSensor() {  // Cleanup for dynamic allocation
    delete this->sensors;
    delete this->oneWire;
  }

  void init() {
    Serial.printf("Initializing temperature sensor on pin %d\n", this->pin);
    if (this->oneWire->search(this->addr)) {
      Serial.print("Found sensor, Address: ");
      for (int i = 0; i < 8; i++) {
        Serial.print(this->addr[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      this->sensorFound = true;
    } else {
      Serial.println("Failed to find sensor");
      this->sensorFound = false;
    }
  }

  float get_temperature() {
    if (!this->sensorFound) {
      Serial.println("No sensor found, skipping read");
      return 0.0;
    }
    Serial.println("Requesting temperatures...");
    this->sensors->requestTemperatures();
    Serial.println("Getting temperature...");
    float temp = this->sensors->getTempC(this->addr);
    Serial.println("Temperature retrieved");
    if (temp != DEVICE_DISCONNECTED_C) {
      return temp;
    } else {
      Serial.println("ERROR: Unable to read temperature");
      return 0.0;
    }
  }
};