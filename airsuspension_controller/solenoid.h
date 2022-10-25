#ifndef solenoid_h
#define solenoid_h

class Solenoid {
private:
  int pin;
  bool bopen;
public:
  Solenoid();
  Solenoid(int pin);
  void open();
  void close();
  bool isOpen();
};

#endif
