class LED {
  uint pin;
  bool low_on;
  int on;
  int off;
public:
  LED(uint pin, bool low_on = false) : pin(pin), low_on(low_on) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, low_on ? HIGH : LOW);
    if(low_on) {
      this->on = LOW;
      this->off = HIGH;
    } else {
      this->on = HIGH;
      this->off = LOW;
    }
  }

  void flash(long time) {
    this->turn_on();
    delay(time);
    this->turn_off();
  }

  void turn_on() {
    digitalWrite(this->pin, this->on);
  }

  void turn_off() {
    digitalWrite(this->pin, this->off);
  }
};