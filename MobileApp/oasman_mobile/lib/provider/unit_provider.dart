import 'package:flutter/material.dart';

class UnitProvider extends ChangeNotifier {
  String _unit = 'Psi'; // Default unit

  String get unit => _unit;

  // Convert pressure to Bar if needed
  double convertToBar(double psi) {
    return psi * 0.0689476;
  }

  // Update the unit and notify listeners
  void setUnit(String newUnit) {
    if (_unit != newUnit) {
      _unit = newUnit;
      notifyListeners();
    }
  }
}
