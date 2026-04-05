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
  late BLEManager bleManager;
  bool _bleListenerAttached = false;
  int _lastSyncedConfigRevision = -1;

  /// Phone-only copy for SharedPreferences (mirrors `globalSettings!.passkeyText`).
  String passkeyText = '';

  File? _imageFile;
  final ImagePicker _picker = ImagePicker();

  late TextEditingController passkeyController;
  late TextEditingController broadcastController;
  late TextEditingController shutdownTimeController;
  late TextEditingController minPressureController;
  late TextEditingController maxPressureController;

  String? _lastUnits;

  @override
  void initState() {
    super.initState();
    _initialize();
  }

  void _onBleManagerChanged() {
    if (!mounted || !_settingsLoaded) return;
    final bm = bleManager;
    if (!bm.isConnected()) {
      _lastSyncedConfigRevision = -1;
      return;
    }
    if (bm.configRevision <= _lastSyncedConfigRevision) return;
    _lastSyncedConfigRevision = bm.configRevision;
    shutdownTimeController.text = bm.systemShutoffTimeM.toString();
    if (bm.bleBroadcastName.isNotEmpty) {
      broadcastController.text = bm.bleBroadcastName;
    }
    setState(() => _lastUnits = null);
  }

  @override
  void dispose() {
    if (_bleListenerAttached) {
      bleManager.removeListener(_onBleManagerChanged);
    }
    passkeyController.dispose();
    broadcastController.dispose();
    shutdownTimeController.dispose();
    minPressureController.dispose();
    maxPressureController.dispose();
    super.dispose();
  }

  Future<void> _initialize() async {
    await _loadSettings(); // now load settings
  }

  void onLeavePage() {
    _settingsLoaded = false;
    _saveSettings();
  }

  Future<void> _persistPhoneSettings() async {
    final prefs = await SharedPreferences.getInstance();
    passkeyText = globalSettings!.passkeyText;
    await prefs.setString('_units', globalSettings!.units);
    await prefs.setString('_passkeyText', passkeyText);
  }

  void _applyPasskeyFromController() {
    try {
      final text = passkeyController.text.trim();
      final pk = int.parse(text);
      globalSettings!.passkeyText = text;
      passkeyText = text;
      bleManager.passkey = pk;
    } catch (_) {
      globalSettings!.passkeyText = '202777';
      passkeyText = '202777';
      bleManager.passkey = 202777;
    }
  }

  void _applyCompressorFromControllers(
      BLEManager bm, UnitProvider unitProvider, String units) {
    void apply(TextEditingController c, void Function(int) assign) {
      final t = c.text.trim();
      if (t.isEmpty) {
        assign(0);
        return;
      }
      try {
        if (units == 'Bar') {
          assign(unitProvider.convertToPsi(double.parse(t)).toInt());
        } else {
          assign(int.parse(t));
        }
      } catch (_) {}
    }

    apply(minPressureController, (v) => bm.compressorOnPSI = v);
    apply(maxPressureController, (v) => bm.compressorOffPSI = v);
  }

  /// Writes phone prefs; if connected, pushes controller values to the manifold and saves config.
  Future<void> _saveManifoldConfigNow({bool showSnackBar = true}) async {
    if (!mounted) return;
    try {
      _applyPasskeyFromController();
      await _persistPhoneSettings();

      if (!bleManager.isConnected()) {
        if (mounted && showSnackBar) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('Saved'),
              duration: Duration(seconds: 1),
            ),
          );
        }
        return;
      }

      final bm = bleManager;
      final st = int.tryParse(shutdownTimeController.text.trim());
      if (st != null) bm.systemShutoffTimeM = st;
      bm.bleBroadcastName = broadcastController.text;

      final unitProvider = Provider.of<UnitProvider>(context, listen: false);
      _applyCompressorFromControllers(bm, unitProvider, unitProvider.unit);

      if (bm.compressorOnPSI >= bm.compressorOffPSI) {
        bm.compressorOffPSI = bm.compressorOnPSI + 1;
      }

      bm.saveConfigToManifold();
      bm.refreshFromUi();

      if (mounted && showSnackBar) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Saved'),
            duration: Duration(seconds: 1),
          ),
        );
      }
    } catch (e, st) {
      debugPrint('Save failed: $e $st');
    }
  }

  void _onTextFieldDone() {
    FocusScope.of(context).unfocus();
    _saveManifoldConfigNow();
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

    passkeyController =
        TextEditingController(text: globalSettings!.passkeyText);
    final bm = bleManager;
    final broadcastInitial =
        bm.connectedDevice != null && bm.bleBroadcastName.isNotEmpty
            ? bm.bleBroadcastName
            : '';
    broadcastController = TextEditingController(text: broadcastInitial);
    shutdownTimeController = TextEditingController(
        text: bm.connectedDevice != null
            ? bm.systemShutoffTimeM.toString()
            : '0');
    minPressureController = TextEditingController();
    maxPressureController = TextEditingController();

    if (bm.connectedDevice != null) {
      _lastSyncedConfigRevision = bm.configRevision;
      print("Manifold's settings loaded");
    }
    _settingsLoaded = true;
    setState(() {});
  }

  // Save when leaving the page (silent; also catches unsubmitted text fields).
  Future<void> _saveSettings() async {
    await _saveManifoldConfigNow(showSnackBar: false);
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
    final bm = Provider.of<BLEManager>(context, listen: false);
    if (!_bleListenerAttached) {
      bleManager = bm;
      bleManager.addListener(_onBleManagerChanged);
      _bleListenerAttached = true;
    } else {
      bleManager = bm;
    }
  }

  @override
  Widget build(BuildContext context) {
    if (!_settingsLoaded) {
      return const Center(child: CircularProgressIndicator());
    }
    return Consumer<BLEManager>(
      builder: (context, bleManager, _) {
        return Column(
          children: [
            Padding(
              padding: const EdgeInsets.symmetric(vertical: 12),
              child: Text(
                'Settings',
                style: const TextStyle(
                  fontSize: 25,
                  fontWeight: FontWeight.w600,
                  fontFamily: 'Roboto',
                  color: Colors.white,
                ),
              ),
            ),
            Expanded(
              child: SingleChildScrollView(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    _buildUploadImageSection(),
                    _buildConfigSection(context, bleManager),
                    if (bleManager.isConnected()) ...[
                      _buildStatusSection(bleManager),
                      _buildAIStatusSection(bleManager),
                      _buildBasicSettingsSection(bleManager),
                      _buildUnitsSection(),
                      ElevatedButton(
                        onPressed: () {
                          try {
                            bleManager.sendRestCommand(bleManager
                                .buildRestPacket(BTOasIdentifier.REBOOT, []));
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
                          'Reboot/Turn Off',
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
    );
  }

  Widget _buildStatusSection(BLEManager bm) {
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
                  'Compressor Status:',
                  style: TextStyle(color: Colors.white, fontSize: 16),
                ),
                Switch(
                  value: bm.compressorOn,
                  onChanged: (value) {
                    bm.sendCompressorStatus(value);
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

  Widget _buildAIStatusSection(BLEManager bm) {
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
                  'Enabled:',
                  style: TextStyle(color: Colors.white, fontSize: 16),
                ),
                Switch(
                  value: bm.aiStatusEnabled,
                  onChanged: (value) {
                    bm.aiStatusEnabled = value;
                    bm.refreshFromUi();
                    _saveManifoldConfigNow();
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

  Widget _buildConfigSection(BuildContext context, BLEManager bm) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Config',
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
                  'Bluetooth Passkey (6 digits)',
                  passkeyController,
                  onChanged: (value) {
                    try {
                      bm.passkey = int.parse(value);
                      globalSettings!.passkeyText = value;
                      setState(() {
                        passkeyText = value;
                      });
                    } catch (e) {
                      bm.passkey = 202777;
                      setState(() {
                        passkeyText = "202777";
                      });
                    }
                  },
                  isNumberInput: true,
                  limitChar: 6,
                  tooltipTitle: 'Bluetooth Passkey (6 digits)',
                  tooltip:
                      'Must match the manifold. Validates on connection. When the manifold is connected, updating this updates the manifold too. Six digits only.',
                  saveWhenKeyboardDone: true,
                ),
                if (bm.connectedDevice != null) ...[
                  _buildKeyboardInputRow(
                    'Bluetooth broadcast name',
                    broadcastController,
                    limitChar: 10,
                    tooltipTitle: 'Bluetooth broadcast name',
                    tooltip:
                        'Manifold BLE advertising name. Takes effect after reboot or next start. Max 10 characters.',
                    saveWhenKeyboardDone: true,
                  ),
                  _buildKeyboardInputRow(
                    'Shutoff Time (Minutes)',
                    shutdownTimeController,
                    isNumberInput: true,
                    limitChar: 3,
                    tooltip:
                        'Minutes the air ride system stays powered after ignition off.',
                    saveWhenKeyboardDone: true,
                  ),
                  Consumer<UnitProvider>(
                    builder: (context, unitProvider, _) {
                      final units = unitProvider.unit;
                      if (_lastUnits != units) {
                        minPressureController.text = units == 'Bar'
                            ? unitProvider
                                .convertToBar(bm.compressorOnPSI.toDouble())
                                .toStringAsFixed(2)
                            : bm.compressorOnPSI.toString();
                        maxPressureController.text = units == 'Bar'
                            ? unitProvider
                                .convertToBar(bm.compressorOffPSI.toDouble())
                                .toStringAsFixed(2)
                            : bm.compressorOffPSI.toString();
                        _lastUnits = units;
                      }
                      return Column(
                        children: [
                          _buildKeyboardInputRow(
                            'Compressor On PSI',
                            minPressureController,
                            isNumberInput: true,
                            units: units,
                            tooltipTitle: 'Compressor On PSI',
                            tooltip:
                                'Compressor runs when pressure is below this and stops above Compressor Off PSI. Respect tank and compressor ratings.',
                            saveWhenKeyboardDone: true,
                          ),
                          _buildKeyboardInputRow(
                            'Compressor Off PSI',
                            maxPressureController,
                            isNumberInput: true,
                            units: units,
                            tooltipTitle: 'Compressor Off PSI',
                            tooltip:
                                'Compressor stops when pressure is above this. Respect tank and compressor ratings.',
                            saveWhenKeyboardDone: true,
                          ),
                        ],
                      );
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

  Widget _buildBasicSettingsSection(BLEManager bm) {
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
                  'Maintain Preset',
                  bm.maintainPressure,
                  (value) {
                    bm.maintainPressure = value;
                    bm.refreshFromUi();
                    _saveManifoldConfigNow();
                  },
                ),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text(
                      'Rise on start',
                      style: TextStyle(color: Colors.white, fontSize: 16),
                    ),
                    Switch(
                      value: bm.riseOnStart,
                      onChanged: (value) {
                        bm.riseOnStart = value;
                        bm.refreshFromUi();
                        _saveManifoldConfigNow();
                      },
                      activeColor: const Color(0xFFBB86FC),
                    ),
                  ],
                ),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text(
                      'Fall on shutdown',
                      style: TextStyle(color: Colors.white, fontSize: 16),
                    ),
                    Switch(
                      value: bm.airOutOnShutoff,
                      onChanged: (value) {
                        bm.airOutOnShutoff = value;
                        bm.refreshFromUi();
                        _saveManifoldConfigNow();
                      },
                      activeColor: const Color(0xFFBB86FC),
                    ),
                  ],
                ),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Row(
                      crossAxisAlignment: CrossAxisAlignment.center,
                      children: [
                        const Text(
                          'Safety Mode',
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
                              'Safety Mode',
                              'When safety mode is enabled the compressor is disabled.',
                            );
                          },
                        ),
                      ],
                    ),
                    _buildSwitch(
                      '',
                      bm.safetyMode,
                      (value) {
                        bm.safetyMode = value;
                        bm.refreshFromUi();
                        _saveManifoldConfigNow();
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

  Widget _buildUnitsSection() {
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
                        'Units',
                        'Choose which pressure unit you prefer. Default is PSI.',
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
                      title: const Text('PSI',
                          style: TextStyle(color: Colors.white)),
                      value: 'Psi',
                      groupValue: unitProvider.unit,
                      onChanged: (value) {
                        unitProvider.setUnit(value!);
                        globalSettings!.units = value;
                        _persistPhoneSettings();
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
                        _persistPhoneSettings();
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

  Widget _buildKeyboardInputRow(
    String label,
    TextEditingController controller, {
    ValueChanged<String>? onChanged,
    bool isNumberInput = false,
    int limitChar = 0,
    String units = '',
    String tooltip = '',
    String tooltipTitle = '',
    bool saveWhenKeyboardDone = false,
  }) {
    final helpTitle = tooltipTitle.isNotEmpty ? tooltipTitle : label;
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
                  helpTitle,
                  tooltip,
                );
              },
            ),
          Expanded(
            child: TextFormField(
              controller: controller,
              keyboardType:
                  isNumberInput ? TextInputType.number : TextInputType.text,
              textInputAction: saveWhenKeyboardDone
                  ? TextInputAction.done
                  : TextInputAction.next,
              onFieldSubmitted:
                  saveWhenKeyboardDone ? (_) => _onTextFieldDone() : null,
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
