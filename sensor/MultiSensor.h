#include <Wire.h>

#define CMD_GET_STATUS 0xE488
#define CMD_START_PERIODIC_MEASURING 0x21b1
#define CMD_READ_MEASUREMENT 0xec05
#define CMD_STOP_PERIODIC_MEASURING 0x3f86
#define CMD_MEASURE_SINGLE_SHOT 0x219d
#define CMD_PERFORM_SELF_TEST 0x3639

struct MeasurementResult {
  short relative_humidity;
  float temperature;
  float co2;
};

class MultiSensor {
private:
  uint address;
  uint pin;
protected:
  void send_command(uint16_t cmd) {
    Wire.beginTransmission(this->address);
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

  uint8_t compute_crc(uint8_t data[], int len) {
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


  void read_bytes(void* buffer, uint16_t len) {
      uint8_t* byteBuffer = (uint8_t*)buffer;  // Cast void* to byte array pointer
      for(int i = 0; i < len; i++) {
          byteBuffer[i] = Wire.read();         // Assign directly
      }
  }
public:
  MultiSensor(uint address) : address(address) {
    Serial.printf("Instantiated multi sensor. Address: %d, pin: %d\n");
  }

  void init() {
    this->send_command(CMD_STOP_PERIODIC_MEASURING);
    delay(500);
  }

  int perform_self_check() {
    this->send_command(CMD_PERFORM_SELF_TEST);
    delay(10000);
    Wire.requestFrom(this->address, 3);
    if(Wire.available() == 3) {
      uint8_t buffer[3];
      this->read_bytes(buffer, 3);
      return 0;
    } else {
      Serial.printf("Unexpected response from self-check %d\n", Wire.available());
      return -1;
    }
  }

  MeasurementResult read_measurement() {
    this->send_command(CMD_MEASURE_SINGLE_SHOT);
    delay(5000);
    this->send_command(CMD_READ_MEASUREMENT);
    delay(1);
    Wire.requestFrom(this->address, 9);
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

        if (this->compute_crc(co2_bytes, 2) == co2_crc &&
            this->compute_crc(temp_bytes, 2) == temp_crc &&
            this->compute_crc(rh_bytes, 2) == rh_crc) {
            MeasurementResult result;
            uint16_t co2 = (co2_bytes[0] << 8) | co2_bytes[1];
            uint16_t temp_raw = (temp_bytes[0] << 8) | temp_bytes[1];
            uint16_t rh_raw = (rh_bytes[0] << 8) | rh_bytes[1];
            float temp = -45 + 175 * (float)temp_raw / 65535.0;
            float rh = 100 * (float)rh_raw / 65535.0;
            result.co2 = co2;
            result.temperature = temp;
            result.relative_humidity = rh;
            
            return result;
        } else {
            Serial.println("CRC error in measurement data");
        }
    } else {
        Serial.println("Failed to read measurement data");
    }
    0;
  }
};