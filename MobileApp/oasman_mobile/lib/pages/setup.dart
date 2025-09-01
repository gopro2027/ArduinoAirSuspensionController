import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:flutter/services.dart';
import '../models/appSettings.dart';
import '../provider/unit_provider.dart';
import '../ble_manager.dart';
import 'dart:io';
import 'package:image_picker/image_picker.dart';
import 'package:path_provider/path_provider.dart';
import 'package:shared_preferences/shared_preferences.dart';

class ConnectManifoldCard extends StatelessWidget {
  const ConnectManifoldCard({super.key});

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.symmetric(vertical: 16),
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: Colors.grey[850],
        borderRadius: BorderRadius.circular(16),
        border: Border.all(color: Colors.white24, width: 1),
      ),
      child: Row(
        children: [
          const Icon(
            Icons.bluetooth_disabled_rounded,
            color: Colors.white54,
            size: 32,
          ),
          const SizedBox(width: 12),
          Expanded(
            child: Text(
              'Please connect a Bluetooth manifold first',
              style: const TextStyle(
                color: Colors.white70,
                fontSize: 16,
                fontWeight: FontWeight.w500,
              ),
            ),
          ),
        ],
      ),
    );
  }
}



class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => SettingsPageState();
}

class SettingsPageState extends State<SettingsPage> {
  late BLEManager bleManager;
  String inputType = 'Buttons';
  bool maintainPressure = true;
  bool riseOnStart = true;
  bool dropDownWhenOff = true; //airOutOnShutoff on the controller
  double dropDownDelay = 12.0;
  double liftUpDelay = 12.0;
  double solenoidOpenTime = 100.0;
  double solenoidPsi = 5.0;
  String readoutType = 'Height sensors';
  String units = globalSettings!.units;
  String passkeyText = globalSettings!.passkeyText;
  bool safetyMode = true;
  String bleBroadcastName = 'OasMan';
  int compressorOnPSI = 120;
  int compressorOffPSI = 140;

  File? _imageFile;
  final ImagePicker _picker = ImagePicker();

  @override
  void initState() {
    _loadSettings();
    super.initState();
  }

  void onLeavePage() {
    _saveSettings();
  }

  // Load saved settings
  Future<void> _loadSettings() async {
    final prefs = await SharedPreferences.getInstance();
    //load car image
    final savedPath = prefs.getString('uploaded_image');
    if (savedPath != null && File(savedPath).existsSync()) {
      setState(() {
        _imageFile = File(savedPath);
      });
    }

    //load app settings saved on the phone
    setState(() {
      units = prefs.getString('_units') ?? 'Psi';
      passkeyText = prefs.getString('_passkeyText') ?? '202777';
    });
    print("App's settings loaded");

    //load app settings saved on the manifold
    if (bleManager.connectedDevice != null) {
    riseOnStart = bleManager.riseOnStart;
    maintainPressure = bleManager.maintainPressure;
    dropDownWhenOff = bleManager.airOutOnShutoff;
    safetyMode = bleManager.safetyMode;
    bleBroadcastName = bleManager.bleBroadcastName;
    compressorOnPSI = bleManager.compressorOnPSI;
    compressorOffPSI = bleManager.compressorOffPSI;
    print("Manifold's settings loaded");
    };

  }

  // Save settings
  Future<void> _saveSettings() async {
    //Save settings to phone's
    final prefs = await SharedPreferences.getInstance();
    units = globalSettings!.units;
    await prefs.setString('_units', units);
    await prefs.setString('_passkeyText', passkeyText);

    //Save settings to manifold
    if (bleManager.connectedDevice != null) {
    bleManager.passkey = int.parse(passkeyText);
    bleManager.bleBroadcastName = bleBroadcastName;
    bleManager.compressorOnPSI = compressorOnPSI;
    bleManager.compressorOffPSI = compressorOffPSI;
    }

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Settings saved!')),
      );
    }
  }

  Future<void> _pickImage() async {
    final pickedFile = await _picker.pickImage(source: ImageSource.gallery);
    if (pickedFile != null) {
      final appDir = await getApplicationDocumentsDirectory();
      final fileName = pickedFile.name;
      final savedImage =
          await File(pickedFile.path).copy('${appDir.path}/$fileName');

      final prefs = await SharedPreferences.getInstance();
      await prefs.setString('uploaded_image', savedImage.path);

      setState(() {
        _imageFile = savedImage;
      });
    }
  }

  Widget _buildUploadImageSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Controller car image',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          GestureDetector(
            onTap: _pickImage,
            child: Container(
              height: 200,
              width: 100,
              decoration: BoxDecoration(
                border: Border.all(color: Color(0xFFBB86FC), width: 2),
                borderRadius: BorderRadius.circular(16),
                color: Colors.grey[900],
                image: _imageFile != null
                    ? DecorationImage(
                        image: FileImage(_imageFile!),
                        fit: BoxFit.cover,
                      )
                    : null,
              ),
              child: _imageFile == null
                  ? Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Icon(Icons.upload,
                              size: 30, color: Color(0xFFBB86FC)),
                          const SizedBox(height: 8),
                          Text('Tap to upload image',
                              style: TextStyle(color: Colors.white70),
                              textAlign: TextAlign.center),
                        ],
                      ),
                    )
                  : null,
            ),
          ),
          if (_imageFile != null)
            TextButton(
              onPressed: () async {
                final prefs = await SharedPreferences.getInstance();
                await prefs.remove('uploaded_image');
                setState(() {
                  _imageFile = null;
                });
              },
              child: Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Icon(Icons.delete, size: 20, color: Color(0xFFBB86FC)),
                  const SizedBox(height: 8),
                  const Text(
                    'Remove Image',
                    style: TextStyle(color: Color(0xFFBB86FC)),
                  ),
                ],
              ),
            ),
        ],
      ),
    );
  }

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    bleManager = Provider.of<BLEManager>(context);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF121212),
      appBar: AppBar(
        backgroundColor: const Color(0xFF121212),
        elevation: 0,
        title: const Text(
          'SETTINGS',
          style: TextStyle(
            fontSize: 25,
            fontWeight: FontWeight.w600,
            fontFamily: 'Roboto', // You can change this to a custom font
            color: Colors.white,
          ),
        ),
        centerTitle: true,
      ),
      body: Column(
        children: [
          Expanded(
            child: SingleChildScrollView(
              padding: const EdgeInsets.all(16.0),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  _buildUploadImageSection(),
                  _buildBluetoothSection(),
                  if (bleManager.connectedDevice != null) ...[
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
                  ]else ...[
const ConnectManifoldCard(),
                  ],
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildBluetoothSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'BLE Settings',
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
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
              ),
            ),
            child: Column(
              children: [
                _buildKeyboardInputRow(
                  "Passkey",
                  passkeyText,
                  (value) {
                    try {
                      bleManager.passkey = int.parse(value);
                      setState(() {
                        passkeyText = value;
                      });
                    } catch (e) {
                      print(e);
                      bleManager.passkey = 202777;
                      setState(() {
                        passkeyText = "202777";
                      });
                    }
                  },isNumberInput: true,
                ),
                if (bleManager.connectedDevice != null) ...[
                _buildKeyboardInputRow(
                  "Broadcast name",
                  bleBroadcastName,
                  (value) {
                    setState(() {
                      bleBroadcastName = value;
                    });
                  },
                ),
              ],
              ],
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
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
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
                  activeColor: Color(0xFFBB86FC),
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
                  activeColor: Color(0xFFBB86FC),
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
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
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
                activeColor: Color(0xFFBB86FC),
              ),
            ],
          ),
          if (dropDownWhenOff)
            Container(
              padding:
                  const EdgeInsets.symmetric(horizontal: 16.0, vertical: 16.0),
              decoration: BoxDecoration(
                border: Border(
                  left: BorderSide(color: Color(0xFFBB86FC), width: 2),
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
                        icon: Icon(Icons.remove, color: Color(0xFFBB86FC)),
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
                        icon: Icon(Icons.add, color: Color(0xFFBB86FC)),
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
                value: riseOnStart,
                onChanged: (value) {
                  setState(() {
                    riseOnStart = value;
                  });
                },
                activeColor: Color(0xFFBB86FC),
              ),
            ],
          ),
          if (riseOnStart)
            Container(
              padding:
                  const EdgeInsets.symmetric(horizontal: 16.0, vertical: 16.0),
              decoration: BoxDecoration(
                border: Border(
                  left: BorderSide(color: Color(0xFFBB86FC), width: 2),
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
                        icon: Icon(Icons.remove, color: Color(0xFFBB86FC)),
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
                        icon: Icon(Icons.add, color: Color(0xFFBB86FC)),
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
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
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
                        backgroundColor: Color(0xFFBB86FC),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(8.0),
                        ),
                        padding: const EdgeInsets.symmetric(
                            horizontal: 24, vertical: 12),
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
                        style: TextStyle(color: Color(0xFFBB86FC)),
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
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
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
                  activeColor: Color(0xFFBB86FC),
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
                  activeColor: Color(0xFFBB86FC),
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
          Row(
            children: [
              const Text(
                'Compressor',
                style: TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
              IconButton(
                icon: const Icon(Icons.help_outline,
                    size: 20, color: Colors.grey),
                onPressed: () {
                  showInfoDialog(
                    context,
                    'Compressor pressures',
                    'The compressor will start if pressure is below the "min pressure" and stop when it is above the "max pressure".\nCAUTION! Always check both tank\'s and compressor\'s pressure ratings!',
                  );
                },
              ),
            ],
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
              ),
            ),
            child: Column(
              children: [
                _buildKeyboardInputRow(
                    "Min pressure", compressorOnPSI.toString(), (value) {
                  setState(() {
                    compressorOnPSI = int.parse(value);
                  });
                }, isNumberInput: true),
                _buildKeyboardInputRow(
                    "Max pressure", compressorOffPSI.toString(), (value) {
                  setState(() {
                    compressorOffPSI = int.parse(value);
                  });
                }, isNumberInput: true),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

Widget _buildAdjustableRow(
    String label, double value, String unit, ValueChanged<double> onChanged) {
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
              icon: Icon(Icons.remove, color: Color(0xFFBB86FC)),
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
              icon: Icon(Icons.add, color: Color(0xFFBB86FC)),
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

Widget _buildKeyboardInputRow(
    String label, String value, ValueChanged<String> onChanged,
    {bool isNumberInput = false}) {
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
        Expanded(
          child: TextFormField(
            initialValue: value,
            onChanged: onChanged,
            keyboardType:
                isNumberInput ? TextInputType.number : TextInputType.text,
            inputFormatters:
                isNumberInput ? [FilteringTextInputFormatter.digitsOnly] : [],
            style: TextStyle(color: Colors.white), // text color
            decoration: InputDecoration(
              border: OutlineInputBorder(),
              filled: true,
              fillColor: Colors.grey[900], // dark background for the field
              hintText: 'Enter your manifold passkey',
              hintStyle: TextStyle(color: Colors.white54), // hint color
            ),
          ),
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
            value: value, onChanged: onChanged, activeColor: Color(0xFFBB86FC)),
      ],
    ),
  );
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
              left: BorderSide(color: Color(0xFFBB86FC), width: 2),
            ),
          ),
          child: ElevatedButton(
            onPressed: () {},
            style: ElevatedButton.styleFrom(
              backgroundColor: Color(0xFFBB86FC),
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

void showInfoDialog(BuildContext context, String title, String message) {
  showDialog(
    context: context,
    builder: (context) {
      return AlertDialog(
        title: Text(title),
        content: Text(message),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('OK'),
          ),
        ],
      );
    },
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
            Row(
              children: [
                const Text(
                  'Units',
                  style: TextStyle(
                    fontSize: 18,
                    fontWeight: FontWeight.bold,
                    color: Colors.white,
                  ),
                ),
                IconButton(
                  icon: const Icon(Icons.help_outline,
                      size: 20, color: Colors.grey),
                  onPressed: () {
                    showInfoDialog(
                      context,
                      'Pressure unit',
                      'Choose which pressure unit you prefer. Default is "PSI".',
                    );
                  },
                ),
              ],
            ),
            const SizedBox(height: 16),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16.0),
              decoration: BoxDecoration(
                border: Border(
                  left: BorderSide(color: Color(0xFFBB86FC), width: 2),
                ),
              ),
              child: Column(
                children: [
                  RadioListTile<String>(
                    title: const Text('Psi',
                        style: TextStyle(color: Colors.white)),
                    value: 'Psi',
                    activeColor: Color(0xFFBB86FC),
                    groupValue: globalSettings!.units,
                    onChanged: (value) async {
                      unitProvider.setUnit(value!);
                      globalSettings!.units = value;
                    },
                  ),
                  RadioListTile<String>(
                    title: const Text('Bar',
                        style: TextStyle(color: Colors.white)),
                    value: 'Bar',
                    activeColor: Color(0xFFBB86FC),
                    groupValue: globalSettings!.units,
                    onChanged: (value) async {
                      unitProvider.setUnit(value!);
                      globalSettings!.units = value;
                    },
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
                  left: BorderSide(color: Color(0xFFBB86FC), width: 2),
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
                    activeColor: Color(0xFFBB86FC),
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
                    activeColor: Color(0xFFBB86FC),
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
