import 'package:flutter/material.dart';
import 'header.dart';
import 'package:provider/provider.dart';
import '../provider/unit_provider.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
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

 @override
Widget build(BuildContext context) {
  return Scaffold(
    backgroundColor: const Color(0xFF121212),
    body: Column(
      children: [
      
        Expanded(
          child: SingleChildScrollView(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _buildInputTypeSection(),
                _buildBasicSettingsSection(),
                _buildDropDownWhenOffSection(),
                _buildLiftUpWhenOnSection(),
                _buildSolenoidsSection(),
                _buildReadoutTypeSection(),
                _buildTankPressureSection(),
                _buildDutyCycleSection(),
                _buildUnitsSection(context),
                _buildSystemSection(),
              ],
            ),
          ),
        ),
      ],
    ),
  );
}


  
  Widget _buildInputTypeSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Input type',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC) , width: 2),
              ),
            ),
            child: Column(
              children: [
                RadioListTile<String>(
                  title: Text(
                    'Pressure',
                    style: TextStyle(color: Colors.white),
                  ),
                  value: 'Pressure',
                  groupValue: inputType,
                  onChanged: (value) {
                    setState(() {
                      inputType = value!;
                    });
                  },
                  activeColor: Color(0xFFBB86FC) ,
                ),
                RadioListTile<String>(
                  title: Text(
                    'Buttons',
                    style: TextStyle(color: Colors.white),
                  ),
                  value: 'Buttons',
                  groupValue: inputType,
                  onChanged: (value) {
                    setState(() {
                      inputType = value!;
                    });
                  },
                  activeColor: Color(0xFFBB86FC) ,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildBasicSettingsSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Basic settings',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC) , width: 2),
              ),
            ),
            child: Column(
              children: [
                _buildSwitch(
                  'Maintain pressure',
                  maintainPressure,
                  (value) {
                    setState(() {
                      maintainPressure = value;
                    });
                  },
                ),
                _buildSwitch(
                  'Rise on start',
                  riseOnStart,
                  (value) {
                    setState(() {
                      riseOnStart = value;
                    });
                  },
                ),
                _buildSwitch(
                  'Drop down when off',
                  dropDownWhenOff,
                  (value) {
                    setState(() {
                      dropDownWhenOff = value;
                    });
                  },
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildDropDownWhenOffSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Drop down when off',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
              Switch(
                value: dropDownWhenOff,
                onChanged: (value) {
                  setState(() {
                    dropDownWhenOff = value;
                  });
                },
                activeColor: Color(0xFFBB86FC) ,
              ),
            ],
          ),
          if (dropDownWhenOff)
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16.0, vertical: 16.0),
              decoration: BoxDecoration(
                border: Border(
                  left: BorderSide(color: Color(0xFFBB86FC) , width: 2),
                ),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text(
                    'Delay',
                    style: TextStyle(color: Colors.white, fontSize: 16),
                  ),
                  Row(
                    children: [
                      IconButton(
                        icon: Icon(Icons.remove, color: Color(0xFFBB86FC) ),
                        onPressed: () {
                          setState(() {
                            if (dropDownDelay > 0) dropDownDelay--;
                          });
                        },
                      ),
                      Text(
                        '${dropDownDelay.toStringAsFixed(0)}s',
                        style: TextStyle(color: Colors.white, fontSize: 16),
                      ),
                      IconButton(
                        icon: Icon(Icons.add, color: Color(0xFFBB86FC) ),
                        onPressed: () {
                          setState(() {
                            dropDownDelay++;
                          });
                        },
                      ),
                    ],
                  ),
                ],
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildLiftUpWhenOnSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                'Lift up when on',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
              Switch(
                value: liftUpWhenOn,
                onChanged: (value) {
                  setState(() {
                    liftUpWhenOn = value;
                  });
                },
                activeColor: Color(0xFFBB86FC) ,
              ),
            ],
          ),
          if (liftUpWhenOn)
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16.0, vertical: 16.0),
              decoration: BoxDecoration(
                border: Border(
                  left: BorderSide(color: Color(0xFFBB86FC) , width: 2),
                ),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text(
                    'Delay',
                    style: TextStyle(color: Colors.white, fontSize: 16),
                  ),
                  Row(
                    children: [
                      IconButton(
                        icon: Icon(Icons.remove, color: Color(0xFFBB86FC) ),
                        onPressed: () {
                          setState(() {
                            if (liftUpDelay > 0) liftUpDelay--;
                          });
                        },
                      ),
                      Text(
                        '${liftUpDelay.toStringAsFixed(0)}s',
                        style: TextStyle(color: Colors.white, fontSize: 16),
                      ),
                      IconButton(
                        icon: Icon(Icons.add, color: Color(0xFFBB86FC) ),
                        onPressed: () {
                          setState(() {
                            liftUpDelay++;
                          });
                        },
                      ),
                    ],
                  ),
                ],
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildSolenoidsSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Solenoids',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC) , width: 2),
              ),
            ),
            child: Column(
              children: [
                _buildAdjustableRow(
                  'Solenoid open time on click (not calibrated)',
                  solenoidOpenTime,
                  'ms',
                  (value) {
                    setState(() {
                      solenoidOpenTime = value;
                    });
                  },
                ),
                _buildAdjustableRow(
                  'Solenoid psi on click (calibrated)',
                  solenoidPsi,
                  'psi',
                  (value) {
                    setState(() {
                      solenoidPsi = value;
                    });
                  },
                ),
                const SizedBox(height: 16),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    ElevatedButton(
                      onPressed: () {},
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Color(0xFFBB86FC) ,
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
                        style: TextStyle(color: Color(0xFFBB86FC) ),
                      ),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildReadoutTypeSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Readout type',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC) , width: 2),
              ),
            ),
            child: Column(
              children: [
                RadioListTile<String>(
                  title: Text(
                    'Pressure',
                    style: TextStyle(color: Colors.white),
                  ),
                  value: 'Pressure',
                  groupValue: readoutType,
                  onChanged: (value) {
                    setState(() {
                      readoutType = value!;
                    });
                  },
                  activeColor: Color(0xFFBB86FC) ,
                ),
                RadioListTile<String>(
                  title: Text(
                    'Height sensors',
                    style: TextStyle(color: Colors.white),
                  ),
                  value: 'Height sensors',
                  groupValue: readoutType,
                  onChanged: (value) {
                    setState(() {
                      readoutType = value!;
                    });
                  },
                  activeColor: Color(0xFFBB86FC) ,
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTankPressureSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Tank pressure',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC) , width: 2),
              ),
            ),
            child: ElevatedButton(
              onPressed: () {},
              style: ElevatedButton.styleFrom(
                backgroundColor: Color(0xFFBB86FC) ,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(8.0),
                ),
                padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
              ),
              child: const Text(
                'Set tank pressure',
                style: TextStyle(color: Colors.black),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildAdjustableRow(String label, double value, String unit, ValueChanged<double> onChanged) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Expanded(
            child: Text(
              label,
              style: TextStyle(color: Colors.white, fontSize: 16),
            ),
          ),
          Row(
            children: [
              IconButton(
                icon: Icon(Icons.remove, color: Color(0xFFBB86FC) ),
                onPressed: () {
                  if (value > 0) {
                    onChanged(value - 1);
                  }
                },
              ),
              Text(
                '${value.toStringAsFixed(0)} $unit',
                style: TextStyle(color: Colors.white, fontSize: 16),
              ),
              IconButton(
                icon: Icon(Icons.add, color: Color(0xFFBB86FC) ),
                onPressed: () {
                  onChanged(value + 1);
                },
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildSwitch(String label, bool value, ValueChanged<bool> onChanged) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: TextStyle(color: Colors.white, fontSize: 16),
          ),
          Switch(
            value: value,
            onChanged: onChanged,
            activeColor: Color(0xFFBB86FC) 
          ),
        ],
      ),
    );
  }
}
  Widget _buildDutyCycleSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Duty cycle',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC) , width: 2),
              ),
            ),
            child: ElevatedButton(
              onPressed: () {},
              style: ElevatedButton.styleFrom(
                backgroundColor: Color(0xFFBB86FC) ,
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(8.0),
                ),
                padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
              ),
              child: const Text(
                'Set Duty cycle',
                style: TextStyle(color: Colors.black),
              ),
            ),
          ),
        ],
      ),
    );
  }

    
  Widget _buildUnitsSection(BuildContext context) {
    return Consumer<UnitProvider>(
      builder: (context, unitProvider, child) {
        return Padding(
          padding: const EdgeInsets.symmetric(vertical: 24.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text(
                'Units',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
              RadioListTile<String>(
                title: const Text('Psi', style: TextStyle(color: Colors.white)),
                value: 'Psi',
                groupValue: unitProvider.unit,
                onChanged: (value) {
                  unitProvider.setUnit(value!);
                },
              ),
              RadioListTile<String>(
                title: const Text('Bar', style: TextStyle(color: Colors.white)),
                value: 'Bar',
                groupValue: unitProvider.unit,
                onChanged: (value) {
                  unitProvider.setUnit(value!);
                },
              ),
            ],
          ),
        );
      },
    );
  }

    Widget _buildSystemSection() {
    String system = '2 point';

    return StatefulBuilder(
      builder: (BuildContext context, StateSetter setState) {
        return Padding(
          padding: const EdgeInsets.symmetric(vertical: 24.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                'System',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 16.0),
                decoration: BoxDecoration(
                  border: Border(
                    left: BorderSide(color: Color(0xFFBB86FC) , width: 2),
                  ),
                ),
                child: Column(
                  children: [
                    RadioListTile<String>(
                      title: Text(
                        '4 point',
                        style: TextStyle(color: Colors.white),
                      ),
                      value: '4 point',
                      groupValue: system,
                      onChanged: (value) {
                        setState(() {
                          system = value!;
                        });
                      },
                      activeColor: Color(0xFFBB86FC) ,
                    ),
                    RadioListTile<String>(
                      title: Text(
                        '2 point',
                        style: TextStyle(color: Colors.white),
                      ),
                      value: '2 point',
                      groupValue: system,
                      onChanged: (value) {
                        setState(() {
                          system = value!;
                        });
                      },
                      activeColor: Color(0xFFBB86FC) ,
                    ),
                  ],
                ),
              ),
            ],
          ),
        );
      },
    );
  }


