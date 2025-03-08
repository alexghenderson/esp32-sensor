#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "TemperatureSensor.h"
#include "MultiSensor.h"
#include "crypto.h"
#include "led.h"
#include "secrets.h"

#define LED_PIN 8 // Led Pin
#define SDA_PIN 8 // I2C Data Pin
#define SCK_PIN 9 // I2C Clock Pin
#define SCD41_ADDRESS 0x62  // I2C address of the SCD41 sensor

// TemperatureSensor* temp_sensor = nullptr;
MultiSensor* multi_sensor = nullptr;
LED onboard_led(GPIO_NUM_8, true);

void connect_to_wifi(const char* ssid, const char* password) {
// Set WiFi mode to station (client) mode
  WiFi.mode(WIFI_STA);
	WiFi.setTxPower(WIFI_POWER_8_5dBm);
  
  // Disconnect from any previous connections
  WiFi.disconnect();
  delay(100);
  
  // Print available networks (scanning)
  Serial.println("Scanning for networks...");
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; i++) {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" | BSSID: ");
    Serial.print(WiFi.BSSIDstr(i)); // MAC address as string
    Serial.print(" (RSSI: ");
    Serial.print(WiFi.RSSI(i));
    Serial.print(" dBm | Channel: ");
    Serial.println(WiFi.channel(i));
  }
  // Begin connection
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  // Wait for connection with timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    onboard_led.flash(500);
    delay(500);
  }

  // Check result
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\nFailed to connect. Status codes:");
    Serial.print("WiFi.status(): ");
    Serial.println(WiFi.status());
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        Serial.println("SSID not found");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("Connection failed (wrong password?)");
        break;
      case WL_IDLE_STATUS:
        Serial.println("Idle status");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }
}

void setup() {
    Serial.begin(115200);
    delay(1000);  // Wait for Serial to stabilize
    Serial.println("Starting...");
    
    connect_to_wifi(ssid, password);
    if(WiFi.status() == WL_CONNECTED) {
      onboard_led.turn_on();
    }
    
    // temp_sensor = new TemperatureSensor(TEMP_DATA_PIN);
    Serial.println("Initializing multi sensor");
    multi_sensor = new MultiSensor(SCD41_ADDRESS);


    Wire.begin(SDA_PIN, SCK_PIN);  // Initialize I2C with your pins
    delay(2000);  // Give sensor time to power up

    // temp_sensor->init();
    multi_sensor->init();

    multi_sensor->perform_self_check();
}

void report_data(const char* name, const char* field, const char* value, const char* type) {
    WiFiClientSecure *client = new WiFiClientSecure;
    HTTPClient https;

    Serial.println("Ingesting data");

    if (WiFi.status() == WL_CONNECTED) {
      if(client) {
          client->setInsecure();
          // client->setCACert(rootCert);

          Serial.print("Attempting to connect to: ");
          Serial.println(serverName);

          // Try connecting explicitly first to debug
          if (!client->connect("sensor-ingest.fly.dev", 443)) {
              Serial.println("Connection failed at client level");
              Serial.print("Last error: ");
              Serial.println(client->lastError(nullptr, 0));
              delete client;
              return;
          }

          if(https.begin(*client, serverName)) {
              https.addHeader("Content-Type", "application/json");

              StaticJsonDocument<200> json;
              json["sensor_name"] = name;
              json["field"] = field;
              json["value"] = value;
              json["type"] = type;

              String body;
              serializeJson(json, body);

              String signature = sign_message(private_key, body.c_str());
              https.addHeader("X-Signature", signature.c_str());
              Serial.print("Signature: ");
              Serial.println(signature);

              Serial.println("Sending JSON:" + body);

              int httpResponseCode = https.POST(body);

              if(httpResponseCode > 0) {
                String response = https.getString();
                Serial.println("HTTP Response code: " + String(httpResponseCode));
                Serial.println("Response: " + response);
              } else {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
                Serial.print("Error: ");
                Serial.println(https.errorToString(httpResponseCode));
              }

              https.end();
          } else {
            Serial.println("Unable to connect");
          }

          delete client;
      } else {
        Serial.println("Unable to create client");
      }
    } else {
      Serial.println("WiFi is not connected");
    }
}


void loop() {
    if(WiFi.status() == WL_CONNECTED) {
      onboard_led.turn_on();
      delay(1000);
    } else {
      onboard_led.turn_off();
    }

    MeasurementResult sensor_result = multi_sensor->read_measurement();
    report_data("bedroom", "temperature", String(sensor_result.temperature).c_str(), "number");
    report_data("bedroom", "relative_humidity", String(sensor_result.relative_humidity).c_str(), "number");
    report_data("bedroom", "co2", String(sensor_result.co2).c_str(), "number");
    // float temp = temp_sensor->get_temperature();
    // printf("Temp Control %f\n", temp);
    printf("result->temperature: %f\n", sensor_result.temperature);
    printf("result->relative_humidity: %d\n", sensor_result.relative_humidity);
    printf("result->co2: %f\n", sensor_result.co2);
    delay(60 * 1000 * 15);
}
