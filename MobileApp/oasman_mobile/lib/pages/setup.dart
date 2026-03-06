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

bool _settingsLoaded = false;

class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => SettingsPageState();
}

class SettingsPageState extends State<SettingsPage> {
  late UnitProvider unitprovider;
  late BLEManager bleManager;
  bool maintainPressure = false;
  bool riseOnStart = false;
  bool dropDownWhenOff = false; // airOutOnShutoff on the controller
  String passkeyText = "";
  bool safetyMode = true;
  String bleBroadcastName = '';
  int compressorOnPSI = 120;
  int compressorOffPSI = 140;
  int systemShutoffTimeM = 0;
  int pressureSensorMax = 0;
  int bagVolumePercentage = 0;
  int bagMaxPressure = 0;

  File? _imageFile;
  final ImagePicker _picker = ImagePicker();

  late TextEditingController passkeyController;
  late TextEditingController broadcastController;
  late TextEditingController shutdownTimeController;
  late TextEditingController minPressureController;
  late TextEditingController maxPressureController;

  void initState() {
    super.initState();
    bleManager = BLEManager();
    _initialize();
  }

  @override
  void dispose() {
    passkeyController.dispose();
    broadcastController.dispose();
    super.dispose();
  }

  Future<void> _initialize() async {
    await _loadSettings(); // now load settings
  }

  void onLeavePage() {
    _settingsLoaded = false;
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
      globalSettings!.units = prefs.getString('_units') ?? 'Psi';
      passkeyText = prefs.getString('_passkeyText') ?? '202777';
    });
    print("App's settings loaded");

    //load app settings saved on the manifold
    if (bleManager.connectedDevice != null) {
      setState(() {
        riseOnStart = bleManager.riseOnStart;
        maintainPressure = bleManager.maintainPressure;
        dropDownWhenOff = bleManager.airOutOnShutoff;
        safetyMode = bleManager.safetyMode;
        bleBroadcastName = bleManager.bleBroadcastName;
        compressorOnPSI = bleManager.compressorOnPSI;
        compressorOffPSI = bleManager.compressorOffPSI;
        systemShutoffTimeM = bleManager.systemShutoffTimeM;
      });
      print("Manifold's settings loaded");
    }
    passkeyController =
        TextEditingController(text: globalSettings!.passkeyText);
    broadcastController = TextEditingController(text: bleBroadcastName);
    shutdownTimeController =
        TextEditingController(text: systemShutoffTimeM.toString());
    minPressureController = TextEditingController();
    maxPressureController = TextEditingController();
    _settingsLoaded = true;
  }

  // Save settings
  Future<void> _saveSettings() async {
    //Save settings to phone's
    final prefs = await SharedPreferences.getInstance();
    final units = globalSettings!.units;
    passkeyText = globalSettings!.passkeyText;
    await prefs.setString('_units', units);
    await prefs.setString('_passkeyText', passkeyText);

    //Save settings to manifold
    if (bleManager.isConnected()) {
      if (compressorOnPSI >= compressorOffPSI) {
        //the compression max pressure cannot be smaller then the min pressure
        compressorOffPSI = compressorOnPSI + 1;
      }
      bleManager.passkey = int.parse(passkeyText);
      bleManager.bleBroadcastName = bleBroadcastName;
      bleManager.compressorOnPSI = compressorOnPSI;
      bleManager.compressorOffPSI = compressorOffPSI;
      bleManager.systemShutoffTimeM = systemShutoffTimeM;
      bleManager.safetyMode = safetyMode;
      bleManager.riseOnStart = riseOnStart;
      bleManager.maintainPressure = maintainPressure;
      bleManager.airOutOnShutoff = dropDownWhenOff;
      print("save started");
      print(bleBroadcastName);
      bleManager.saveConfigToManifold();
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
    if (!_settingsLoaded) {
      return const Center(child: CircularProgressIndicator());
    }
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
            fontFamily: 'Roboto',
            color: Colors.white,
          ),
        ),
        centerTitle: true,
      ),
      body: Consumer<BLEManager>(
        builder: (context, bleManager, _) {
          return Column(
            children: [
              Expanded(
                child: SingleChildScrollView(
                  padding: const EdgeInsets.all(16.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      _buildUploadImageSection(),
                      _buildBluetoothSection(),
                      if (bleManager.isConnected()) ...[
                        _buildStatusSection(),
                        _buildBasicSettingsSection(),
                        _buildDropDownWhenOffSection(),
                        _buildLiftUpWhenOnSection(),
                        _buildUnitsSection(context),
                        _buildTankPressureSection(context),
                        _buildAIStatusSection(),
                        ElevatedButton(
                          onPressed: () {
                            try {
                              bleManager.sendRestCommand([BTOasIdentifier.REBOOT]);
                              debugPrint("Reboot the manifold");
                            } catch (e) {
                              debugPrint("Failed to send command: $e");
                            }
                          },
                          style: ElevatedButton.styleFrom(
                            padding: const EdgeInsets.symmetric(
                                horizontal: 20, vertical: 12),
                            shape: RoundedRectangleBorder(
                              borderRadius: BorderRadius.circular(12),
                            ),
                          ),
                          child: const Text(
                            "Reboot the manifold!",
                            style: TextStyle(fontSize: 16),
                          ),
                        ),
                      ] else ...[
                        const ConnectManifoldCard(),
                      ],
                    ],
                  ),
                ),
              ),
            ],
          );
        },
      ),
    );
  }

  Widget _buildStatusSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Status',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: const BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text(
                  'Compressor on',
                  style: TextStyle(color: Colors.white, fontSize: 16),
                ),
                Switch(
                  value: bleManager.compressorOn,
                  onChanged: (value) {
                    bleManager.sendCompressorStatus(value);
                    setState(() {});
                  },
                  activeColor: const Color(0xFFBB86FC),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildAIStatusSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'ML/AI',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: const BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text(
                  'AI status',
                  style: TextStyle(color: Colors.white, fontSize: 16),
                ),
                Switch(
                  value: bleManager.aiStatusEnabled,
                  onChanged: (value) {
                    setState(() {
                      bleManager.aiStatusEnabled = value;
                    });
                  },
                  activeColor: const Color(0xFFBB86FC),
                ),
              ],
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
                _buildKeyboardInputRow("Passkey", passkeyController, (value) {
                  try {
                    bleManager.passkey = int.parse(value);
                    globalSettings!.passkeyText = value;
                    setState(() {
                      passkeyText = value;
                    });
                  } catch (e) {
                    bleManager.passkey = 202777;
                    setState(() {
                      passkeyText = "202777";
                    });
                  }
                },
                    isNumberInput: true,
                    limitChar: 6,
                    tooltipTitle: "Passkey",
                    tooltip:
                        "The passkey is a kind of password which should match between the app and the manifold controller. It validates upon connection. If you change during the manifold connected, it will change on it too. The passkey only can be numbers and 6 digits."),
                if (bleManager.connectedDevice != null) ...[
                  _buildKeyboardInputRow("Broadcast name", broadcastController,
                      (value) {
                    setState(() {
                      bleBroadcastName = value;
                    });
                  },
                      limitChar: 10,
                      tooltipTitle: "Manifold's bluetooth name",
                      tooltip:
                          "Change the name of bluetooth broadcast name of the manifold. The change takes place after a reboot or next start. Max 10 characters."),
                ],
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
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Row(
                      crossAxisAlignment: CrossAxisAlignment.center,
                      children: [
                        const Text(
                          'Safety mode',
                          style: TextStyle(
                            color: Colors.white,
                            fontSize: 16,
                          ),
                        ),
                        IconButton(
                          icon: const Icon(Icons.help_outline,
                              size: 20, color: Colors.grey),
                          onPressed: () {
                            showInfoDialog(
                              context,
                              'Safety mode',
                              'When safety mode is enabled the compressor is disabled.',
                            );
                          },
                        ),
                      ],
                    ),
                    _buildSwitch(
                      '',
                      safetyMode,
                      (value) {
                        setState(() {
                          safetyMode = value;
                        });
                      },
                    ),
                  ],
                ),
                _buildSwitch(
                  'Maintain pressure',
                  maintainPressure,
                  (value) {
                    setState(() {
                      maintainPressure = value;
                    });
                  },
                ),
                Row(
                  crossAxisAlignment: CrossAxisAlignment.center,
                  children: [
                    const SizedBox(width: 5),
                    Expanded(
                      child: _buildKeyboardInputRow(
                        "System off delay",
                        shutdownTimeController,
                        (value) {
                          setState(() {
                            if (value.isEmpty) {
                              // Handle empty input safely
                              systemShutoffTimeM = 0; // or keep previous value
                            } else {
                              systemShutoffTimeM = int.parse(value);
                            }
                          });
                        },
                        isNumberInput: true,
                        limitChar: 3,
                        units: "min",
                        tooltip: "Define the number of minutes the air ride system remains powered after the ignition is switched off.",
                      ),
                    ),
                  ],
                )
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
          const Text(
            'Drop down when off',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: const BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text(
                  'Air out on shutoff',
                  style: TextStyle(color: Colors.white, fontSize: 16),
                ),
                Switch(
                  value: dropDownWhenOff,
                  onChanged: (value) {
                    setState(() {
                      dropDownWhenOff = value;
                    });
                  },
                  activeColor: const Color(0xFFBB86FC),
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
          const Text(
            'Lift up when on',
            style: TextStyle(
              fontSize: 18,
              fontWeight: FontWeight.bold,
              color: Colors.white,
            ),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: const BoxDecoration(
              border: Border(
                left: BorderSide(color: Color(0xFFBB86FC), width: 2),
              ),
            ),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text(
                  'Rise on start',
                  style: TextStyle(color: Colors.white, fontSize: 16),
                ),
                Switch(
                  value: riseOnStart,
                  onChanged: (value) {
                    setState(() {
                      riseOnStart = value;
                    });
                  },
                  activeColor: const Color(0xFFBB86FC),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  String? _lastUnits;
  _buildTankPressureSection(BuildContext context) {
    return Consumer<UnitProvider>(
      builder: (context, unitProvider, child) {
        final units = unitProvider.unit;

        // Only update text if units changed
        if (_lastUnits != units) {
          minPressureController.text = units == 'Bar'
              ? unitProvider
                  .convertToBar(compressorOnPSI.toDouble())
                  .toStringAsFixed(2)
              : compressorOnPSI.toString();

          maxPressureController.text = units == 'Bar'
              ? unitProvider
                  .convertToBar(compressorOffPSI.toDouble())
                  .toStringAsFixed(2)
              : compressorOffPSI.toString();

          _lastUnits = units;
        }

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
                        "Min pressure",
                        minPressureController,
                        (value) {
                          setState(() {
                            if (value.isEmpty) {
                              // Handle empty input safely
                              compressorOnPSI = 0; // or keep previous value
                            } else {
                              compressorOnPSI = units == 'Bar'
                                  ? unitProvider
                                      .convertToPsi(double.parse(value))
                                      .toInt()
                                  : int.parse(value);
                            }
                          });
                        },
                        isNumberInput: true,
                        units: units,
                      ),
                      _buildKeyboardInputRow(
                        "Max pressure",
                        maxPressureController,
                        (value) {
                          setState(() {
                            if (value.isEmpty) {
                              // Handle empty input safely
                              compressorOffPSI = 0; // or keep previous value
                            } else {
                              compressorOffPSI = units == 'Bar'
                                  ? unitProvider
                                      .convertToPsi(double.parse(value))
                                      .toInt()
                                  : int.parse(value);
                            }
                          });
                        },
                        isNumberInput: true,
                        units: units,
                      ),
                    ],
                  ),
                ),
              ],
            ));
      },
    );
  }

  Widget _buildKeyboardInputRow(
    String label,
    TextEditingController controller,
    ValueChanged<String> onChanged, {
    bool isNumberInput = false,
    int limitChar = 0,
    String units = '',
    String tooltip = '',
    String tooltipTitle = '',
  }) {
    tooltipTitle = label;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8.0),
      child: Row(
        children: [
          if (label != "")
            Expanded(
              child: Text(
                label,
                style: TextStyle(color: Colors.white, fontSize: 16),
              ),
            ),
          if (tooltip != '')
            IconButton(
              icon:
                  const Icon(Icons.help_outline, size: 20, color: Colors.grey),
              onPressed: () {
                showInfoDialog(
                  context,
                  tooltipTitle,
                  tooltip,
                );
              },
            ),
          Expanded(
            child: TextFormField(
              controller: controller,
              keyboardType:
                  isNumberInput ? TextInputType.number : TextInputType.text,
              inputFormatters: [
                if (isNumberInput)
                  units == "Bar"
                      ? FilteringTextInputFormatter.allow(
                          RegExp(r'^\d*\.?\d{0,1}'))
                      : FilteringTextInputFormatter.digitsOnly,
                if (limitChar != 0) LengthLimitingTextInputFormatter(limitChar),
              ],
              style: TextStyle(color: Colors.white),
              decoration: InputDecoration(
                border: OutlineInputBorder(),
                filled: true,
                fillColor: Colors.grey[900],
                hintStyle: TextStyle(color: Colors.white54),
                suffixText: units,
              ),
              onChanged: onChanged, 
            ),
          ),
        ],
      ),
    );
  }
}

Widget _buildSwitch(String label, bool value, ValueChanged<bool> onChanged) {
  return Padding(
    padding: const EdgeInsets.symmetric(vertical: 8.0),
    child: Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        if (label != "")
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
                    groupValue: unitProvider.unit, // ✅ bind to provider
                    onChanged: (value) {
                      unitProvider.setUnit(value!);
                      globalSettings!.units = value;
                    },
                    activeColor: Color(0xFFBB86FC),
                  ),
                  RadioListTile<String>(
                    title: const Text('Bar',
                        style: TextStyle(color: Colors.white)),
                    value: 'Bar',
                    groupValue: unitProvider.unit,
                    onChanged: (value) {
                      unitProvider.setUnit(value!);
                      globalSettings!.units = value;
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
