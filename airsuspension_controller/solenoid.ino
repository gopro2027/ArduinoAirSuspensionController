class Solenoid {
private:
  int pin;
  boolean bopen;
public:
  Solenoid(int pin);
  void open();
  void close();
  boolean isOpen();
};

Solenoid::Solenoid(int pin) {
  pinMode(pin, OUTPUT);
  this->pin = pin;
  this->bopen = false;
}
void Solenoid::open() {
  digitalWrite(this->pin, HIGH);
  this->bopen = true;
}
void Solenoid::close() {
  digitalWrite(this->pin, LOW);
  this->bopen = false;
}
boolean Solenoid::isOpen() {
  return this->bopen;
}
