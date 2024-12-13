import 'package:flutter/material.dart';
import 'header.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      home: const SettingsPage(),
    );
  }
}

class SettingsPage extends StatelessWidget {
  const SettingsPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF121212),
      body: SingleChildScrollView(
        child: Column(
          children: [
            const Header(),
            Container(
              padding: const EdgeInsets.all(16.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  _buildSettingsSectionWithLine('Input type', [
                    _buildFunctionWithLine(
                      _buildRadioGroup(
                        options: ['Pressure', 'Buttons'],
                        groupValue: 'Pressure',
                        onChanged: (value) {},
                      ),
                    ),
                  ]),
                  _buildSettingsSectionWithLine('Basic settings', [
                    _buildFunctionWithLine(_buildSwitch('Maintain pressure', true, (value) {})),
                    _buildFunctionWithLine(_buildSwitch('Rise on start', true, (value) {})),
                    _buildFunctionWithLine(_buildSwitch('Drop down when off', true, (value) {})),
                    _buildFunctionWithLine(_buildSlider('Drop down delay', 12, 0, 20, (value) {})),
                    _buildFunctionWithLine(_buildSwitch('Lift up when on', true, (value) {})),
                    _buildFunctionWithLine(_buildSlider('Lift up delay', 12, 0, 20, (value) {})),
                  ]),
                  _buildSettingsSectionWithLine('Solenoids', [
                    _buildFunctionWithLine(_buildSlider('Solenoid open time (ms)', 100, 0, 500, (value) {})),
                    _buildFunctionWithLine(_buildSlider('Solenoid psi (calibrated)', 5, 0, 10, (value) {})),
                    _buildFunctionWithLine(Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        ElevatedButton(
                          onPressed: () {},
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.purple,
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(8.0),
                            ),
                            padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
                          ),
                          child: const Text(
                            'Calibrate',
                            style: TextStyle(color: Colors.black),
                          ),
                        ),
                        TextButton(
                          onPressed: () {},
                          style: TextButton.styleFrom(
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(8.0),
                            ),
                          ),
                          child: const Text(
                            'Remove calibration',
                            style: TextStyle(color: Colors.purple),
                          ),
                        ),
                      ],
                    )),
                  ]),
                  _buildSettingsSectionWithLine('Readout type', [
                    _buildFunctionWithLine(
                      _buildRadioGroup(
                        options: ['Pressure', 'Height sensors'],
                        groupValue: 'Pressure',
                        onChanged: (value) {},
                      ),
                    ),
                  ]),
                  _buildSettingsSectionWithLine('Units', [
                    _buildFunctionWithLine(
                      _buildRadioGroup(
                        options: ['Bar', 'Psi'],
                        groupValue: 'Psi',
                        onChanged: (value) {},
                      ),
                    ),
                  ]),
                  _buildSettingsSectionWithLine('System', [
                    _buildFunctionWithLine(
                      _buildRadioGroup(
                        options: ['4 point', '2 point'],
                        groupValue: '4 point',
                        onChanged: (value) {},
                      ),
                    ),
                  ]),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSettingsSectionWithLine(String title, List<Widget> children) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 16.0),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            width: 4,
            color: Colors.purple,
            height: _calculateSectionHeight(children),
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _buildSectionTitle(title),
                ...children,
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildFunctionWithLine(Widget child) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4.0),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            width: 4,
            color: Colors.purple,
          ),
          const SizedBox(width: 8),
          Expanded(child: child),
        ],
      ),
    );
  }

  double _calculateSectionHeight(List<Widget> children) {
    return children.length * 60.0; // Adjust based on the approximate height of each child widget.
  }

  Widget _buildSectionTitle(String title) {
    return Padding(
      padding: const EdgeInsets.only(top: 16.0, bottom: 8.0),
      child: Text(
        title,
        style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.white),
      ),
    );
  }

  Widget _buildSwitch(String label, bool value, ValueChanged<bool> onChanged) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Text(label, style: const TextStyle(color: Colors.white)),
        Switch(
          value: value,
          onChanged: onChanged,
          activeColor: Colors.purple,
        ),
      ],
    );
  }

  Widget _buildSlider(String label, double value, double min, double max, ValueChanged<double> onChanged) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('$label: ${value.toStringAsFixed(1)}', style: const TextStyle(color: Colors.white)),
        Slider(
          value: value,
          min: min,
          max: max,
          divisions: 20,
          label: value.toStringAsFixed(1),
          onChanged: onChanged,
          activeColor: Colors.purple,
        ),
      ],
    );
  }

}

Widget _buildRadioGroup({
  required List<String> options,
  required String groupValue,
  required ValueChanged<String?> onChanged,
}) {
  return Padding(
    padding: const EdgeInsets.symmetric(vertical: 4.0),
    child: Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Container(
          width: 4,
          color: Colors.purple,
          height: options.length * 60.0, // Adjust based on the approximate height of each child widget.
        ),
        const SizedBox(width: 8),
        Expanded(
          child: Column(
            children: options
                .map(
                  (option) => RadioListTile<String>(
                    title: Text(option, style: const TextStyle(color: Colors.white)),
                    value: option,
                    groupValue: groupValue,
                    onChanged: onChanged,
                    activeColor: Colors.purple,
                  ),
                )
                .toList(),
          ),
        ),
      ],
    ),
  );
}