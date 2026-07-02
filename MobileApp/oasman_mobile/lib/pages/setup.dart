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
import 'package:url_launcher/url_launcher.dart';

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
  late TextEditingController bagMaxController;
  late TextEditingController pressureSensorRatingController;
  late TextEditingController bagVolumeController;
  late TextEditingController wifiSsidController;
  late TextEditingController wifiPassController;
  late TextEditingController auxPulseDurationController;
  late TextEditingController auxIntervalCyclesController;

  /// Local latch for "Aux output" switch (not in GETCONFIGVALUES; matches wireless).
  bool _auxOutputLatchUi = false;

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
    bagMaxController.text = bm.bagMaxPressure.toString();
    pressureSensorRatingController.text = bm.pressureSensorMax.toString();
    bagVolumeController.text = bm.bagVolumePercentage.toString();
    auxPulseDurationController.text = bm.auxPulseDuration.toString();
    auxIntervalCyclesController.text = bm.auxIntervalCycles.toString();
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
    bagMaxController.dispose();
    pressureSensorRatingController.dispose();
    bagVolumeController.dispose();
    wifiSsidController.dispose();
    wifiPassController.dispose();
    auxPulseDurationController.dispose();
    auxIntervalCyclesController.dispose();
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
    await prefs.setString('_wifiSsid', wifiSsidController.text);
    await prefs.setString('_wifiPass', wifiPassController.text);
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

      final bagMax = int.tryParse(bagMaxController.text.trim());
      if (bagMax != null) {
        bm.bagMaxPressure = bagMax.clamp(1, 256);
      }
      final psMax = int.tryParse(pressureSensorRatingController.text.trim());
      if (psMax != null) {
        bm.pressureSensorMax = psMax.clamp(0, 65535);
      }
      final bagVol = int.tryParse(bagVolumeController.text.trim());
      if (bagVol != null) {
        bm.bagVolumePercentage = bagVol.clamp(10, 600);
      }

      final auxPulse = int.tryParse(auxPulseDurationController.text.trim());
      if (auxPulse != null) {
        bm.auxPulseDuration = auxPulse.clamp(0, 255);
      }
      final auxIntv = int.tryParse(auxIntervalCyclesController.text.trim());
      if (auxIntv != null) {
        bm.auxIntervalCycles = auxIntv.clamp(0, 255);
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

  Future<void> _showConfirm({
    required String title,
    required String message,
    String confirmLabel = 'Confirm',
    required VoidCallback onConfirm,
  }) async {
    final go = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(title),
        content: Text(message),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: Text(confirmLabel),
          ),
        ],
      ),
    );
    if (go == true && mounted) {
      onConfirm();
    }
  }

  Future<void> _openWebsite() async {
    final uri = Uri.parse('https://oasman.dev');
    final ok = await launchUrl(uri, mode: LaunchMode.externalApplication);
    if (!ok && mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Could not open the website')),
      );
    }
  }

  Future<void> _openPrivacyPolicy() async {
    final uri = Uri.parse('https://oasman.dev/privacy');
    final ok = await launchUrl(uri, mode: LaunchMode.externalApplication);
    if (!ok && mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Could not open the privacy policy')),
      );
    }
  }

  Widget _buildWebsiteLinkSection() {
    return Padding(
      padding: const EdgeInsets.only(top: 16.0, bottom: 0),
      child: TextButton.icon(
        onPressed: _openWebsite,
        icon: const Icon(Icons.open_in_new, size: 20, color: Color(0xFFBB86FC)),
        label: const Text(
          'View website',
          style: TextStyle(color: Colors.white70, fontSize: 16),
        ),
      ),
    );
  }

  Widget _buildPrivacyPolicySection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 16.0),
      child: TextButton.icon(
        onPressed: _openPrivacyPolicy,
        icon: const Icon(Icons.open_in_new, size: 20, color: Color(0xFFBB86FC)),
        label: const Text(
          'Privacy policy',
          style: TextStyle(color: Colors.white70, fontSize: 16),
        ),
      ),
    );
  }

  String _aiTrainedSummary(int bits) {
    String y(bool on) => on ? 'Y' : 'n';
    return 'UF:  ${y((bits & 1) != 0)} UR:  ${y(((bits >> 1) & 1) != 0)}\n'
        'DF: ${y(((bits >> 2) & 1) != 0)} DR: ${y(((bits >> 3) & 1) != 0)}';
  }

  int _rfPresetZeroBased(BLEManager bm, int rfButtonNumber) {
    switch (rfButtonNumber) {
      case BLEManager.rfButtonA:
        return bm.rfButtonAPreset;
      case BLEManager.rfButtonB:
        return bm.rfButtonBPreset;
      case BLEManager.rfButtonC:
        return bm.rfButtonCPreset;
      case BLEManager.rfButtonD:
        return bm.rfButtonDPreset;
      default:
        return 0;
    }
  }

  void _setRfPresetZeroBased(BLEManager bm, int rfButtonNumber, int z) {
    switch (rfButtonNumber) {
      case BLEManager.rfButtonA:
        bm.rfButtonAPreset = z;
        break;
      case BLEManager.rfButtonB:
        bm.rfButtonBPreset = z;
        break;
      case BLEManager.rfButtonC:
        bm.rfButtonCPreset = z;
        break;
      case BLEManager.rfButtonD:
        bm.rfButtonDPreset = z;
        break;
    }
  }

  Widget _buildRfPresetRow(BLEManager bm, String label, int rfButtonNumber) {
    final z = _rfPresetZeroBased(bm, rfButtonNumber).clamp(0, 4);
    final displayed = z + 1;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8.0),
      child: Row(
        children: [
          Expanded(
            child: Text(
              label,
              style: const TextStyle(color: Colors.white, fontSize: 16),
            ),
          ),
          DropdownButton<int>(
            value: displayed,
            dropdownColor: Colors.grey[850],
            style: const TextStyle(color: Colors.white),
            items: [
              for (var i = 1; i <= 5; i++)
                DropdownMenuItem(value: i, child: Text('$i')),
            ],
            onChanged: (v) {
              if (v == null) return;
              bm.sendRfButtonPresetAssign(rfButtonNumber, v);
              _setRfPresetZeroBased(bm, rfButtonNumber, v - 1);
              bm.refreshFromUi();
              setState(() {});
            },
          ),
        ],
      ),
    );
  }

  Widget _readOnlyStatusRow(String label, String value) {
    const labelStyle = TextStyle(color: Colors.white, fontSize: 16);
    const valueStyle = TextStyle(color: Colors.white70, fontSize: 16);
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8.0),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Expanded(
            flex: 2,
            child: Text(label, style: labelStyle),
          ),
          const SizedBox(width: 12),
          Expanded(
            flex: 3,
            child: Text(
              value,
              style: valueStyle,
              textAlign: TextAlign.end,
            ),
          ),
        ],
      ),
    );
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
    bagMaxController = TextEditingController();
    pressureSensorRatingController = TextEditingController();
    bagVolumeController = TextEditingController();
    wifiSsidController =
        TextEditingController(text: prefs.getString('_wifiSsid') ?? '');
    wifiPassController =
        TextEditingController(text: prefs.getString('_wifiPass') ?? '');
    auxPulseDurationController = TextEditingController(
      text: bm.connectedDevice != null
          ? bm.auxPulseDuration.toString()
          : '1',
    );
    auxIntervalCyclesController = TextEditingController(
      text: bm.connectedDevice != null
          ? bm.auxIntervalCycles.toString()
          : '0',
    );

    if (bm.connectedDevice != null) {
      _lastSyncedConfigRevision = bm.configRevision;
      bagMaxController.text = bm.bagMaxPressure.toString();
      pressureSensorRatingController.text = bm.pressureSensorMax.toString();
      bagVolumeController.text = bm.bagVolumePercentage.toString();
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
    final keyboardInset = MediaQuery.viewInsetsOf(context).bottom;
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
            padding: EdgeInsets.fromLTRB(16, 16, 16, 16 + keyboardInset),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _buildUploadImageSection(),
                _buildConfigSection(context),
                Selector<BLEManager, bool>(
                  selector: (_, m) => m.isConnected(),
                  builder: (context, connected, _) {
                    if (!connected) {
                      return const ConnectManifoldCard();
                    }
                    return Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        _buildStatusSection(context),
                        _buildGameControllerSection(context),
                        _buildAIStatusSection(context),
                        _buildManifoldConfigSections(context),
                        _buildAuxillaryOutputSection(context),
                        _buildUnitsSection(),
                        _buildWifiUpdateSection(context),
                        _buildRebootTurnOffButton(context),
                      ],
                    );
                  },
                ),
                _buildWebsiteLinkSection(),
                _buildPrivacyPolicySection(),
              ],
            ),
          ),
        ),
      ],
    );
  }

  /// Config / RF / levelling fields only change with GETCONFIGVALUES or local edits,
  /// not with high-frequency STATUSREPORT packets.
  Widget _buildManifoldConfigSections(BuildContext context) {
    return Selector<
        BLEManager,
        (
          bool,
          bool,
          bool,
          bool,
          bool,
          bool,
          int,
          int,
          int,
          int,
        )>(
      selector: (_, m) => (
        m.maintainPressure,
        m.sensorlessLeveling,
        m.riseOnStart,
        m.airOutOnShutoff,
        m.safetyMode,
        m.heightSensorMode,
        m.rfButtonAPreset,
        m.rfButtonBPreset,
        m.rfButtonCPreset,
        m.rfButtonDPreset,
      ),
      builder: (context, _, __) {
        final bm = context.read<BLEManager>();
        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            _buildBasicSettingsSection(bm),
            _buildLevellingSection(bm),
          ],
        );
      },
    );
  }

  Widget _buildStatusSection(BuildContext context) {
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
          Selector<
              BLEManager,
              (
                bool,
                bool,
                bool,
                bool,
              )>(
            selector: (_, m) => (
              m.compressorFrozen,
              m.vehicleOn,
              m.ebrakeOn,
              m.compressorOn,
            ),
            builder: (context, status, _) {
              final bm = context.read<BLEManager>();
              final (frozen, accOn, ebrake, compOn) = status;
              return Container(
                padding: const EdgeInsets.symmetric(horizontal: 16.0),
                decoration: const BoxDecoration(
                  border: Border(
                    left: BorderSide(color: Color(0xFFBB86FC), width: 2),
                  ),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    _readOnlyStatusRow(
                      'Compressor Frozen:',
                      frozen ? 'Yes' : 'No',
                    ),
                    _readOnlyStatusRow(
                      'ACC Status:',
                      accOn ? 'On' : 'Off',
                    ),
                    _readOnlyStatusRow(
                      'E-Brake Status:',
                      ebrake ? 'On' : 'Off',
                    ),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        const Text(
                          'Compressor Status:',
                          style: TextStyle(color: Colors.white, fontSize: 16),
                        ),
                        Switch(
                          value: compOn,
                          onChanged: (value) {
                            bm.sendCompressorStatus(value);
                          },
                          activeColor: const Color(0xFFBB86FC),
                        ),
                      ],
                    ),
                  ],
                ),
              );
            },
          ),
        ],
      ),
    );
  }

  Widget _buildAIStatusSection(BuildContext context) {
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
          Selector<BLEManager, (int, int, bool)>(
            selector: (_, m) =>
                (m.aiLearnPercent, m.aiReadyBittset, m.aiStatusEnabled),
            builder: (context, ai, _) {
              final bm = context.read<BLEManager>();
              final (learnPct, trainedBits, aiEnabled) = ai;
              return Container(
                padding: const EdgeInsets.symmetric(horizontal: 16.0),
                decoration: const BoxDecoration(
                  border: Border(
                    left: BorderSide(color: Color(0xFFBB86FC), width: 2),
                  ),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    _readOnlyStatusRow(
                      'Learn Progress:',
                      '$learnPct%',
                    ),
                    Padding(
                      padding: const EdgeInsets.symmetric(vertical: 8.0),
                      child: Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          const Text(
                            'Trained:',
                            style: TextStyle(color: Colors.white, fontSize: 16),
                          ),
                          Flexible(
                            child: Text(
                              _aiTrainedSummary(trainedBits),
                              style: const TextStyle(
                                  color: Colors.white70, fontSize: 14),
                              textAlign: TextAlign.right,
                            ),
                          ),
                        ],
                      ),
                    ),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        const Text(
                          'Enabled:',
                          style: TextStyle(color: Colors.white, fontSize: 16),
                        ),
                        Switch(
                          value: aiEnabled,
                          onChanged: (value) {
                            bm.aiStatusEnabled = value;
                            bm.refreshFromUi();
                            _saveManifoldConfigNow();
                          },
                          activeColor: const Color(0xFFBB86FC),
                        ),
                      ],
                    ),
                    const SizedBox(height: 8),
                    TextButton(
                      onPressed: () => _showConfirm(
                        title: 'Reset Learned AI data?',
                        message:
                            'Run this if AI has completed training and you are getting inaccurate presets.',
                        onConfirm: () {
                          bm.sendResetAi();
                          if (mounted) {
                            ScaffoldMessenger.of(context).showSnackBar(
                              const SnackBar(
                                content: Text('Reset AI command sent'),
                                duration: Duration(seconds: 2),
                              ),
                            );
                          }
                        },
                      ),
                      child: const Text('Reset Learned Data'),
                    ),
                  ],
                ),
              );
            },
          ),
        ],
      ),
    );
  }

  Widget _buildConfigSection(BuildContext context) {
    final bm = bleManager;
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
                Selector<BLEManager, bool>(
                  selector: (_, m) => m.connectedDevice != null,
                  builder: (context, hasDevice, _) {
                    if (!hasDevice) return const SizedBox.shrink();
                    final m = context.read<BLEManager>();
                    return Column(
                      children: [
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
                                      .convertToBar(
                                          m.compressorOnPSI.toDouble())
                                      .toStringAsFixed(2)
                                  : m.compressorOnPSI.toString();
                              maxPressureController.text = units == 'Bar'
                                  ? unitProvider
                                      .convertToBar(
                                          m.compressorOffPSI.toDouble())
                                      .toStringAsFixed(2)
                                  : m.compressorOffPSI.toString();
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
                                _buildKeyboardInputRow(
                                  'Bag Max PSI',
                                  bagMaxController,
                                  isNumberInput: true,
                                  limitChar: 3,
                                  tooltipTitle: 'Bag Max PSI',
                                  tooltip:
                                      'Maximum bag pressure target (1–256 PSI). Sent to the manifold with Save.',
                                  saveWhenKeyboardDone: true,
                                ),
                                _buildKeyboardInputRow(
                                  'Pressure Sensor Rating PSI',
                                  pressureSensorRatingController,
                                  isNumberInput: true,
                                  limitChar: 5,
                                  tooltipTitle: 'Pressure Sensor Rating PSI',
                                  tooltip:
                                      'Rated range of your pressure sensors. Must match manifold configuration.',
                                  saveWhenKeyboardDone: true,
                                ),
                                _buildKeyboardInputRow(
                                  'Bag Volume Percentage',
                                  bagVolumeController,
                                  isNumberInput: true,
                                  limitChar: 3,
                                  tooltipTitle: 'Bag Volume Percentage',
                                  tooltip:
                                      'Bag volume factor (10–600). Matches the wireless controller slider range.',
                                  saveWhenKeyboardDone: true,
                                ),
                              ],
                            );
                          },
                        ),
                      ],
                    );
                  },
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildGameControllerSection(BuildContext context) {
    final bm = context.read<BLEManager>();
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Game Controller',
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
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                TextButton(
                  onPressed: () => _showConfirm(
                    title: 'Confirm?',
                    message:
                        'After confirming, OASMan will become pairable and the next controller to pair will be allowed and remembered (max 20 devices).',
                    onConfirm: () {
                      bm.sendBp32Command(BLEManager.bp32EnableNewConn);
                      if (mounted) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(
                            content: Text('Connect your controller'),
                            duration: Duration(seconds: 3),
                          ),
                        );
                      }
                    },
                  ),
                  child: const Text('Allow New Controller'),
                ),
                TextButton(
                  onPressed: () => _showConfirm(
                    title: 'Confirm?',
                    message:
                        'All paired game controllers will be removed from memory and actively connected ones disconnected.',
                    onConfirm: () {
                      bm.sendBp32Command(BLEManager.bp32ForgetDevices,
                          value: false);
                      if (mounted) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(
                            content: Text('Controllers forgotten'),
                            duration: Duration(seconds: 2),
                          ),
                        );
                      }
                    },
                  ),
                  child: const Text('Un-pair All Controllers'),
                ),
                TextButton(
                  onPressed: () => _showConfirm(
                    title: 'Confirm?',
                    message:
                        'Disconnects paired controllers. On some devices, use the system button to disconnect.',
                    onConfirm: () {
                      bm.sendBp32Command(BLEManager.bp32DisconnectDevices,
                          value: false);
                      if (mounted) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(
                            content: Text('Controllers disconnected'),
                            duration: Duration(seconds: 2),
                          ),
                        );
                      }
                    },
                  ),
                  child: const Text('Disconnect Controllers'),
                ),
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
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                _buildSwitch(
                  bm.heightSensorMode ? 'Maintain Height' : 'Auto Leak Detect Refill',
                  bm.maintainPressure,
                  (value) {
                    bm.maintainPressure = value;
                    bm.refreshFromUi();
                    _saveManifoldConfigNow();
                  },
                ),
                if (!bm.heightSensorMode)
                  _buildSwitch(
                    'Sensorless Level',
                    bm.sensorlessLeveling,
                    (value) {
                      bm.sensorlessLeveling = value;
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
                // TextButton(
                //   onPressed: () => _showConfirm(
                //     title: 'Detect Pressure Sensors?',
                //     message:
                //         'WARNING: YOUR CAR WILL BE AIRED OUT. This routine learns which pressure sensors map to which wheels.',
                //     onConfirm: () {
                //       bm.sendDetectPressureSensors();
                //       if (mounted) {
                //         ScaffoldMessenger.of(context).showSnackBar(
                //           const SnackBar(
                //             content: Text('Detection routine started'),
                //             duration: Duration(seconds: 3),
                //           ),
                //         );
                //       }
                //     },
                //   ),
                //   child: const Text('Detect Pressure Sensors'),
                // ),
                const Padding(
                  padding: EdgeInsets.only(top: 16, bottom: 8),
                  child: Text(
                    'Key Fob Settings',
                    style: TextStyle(
                      fontSize: 16,
                      fontWeight: FontWeight.w600,
                      color: Colors.white,
                    ),
                  ),
                ),
                TextButton(
                  onPressed: () => _showConfirm(
                    title: 'Unlearn key fob?',
                    message:
                        'Your key fob will be unlearned. Requires an OASMan Key Fob Receiver (RX480E).',
                    onConfirm: () {
                      bm.sendRfUnlearnFob();
                      if (mounted) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(
                            content: Text('Unlearning key fob…'),
                            duration: Duration(seconds: 2),
                          ),
                        );
                      }
                    },
                  ),
                  child: const Text('Unlearn Fob'),
                ),
                TextButton(
                  onPressed: () => _showConfirm(
                    title: 'Learn fob?',
                    message: 'Requires an OASMan Key Fob Receiver (RX480E).',
                    onConfirm: () {
                      bm.sendRfLearnFobMomentary();
                      if (mounted) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(
                            content: Text('Learning key fob mode…'),
                            duration: Duration(seconds: 2),
                          ),
                        );
                      }
                    },
                  ),
                  child: const Text('Learn Fob'),
                ),
                _buildRfPresetRow(
                    bm, 'Button A Preset Number', BLEManager.rfButtonA),
                _buildRfPresetRow(
                    bm, 'Button B Preset Number', BLEManager.rfButtonB),
                _buildRfPresetRow(
                    bm, 'Button C Preset Number', BLEManager.rfButtonC),
                _buildRfPresetRow(
                    bm, 'Button D Preset Number', BLEManager.rfButtonD),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildLevellingSection(BLEManager bm) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Levelling Mode',
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
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                RadioListTile<bool>(
                  title: const Text('Pressure Sensor',
                      style: TextStyle(color: Colors.white)),
                  value: false,
                  groupValue: bm.heightSensorMode,
                  onChanged: (v) {
                    if (v == null) return;
                    bm.heightSensorMode = v;
                    bm.refreshFromUi();
                    _saveManifoldConfigNow();
                  },
                  activeColor: const Color(0xFFBB86FC),
                ),
                RadioListTile<bool>(
                  title: const Text('Level Sensor',
                      style: TextStyle(color: Colors.white)),
                  value: true,
                  groupValue: bm.heightSensorMode,
                  onChanged: (v) {
                    if (v == null) return;
                    bm.heightSensorMode = v;
                    bm.refreshFromUi();
                    _saveManifoldConfigNow();
                  },
                  activeColor: const Color(0xFFBB86FC),
                ),
                if (bm.heightSensorMode) ...[
                  const SizedBox(height: 8),
                  TextButton(
                    onPressed: () => _showConfirm(
                      title: 'Calibrate Min Height?',
                      message:
                          'Please air out your car to the lowest it goes before you click ok',
                      confirmLabel: 'OK',
                      onConfirm: () {
                        bm.sendCalibrateHeightSensors(false);
                        if (mounted) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(
                              content: Text('Calibrated min height'),
                              duration: Duration(seconds: 2),
                            ),
                          );
                        }
                      },
                    ),
                    child: const Text('Calibrate Min Height'),
                  ),
                  TextButton(
                    onPressed: () => _showConfirm(
                      title: 'Calibrate Max Height?',
                      message:
                          'Raise your vehicle as high as it can go before you click ok',
                      confirmLabel: 'OK',
                      onConfirm: () {
                        bm.sendCalibrateHeightSensors(true);
                        if (mounted) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(
                              content: Text('Calibrated max height'),
                              duration: Duration(seconds: 2),
                            ),
                          );
                        }
                      },
                    ),
                    child: const Text('Calibrate Max Height'),
                  ),
                ],
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildAuxillaryOutputSection(BuildContext context) {
    const unitLabels = [
      'Deciseconds',
      'Seconds',
      'Minutes',
      'Hours',
    ];
    // Only aux config fields — not live pressure/status — so this subtree does not
    // rebuild on every STATUSREPORT when nothing aux-related changed.
    return Selector<BLEManager, (int, int)>(
      selector: (_, m) => (m.auxModeByte, m.auxTimeUnit),
      builder: (context, _, __) {
        final bm = context.read<BLEManager>();
        return Padding(
          padding: const EdgeInsets.symmetric(vertical: 24.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Text(
                'Auxillary Output',
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
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Padding(
                      padding: const EdgeInsets.symmetric(vertical: 8.0),
                      child: GestureDetector(
                        onTapDown: (_) {
                          bm.sendAuxillaryOutputControl(true);
                        },
                        onTapUp: (_) {
                          bm.sendAuxillaryOutputControl(false);
                          setState(() => _auxOutputLatchUi = false);
                        },
                        onTapCancel: () {
                          bm.sendAuxillaryOutputControl(false);
                          setState(() => _auxOutputLatchUi = false);
                        },
                        child: Container(
                          padding: const EdgeInsets.symmetric(
                              vertical: 14, horizontal: 12),
                          decoration: BoxDecoration(
                            color: Colors.grey[900],
                            borderRadius: BorderRadius.circular(12),
                            border: Border.all(color: Colors.white24),
                          ),
                          child: const Text(
                            'Hold: Aux output on',
                            textAlign: TextAlign.center,
                            style: TextStyle(color: Colors.white, fontSize: 16),
                          ),
                        ),
                      ),
                    ),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        const Text(
                          'Aux output',
                          style: TextStyle(color: Colors.white, fontSize: 16),
                        ),
                        Switch(
                          value: _auxOutputLatchUi,
                          onChanged: (value) {
                            setState(() => _auxOutputLatchUi = value);
                            bm.sendAuxillaryOutputControl(value);
                          },
                          activeColor: const Color(0xFFBB86FC),
                        ),
                      ],
                    ),
                    _buildSwitch(
                      'Timed pulse on startup',
                      bm.auxStartupTimed,
                      (value) {
                        bm.setAuxStartupTimed(value);
                        bm.refreshFromUi();
                        _saveManifoldConfigNow();
                      },
                    ),
                    _buildSwitch(
                      'Timed pulse on shutdown',
                      bm.auxShutdownTimed,
                      (value) {
                        bm.setAuxShutdownTimed(value);
                        bm.refreshFromUi();
                        _saveManifoldConfigNow();
                      },
                    ),
                    Padding(
                      padding: const EdgeInsets.symmetric(vertical: 8.0),
                      child: Row(
                        children: [
                          const Expanded(
                            child: Text(
                              'Duration unit:',
                              style: TextStyle(color: Colors.white, fontSize: 16),
                            ),
                          ),
                          DropdownButton<int>(
                            value: bm.auxTimeUnit.clamp(0, 3),
                            dropdownColor: Colors.grey[850],
                            style: const TextStyle(color: Colors.white),
                            items: [
                              for (var i = 0; i < 4; i++)
                                DropdownMenuItem(
                                  value: i,
                                  child: Text(unitLabels[i]),
                                ),
                            ],
                            onChanged: (v) {
                              if (v == null) return;
                              bm.auxTimeUnit = v;
                              bm.refreshFromUi();
                              _saveManifoldConfigNow();
                              setState(() {});
                            },
                          ),
                        ],
                      ),
                    ),
                    _buildKeyboardInputRow(
                      'Pulse duration',
                      auxPulseDurationController,
                      isNumberInput: true,
                      limitChar: 3,
                      tooltipTitle: 'Pulse duration',
                      tooltip:
                          'Length of each aux pulse (0–255) in the selected duration unit.',
                      saveWhenKeyboardDone: true,
                    ),
                    _buildKeyboardInputRow(
                      'Interval (cycles)',
                      auxIntervalCyclesController,
                      isNumberInput: true,
                      limitChar: 3,
                      tooltipTitle: 'Interval (cycles)',
                      tooltip:
                          'Interval between pulses in cycle count (0–255), as on the wireless controller.',
                      saveWhenKeyboardDone: true,
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

  Widget _buildWifiUpdateSection(BuildContext context) {
    final bm = context.read<BLEManager>();
    final canUpdate = wifiSsidController.text.trim().isNotEmpty &&
        wifiPassController.text.trim().isNotEmpty;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Wi-Fi / Update',
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
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                _buildKeyboardInputRow(
                  'SSID',
                  wifiSsidController,
                  onChanged: (_) => setState(() {}),
                  limitChar: 49,
                  tooltipTitle: 'Wi-Fi SSID',
                  tooltip:
                      'Network the manifold uses to download updates. Saved on this phone when you save settings.',
                  saveWhenKeyboardDone: true,
                ),
                _buildKeyboardInputRow(
                  'PASS',
                  wifiPassController,
                  obscureText: true,
                  onChanged: (_) => setState(() {}),
                  limitChar: 49,
                  tooltipTitle: 'Wi-Fi password',
                  tooltip: 'Saved on this phone when you save settings.',
                  saveWhenKeyboardDone: true,
                ),
                const SizedBox(height: 8),
                ElevatedButton(
                  onPressed: !canUpdate
                      ? null
                      : () => _showConfirm(
                            title: 'Begin update Wi-Fi service?',
                            message:
                                'Uses the credentials above to download firmware on the manifold. If something goes wrong, open https://oasman.dev on a computer and flash manually. Continue?',
                            confirmLabel: 'Start',
                            onConfirm: () {
                              bm.sendStartWebUpdate(
                                wifiSsidController.text.trim(),
                                wifiPassController.text.trim(),
                              );
                              _persistPhoneSettings();
                              if (mounted) {
                                ScaffoldMessenger.of(context).showSnackBar(
                                  const SnackBar(
                                    content: Text('Start web update sent'),
                                    duration: Duration(seconds: 2),
                                  ),
                                );
                              }
                            },
                          ),
                  style: ElevatedButton.styleFrom(
                    padding: const EdgeInsets.symmetric(
                        horizontal: 20, vertical: 12),
                    shape: RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(12),
                    ),
                  ),
                  child: const Text('Start Software Update'),
                ),
                const SizedBox(height: 12),
                Selector<BLEManager, String>(
                  selector: (_, m) => m.updateStatus,
                  builder: (context, statusText, _) {
                    return _readOnlyStatusRow(
                      'Manifold Update Status:',
                      statusText.isEmpty ? '—' : statusText,
                    );
                  },
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildRebootTurnOffButton(BuildContext context) {
    return Selector<BLEManager, bool>(
      selector: (_, m) => m.vehicleOn,
      builder: (context, accOn, _) {
        final bm = context.read<BLEManager>();
        return Padding(
          padding: const EdgeInsets.only(top: 8, bottom: 24),
          child: ElevatedButton(
            onPressed: () => _showConfirm(
              title: 'Reboot/Turn Off?',
              message: accOn
                  ? 'The manifold will reboot.'
                  : 'The manifold will shut down.',
              confirmLabel: accOn ? 'Reboot' : 'Shut Down',
              onConfirm: () {
                if (accOn) {
                  bm.sendRebootManifold();
                } else {
                  bm.sendTurnOffManifold();
                }
              },
            ),
            style: ElevatedButton.styleFrom(
              padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(12),
              ),
            ),
            child: Text(
              accOn ? 'Reboot' : 'Shut Down',
              style: const TextStyle(fontSize: 16),
            ),
          ),
        );
      },
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
    bool obscureText = false,
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
              obscureText: obscureText,
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
