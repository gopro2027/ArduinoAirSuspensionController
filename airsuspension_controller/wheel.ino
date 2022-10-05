class Wheel {
private:
  int pin;
  Solenoid s_AirIn;
  Solenoid s_AirOut;
public:
  Wheel(int pin);
  void initPressureGoal(int newPressure);
  void pressureGoalRoutine();
};
