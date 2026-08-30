import 'package:flutter/material.dart';
import '../models/appSettings.dart';

/// Bar per PSI. Exposed so callers that only need the conversion (and have no
/// [UnitProvider] instance to hand) don't have to repeat the literal.
const double barPerPsi = 0.0689476;

class UnitProvider extends ChangeNotifier {
  String _unit = globalSettings!.units;

  String get unit => _unit;

  // Convert pressure to Bar if needed
  double convertToBar(double psi) {
    return psi * barPerPsi;
  }

  double convertToPsi(double bar) {
    return bar / barPerPsi;
  }

  // Update the unit and notify listeners
  void setUnit(String newUnit) {
    if (_unit != newUnit) {
      _unit = newUnit;
      notifyListeners();
    }
  }
}
