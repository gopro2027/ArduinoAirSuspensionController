import 'package:flutter/material.dart';

class SettingsProvider with ChangeNotifier {
  String inputType = 'Buttons';
  bool maintainPressure = true;
  bool riseOnStart = true;
  bool dropDownWhenOff = true;
  bool liftUpWhenOn = true;
  double dropDownDelay = 12.0;
  double liftUpDelay = 12.0;
  double solenoidOpenTime = 100.0;
  double solenoidPsi = 5.0;
  String readoutType = 'Height sensors';
  String units = 'Psi';
  String system = '2 point';

  // Save Settings
  void saveSettings({
    required String inputType,
    required bool maintainPressure,
    required bool riseOnStart,
    required bool dropDownWhenOff,
    required bool liftUpWhenOn,
    required double dropDownDelay,
    required double liftUpDelay,
    required double solenoidOpenTime,
    required double solenoidPsi,
    required String readoutType,
    required String units,
    required String system,
  }) {
    this.inputType = inputType;
    this.maintainPressure = maintainPressure;
    this.riseOnStart = riseOnStart;
    this.dropDownWhenOff = dropDownWhenOff;
    this.liftUpWhenOn = liftUpWhenOn;
    this.dropDownDelay = dropDownDelay;
    this.liftUpDelay = liftUpDelay;
    this.solenoidOpenTime = solenoidOpenTime;
    this.solenoidPsi = solenoidPsi;
    this.readoutType = readoutType;
    this.units = units;
    this.system = system;

    notifyListeners(); // Notify listeners about the update
  }
}
