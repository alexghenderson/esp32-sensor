#include <Wire.h>

#define SDA_PIN 8
#define SCK_PIN 9
#define SCD41_ADDRESS 0x62  // I2C address of the SCD41 sensor

#define CMD_GET_STATUS 0xE488
#define CMD_START_PERIODIC_MEASURING 0x21b1
#define CMD_READ_MEASUREMENT 0xec05
#define CMD_STOP_PERIODIC_MEASURING 0x3f86
#define CMD_MEASURE_SINGLE_SHOT 0x219d
#define CMD_PERFORM_SELF_TEST 0x3639

void setup() {
    Serial.begin(115200);
    delay(1000);  // Wait for Serial to stabilize
    Serial.println("Starting...");

    Wire.begin(SDA_PIN, SCK_PIN);  // Initialize I2C with your pins
    delay(2000);  // Give sensor time to power up

    // Stop any ongoing measurements
    sendCommand(0x3F86);
    delay(500);  // Wait as per datasheet
    performSelfCheck();
}

void performSelfCheck() {
  Serial.println("Performing self check");
  sendCommand(CMD_PERFORM_SELF_TEST);
  delay(10000);
  Wire.requestFrom(SCD41_ADDRESS, 3);
  if(Wire.available() == 3) {
    uint8_t buffer[3];
    readBytes(buffer, 3);
    Serial.printf("Received response %d %d\n", buffer[0], buffer[1]);
  } else {
    Serial.printf("Unexpected response from self-check");
  }
}

void readBytes(void* buffer, uint16_t len) {
    uint8_t* byteBuffer = (uint8_t*)buffer;  // Cast void* to byte array pointer
    for(int i = 0; i < len; i++) {
        byteBuffer[i] = Wire.read();         // Assign directly
    }
}

void loop() {
    sendCommand(CMD_MEASURE_SINGLE_SHOT);
    delay(5000);
    readMeasurement();
}

void sendCommand(uint16_t cmd) {
    Wire.beginTransmission(SCD41_ADDRESS);
    Wire.write(highByte(cmd));
    Wire.write(lowByte(cmd));
    byte error = Wire.endTransmission();
    if (error != 0) {
        Serial.print("Error sending command 0x");
        Serial.print(cmd, HEX);
        Serial.print(", error code: ");
        Serial.println(error);
    } else {
        // Serial.print("Sent command 0x");
        // Serial.println(cmd, HEX);
    }
}

void readMeasurement() {
    sendCommand(0xEC05);
    delay(1);
    Wire.requestFrom(SCD41_ADDRESS, 9);
    if (Wire.available() == 9) {
        uint8_t co2_bytes[2], temp_bytes[2], rh_bytes[2];
        uint8_t co2_crc, temp_crc, rh_crc;

        co2_bytes[0] = Wire.read();
        co2_bytes[1] = Wire.read();
        co2_crc = Wire.read();
        temp_bytes[0] = Wire.read();
        temp_bytes[1] = Wire.read();
        temp_crc = Wire.read();
        rh_bytes[0] = Wire.read();
        rh_bytes[1] = Wire.read();
        rh_crc = Wire.read();

        if (computeCRC(co2_bytes, 2) == co2_crc &&
            computeCRC(temp_bytes, 2) == temp_crc &&
            computeCRC(rh_bytes, 2) == rh_crc) {
            uint16_t co2 = (co2_bytes[0] << 8) | co2_bytes[1];
            uint16_t temp_raw = (temp_bytes[0] << 8) | temp_bytes[1];
            uint16_t rh_raw = (rh_bytes[0] << 8) | rh_bytes[1];
            float temp = -45 + 175 * (float)temp_raw / 65535.0;
            float rh = 100 * (float)rh_raw / 65535.0;

            Serial.print("CO2: ");
            Serial.print(co2);
            Serial.print(" ppm, Temp: ");
            Serial.print(temp, 1);
            Serial.print(" C, RH: ");
            Serial.print(rh, 1);
            Serial.println(" %");
        } else {
            Serial.println("CRC error in measurement data");
        }
    } else {
        Serial.println("Failed to read measurement data");
    }
}

uint8_t computeCRC(uint8_t data[], int len) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

// Optional function to read serial number for debugging
void readSerialNumber() {
    sendCommand(0x3682);
    delay(10);
    Wire.requestFrom(SCD41_ADDRESS, 9);
    if (Wire.available() == 9) {
        uint8_t buf[9];
        for (int i = 0; i < 9; i++) {
            buf[i] = Wire.read();
        }
        if (computeCRC(buf, 2) == buf[2] &&
            computeCRC(buf + 3, 2) == buf[5] &&
            computeCRC(buf + 6, 2) == buf[8]) {
            uint16_t word0 = (buf[0] << 8) | buf[1];
            uint16_t word1 = (buf[3] << 8) | buf[4];
            uint16_t word2 = (buf[6] << 8) | buf[7];
            Serial.print("Serial Number: ");
            Serial.print(word0, HEX);
            Serial.print(word1, HEX);
            Serial.println(word2, HEX);
        } else {
            Serial.println("CRC error in serial number");
        }
    } else {
        Serial.println("Failed to read serial number");
    }
}