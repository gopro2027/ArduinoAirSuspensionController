#ifndef solenoid_h
#define solenoid_h

#include "input_type.h"

class Solenoid {
private:
  InputType *pin;
  bool bopen;
public:
  Solenoid();
  Solenoid(InputType *pin);
  void open();
  void close();
  bool isOpen();
};

#endif
