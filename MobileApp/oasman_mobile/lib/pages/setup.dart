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
import 'package:permission_handler/permission_handler.dart';
import 'package:wifi_scan/wifi_scan.dart';
import '../theme/app_theme.dart';

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

/// One drill-down group on the settings screen.
class _SettingsSection {
  const _SettingsSection({
    required this.title,
    required this.icon,
    required this.subtitle,
    required this.builder,
    this.requiresConnection = false,
  });

  final String title;
  final IconData icon;
  final String subtitle;
  final Widget Function(BuildContext context) builder;

  /// Section talks to the manifold, so it is unusable without a link.
  final bool requiresConnection;
}

class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => SettingsPageState();
}

class SettingsPageState extends State<SettingsPage> {
  late BLEManager bleManager;
  bool _bleListenerAttached = false;
  int _lastSyncedConfigRevision = -1;

  /// Which drill-down section is open; null shows the section list.
  /// Stored by title so the list can be rebuilt without stale indices.
  String? _openSectionTitle;

  /// Nearby Wi-Fi networks for the SSID picker.
  List<String> _wifiNetworks = [];
  bool _wifiScanning = false;
  String? _wifiScanError;

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
  late TextEditingController bagStretchBelowController;
  late TextEditingController bagStretchPressureController;
  late TextEditingController compressorCrankOffsetController;
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
    bagStretchBelowController.text =
        bm.AirUpBagStretchTriggerBelowPressure.toString();
    bagStretchPressureController.text = bm.AirUpBagStretchPressure.toString();
    compressorCrankOffsetController.text = bm.compressorCrankOffset.toString();
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
    bagStretchBelowController.dispose();
    bagStretchPressureController.dispose();
    compressorCrankOffsetController.dispose();
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
    await prefs.setBool(
        '_showAllBluetoothDevices', globalSettings!.showAllBluetoothDevices);
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
      final stretchBelow = int.tryParse(bagStretchBelowController.text.trim());
      if (stretchBelow != null) {
        bm.AirUpBagStretchTriggerBelowPressure = stretchBelow.clamp(0, 255);
      }
      final stretchPressure =
          int.tryParse(bagStretchPressureController.text.trim());
      if (stretchPressure != null) {
        bm.AirUpBagStretchPressure = stretchPressure.clamp(0, 255);
      }

      final crankOffset =
          int.tryParse(compressorCrankOffsetController.text.trim());
      if (crankOffset != null) {
        bm.compressorCrankOffset = crankOffset.clamp(0, 255);
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
        icon: Icon(Icons.open_in_new, size: 20, color: AppTheme.accent(context)),
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
        icon: Icon(Icons.open_in_new, size: 20, color: AppTheme.accent(context)),
        label: const Text(
          'Privacy policy',
          style: TextStyle(color: Colors.white70, fontSize: 16),
        ),
      ),
    );
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

  /// Shared style for the settings' in-page action buttons (Learn Fob,
  /// Calibrate…, Allow New Controller, and friends). They were bare
  /// [TextButton]s, which on this dark background read as plain labels rather
  /// than something you can press. Dialog actions and the external links are
  /// deliberately left as text.
  ButtonStyle _actionButtonStyle(BuildContext context) {
    return OutlinedButton.styleFrom(
      foregroundColor: AppTheme.accent(context),
      side: BorderSide(color: AppTheme.accent(context)),
      backgroundColor: const Color(0xFF1E1E1E),
      disabledForegroundColor: Colors.white38,
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(10),
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
    bagStretchBelowController = TextEditingController();
    bagStretchPressureController = TextEditingController();
    compressorCrankOffsetController = TextEditingController();
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
      bagStretchBelowController.text =
          bm.AirUpBagStretchTriggerBelowPressure.toString();
      bagStretchPressureController.text = bm.AirUpBagStretchPressure.toString();
      compressorCrankOffsetController.text = bm.compressorCrankOffset.toString();
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

  /// Theme picker. The presets mirror the Wireless_Controller's "Theme Colors"
  /// setting one for one, so a phone and a controller can be set to the same
  /// look. Selecting one repaints the app immediately and is saved on the
  /// phone; it is not sent to the manifold.
  Widget _buildThemeSection() {
    return Consumer<ThemeProvider>(
      builder: (context, themeProvider, child) {
        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Accent color used throughout the app. These are the same presets '
              'the Wireless Controller offers.',
              style: TextStyle(color: Colors.white54, fontSize: 13),
            ),
            const SizedBox(height: 16),
            Container(
              padding: const EdgeInsets.symmetric(horizontal: 16.0),
              decoration: BoxDecoration(
                border: Border(
                  left: BorderSide(color: AppTheme.accent(context), width: 2),
                ),
              ),
              child: Column(
                children: [
                  for (final preset in ThemePreset.values)
                    RadioListTile<ThemePreset>(
                      title: Text(preset.label,
                          style: const TextStyle(color: Colors.white)),
                      secondary: _buildThemeSwatch(preset),
                      value: preset,
                      groupValue: themeProvider.preset,
                      onChanged: (value) {
                        if (value != null) themeProvider.setPreset(value);
                      },
                      activeColor: AppTheme.accent(context),
                    ),
                ],
              ),
            ),
          ],
        );
      },
    );
  }

  /// All three shades of a preset, so the choice is visible before picking it.
  Widget _buildThemeSwatch(ThemePreset preset) {
    return ClipRRect(
      borderRadius: BorderRadius.circular(6),
      child: SizedBox(
        width: 54,
        height: 22,
        child: Row(
          children: [
            for (final color in [preset.light, preset.medium, preset.dark])
              Expanded(child: Container(color: color)),
          ],
        ),
      ),
    );
  }

  Widget _buildUploadImageSection() {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            'Car image shown on the home screen.',
            style: TextStyle(color: Colors.white54, fontSize: 13),
          ),
          const SizedBox(height: 16),
          GestureDetector(
            onTap: _pickImage,
            child: Container(
              height: 200,
              width: 100,
              decoration: BoxDecoration(
                border: Border.all(color: AppTheme.accent(context), width: 2),
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
                              size: 30, color: AppTheme.accent(context)),
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
            OutlinedButton(
              style: _actionButtonStyle(context),
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
                  Icon(Icons.delete, size: 20, color: AppTheme.accent(context)),
                  const SizedBox(height: 8),
                  Text(
                    'Remove Image',
                    style: TextStyle(color: AppTheme.accent(context)),
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

  /// Settings are grouped into drill-down sections (the standard mobile
  /// settings pattern) rather than one very long scroll. Section names follow
  /// the Wireless_Controller's own section list where the section exists on
  /// both; the controller uses a dropdown only because it has no room for a
  /// list on a 2.8" screen.
  List<_SettingsSection> get _sections => [
        // Config leads: it holds the passkey and the device-visibility toggle,
        // which are what you need before a manifold will connect at all.
        _SettingsSection(
          title: 'Config',
          icon: Icons.settings_outlined,
          subtitle: 'Passkey, compressor PSI, bag limits',
          builder: _buildConfigSection,
        ),
        _SettingsSection(
          title: 'Status',
          icon: Icons.monitor_heart_outlined,
          subtitle: 'Compressor, ACC, e-brake, reboot',
          requiresConnection: true,
          builder: (context) => Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _buildStatusSection(context),
              _buildRebootTurnOffButton(context),
            ],
          ),
        ),
        _SettingsSection(
          title: 'Basic settings',
          icon: Icons.tune,
          subtitle: 'Maintain, rise/fall, safety mode, key fob',
          requiresConnection: true,
          builder: _buildBasicSettingsPage,
        ),
        _SettingsSection(
          title: 'Levelling Mode',
          icon: Icons.height,
          subtitle: 'Pressure or level sensor, calibration',
          requiresConnection: true,
          builder: _buildLevellingPage,
        ),
        _SettingsSection(
          title: 'Auxillary Output',
          icon: Icons.bolt_outlined,
          subtitle: 'Manual control and timed pulses',
          requiresConnection: true,
          builder: _buildAuxillaryOutputSection,
        ),
        _SettingsSection(
          title: 'Game Controller',
          icon: Icons.sports_esports_outlined,
          subtitle: 'Pair, un-pair, disconnect',
          requiresConnection: true,
          builder: _buildGameControllerSection,
        ),
        _SettingsSection(
          title: 'Wifi / Update',
          icon: Icons.system_update_alt,
          subtitle: 'Manifold firmware update over Wi-Fi',
          requiresConnection: true,
          builder: _buildWifiUpdateSection,
        ),
        _SettingsSection(
          title: 'Units',
          icon: Icons.straighten,
          subtitle: 'PSI or Bar',
          builder: (context) => _buildUnitsSection(),
        ),
        _SettingsSection(
          title: 'Appearance',
          icon: Icons.palette_outlined,
          subtitle: 'Theme colors and the home screen car image',
          builder: (context) => Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Padding(
                padding: EdgeInsets.only(top: 24, bottom: 8),
                child: Text(
                  'Theme',
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: 16,
                    fontWeight: FontWeight.w600,
                  ),
                ),
              ),
              _buildThemeSection(),
              const Divider(color: Colors.white12, height: 32),
              const Text(
                'Car Image',
                style: TextStyle(
                  color: Colors.white,
                  fontSize: 16,
                  fontWeight: FontWeight.w600,
                ),
              ),
              _buildUploadImageSection(),
            ],
          ),
        ),
        _SettingsSection(
          title: 'About',
          icon: Icons.info_outline,
          subtitle: 'Website and privacy policy',
          builder: (context) => Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _buildWebsiteLinkSection(),
              _buildPrivacyPolicySection(),
            ],
          ),
        ),
      ];

  /// Leaving a subsection is a natural save point, mirroring the existing
  /// save-on-leave-page behaviour.
  void _closeSection() {
    if (_openSectionTitle == null) return;
    setState(() => _openSectionTitle = null);
    _saveManifoldConfigNow(showSnackBar: false);
  }

  @override
  Widget build(BuildContext context) {
    if (!_settingsLoaded) {
      return const Center(child: CircularProgressIndicator());
    }

    final open = _openSectionTitle == null
        ? null
        : _sections.where((s) => s.title == _openSectionTitle).firstOrNull;

    // Android back should step out of a subsection before leaving the app.
    return PopScope(
      canPop: open == null,
      onPopInvokedWithResult: (didPop, _) {
        if (!didPop) _closeSection();
      },
      // The section list stays mounted (offstage) while a section is open, so
      // its ScrollPosition is never disposed and the list is still where the
      // user left it on the way back. Swapping the widget out instead would
      // build a fresh list every time, always starting at the top.
      child: Stack(
        fit: StackFit.expand,
        children: [
          Offstage(offstage: open != null, child: _buildSectionList()),
          if (open != null) _buildSectionDetail(open),
        ],
      ),
    );
  }

  Widget _buildSectionList() {
    return Column(
      children: [
        const Padding(
          padding: EdgeInsets.symmetric(vertical: 12),
          child: Text(
            'Settings',
            style: TextStyle(
              fontSize: 25,
              fontWeight: FontWeight.w600,
              fontFamily: 'Roboto',
              color: Colors.white,
            ),
          ),
        ),
        Expanded(
          child: Selector<BLEManager, bool>(
            selector: (_, m) => m.isConnected(),
            builder: (context, connected, _) {
              return ListView(
                padding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
                children: [
                  // No global "connect a manifold" card here - each row that
                  // needs a link already says so.
                  for (final s in _sections)
                    _buildSectionTile(s, connected || !s.requiresConnection),
                ],
              );
            },
          ),
        ),
      ],
    );
  }

  Widget _buildSectionTile(_SettingsSection section, bool enabled) {
    return Opacity(
      opacity: enabled ? 1.0 : 0.4,
      child: ListTile(
        contentPadding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
        leading: Icon(section.icon, color: AppTheme.accent(context)),
        title: Text(
          section.title,
          style: const TextStyle(color: Colors.white, fontSize: 17),
        ),
        subtitle: Text(
          enabled ? section.subtitle : 'Connect a manifold first',
          style: const TextStyle(color: Colors.white54, fontSize: 13),
        ),
        trailing: const Icon(Icons.chevron_right, color: Colors.white38),
        onTap: enabled
            ? () {
                setState(() => _openSectionTitle = section.title);
                // Populate the SSID picker up front, like the controller
                // scanning when its dropdown opens.
                if (section.title == 'Wifi / Update') _scanWifiNetworks();
              }
            : null,
      ),
    );
  }

  Widget _buildSectionDetail(_SettingsSection section) {
    final keyboardInset = MediaQuery.viewInsetsOf(context).bottom;
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.fromLTRB(4, 8, 16, 8),
          child: Row(
            children: [
              IconButton(
                icon: const Icon(Icons.arrow_back, color: Colors.white),
                onPressed: _closeSection,
                tooltip: 'Back to settings',
              ),
              Expanded(
                child: Text(
                  section.title,
                  style: const TextStyle(
                    fontSize: 22,
                    fontWeight: FontWeight.w600,
                    fontFamily: 'Roboto',
                    color: Colors.white,
                  ),
                ),
              ),
            ],
          ),
        ),
        Expanded(
          child: SingleChildScrollView(
            padding: EdgeInsets.fromLTRB(16, 0, 16, 16 + keyboardInset),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // A manifold section stays usable only while connected; drop
                // back to the list if the link goes away mid-edit.
                Selector<BLEManager, bool>(
                  selector: (_, m) => m.isConnected(),
                  builder: (context, connected, _) {
                    if (section.requiresConnection && !connected) {
                      return const ConnectManifoldCard();
                    }
                    return section.builder(context);
                  },
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  /// Config / RF / levelling fields only change with GETCONFIGVALUES or local
  /// edits, not with high-frequency STATUSREPORT packets.
  Widget _buildManifoldConfigSelector(Widget Function(BLEManager bm) child) {
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
      builder: (context, _, __) => child(context.read<BLEManager>()),
    );
  }

  Widget _buildBasicSettingsPage(BuildContext context) =>
      _buildManifoldConfigSelector(_buildBasicSettingsSection);

  Widget _buildLevellingPage(BuildContext context) =>
      _buildManifoldConfigSelector(_buildLevellingSection);

  Widget _buildStatusSection(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
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
                decoration: BoxDecoration(
                  border: Border(
                    left: BorderSide(color: AppTheme.accent(context), width: 2),
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
                          activeColor: AppTheme.accent(context),
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

  Widget _buildConfigSection(BuildContext context) {
    final bm = bleManager;
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: AppTheme.accent(context), width: 2),
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
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Flexible(
                      child: Row(
                        crossAxisAlignment: CrossAxisAlignment.center,
                        children: [
                          const Flexible(
                            child: Text(
                              'Show all bluetooth devices',
                              style: TextStyle(
                                color: Colors.white,
                                fontSize: 16,
                              ),
                            ),
                          ),
                          IconButton(
                            icon: const Icon(Icons.help_outline,
                                size: 20, color: Colors.grey),
                            onPressed: () {
                              showInfoDialog(
                                context,
                                'Show all bluetooth devices',
                                'Normally the scan only lists devices advertising the OASMan service. Some phones never report that information, so no devices show up at all. Turn this on to list every bluetooth device nearby and pick the manifold by name.',
                              );
                            },
                          ),
                        ],
                      ),
                    ),
                    _buildSwitch(
                      '',
                      globalSettings!.showAllBluetoothDevices,
                      (value) async {
                        setState(() {
                          globalSettings!.showAllBluetoothDevices = value;
                        });
                        final prefs = await SharedPreferences.getInstance();
                        await prefs.setBool('_showAllBluetoothDevices', value);
                      },
                    ),
                  ],
                ),
                // heightSensorMode is part of the selector so the bag-stretch
                // rows appear/disappear as soon as the levelling mode changes,
                // matching updateLevelModeOptionsVisibility() on the controller.
                Selector<BLEManager, (bool, bool)>(
                  selector: (_, m) =>
                      (m.connectedDevice != null, m.heightSensorMode),
                  builder: (context, sel, _) {
                    final (hasDevice, heightSensorMode) = sel;
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
                                  'Compressor Crank Offset (Seconds)',
                                  compressorCrankOffsetController,
                                  isNumberInput: true,
                                  limitChar: 3,
                                  tooltipTitle: 'Compressor Crank Offset',
                                  tooltip:
                                      'Seconds the compressor stays off after accessory power comes on, so it does not load the electrical system while the engine is cranking (0–255).',
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
                                // Bag stretch only applies to pressure-sensor
                                // mode; the controller hides these in level
                                // sensor mode.
                                if (!heightSensorMode) ...[
                                  _buildKeyboardInputRow(
                                    'Bag Stretch Below PSI',
                                    bagStretchBelowController,
                                    isNumberInput: true,
                                    limitChar: 3,
                                    tooltipTitle: 'Bag Stretch Below PSI',
                                    tooltip:
                                        'Only air up with a stretch step when the bag is currently below this pressure (default 40). Ignored in level sensor mode.',
                                    saveWhenKeyboardDone: true,
                                  ),
                                  _buildKeyboardInputRow(
                                    'Bag Stretch PSI',
                                    bagStretchPressureController,
                                    isNumberInput: true,
                                    limitChar: 3,
                                    tooltipTitle: 'Bag Stretch PSI',
                                    tooltip:
                                        'Extra PSI added on top of the target when airing up, to unroll the bag before settling back to the target. 0 disables bag stretch.',
                                    saveWhenKeyboardDone: true,
                                  ),
                                ],
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
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: AppTheme.accent(context), width: 2),
              ),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                const SizedBox(height: 8),
                OutlinedButton(
                  style: _actionButtonStyle(context),
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
                const SizedBox(height: 8),
                OutlinedButton(
                  style: _actionButtonStyle(context),
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
                const SizedBox(height: 8),
                OutlinedButton(
                  style: _actionButtonStyle(context),
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
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: AppTheme.accent(context), width: 2),
              ),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                _buildSwitch(
                  bm.heightSensorMode ? 'Maintain Height' : 'Maintain Pressure',
                  bm.maintainPressure,
                  (value) {
                    bm.maintainPressure = value;
                    bm.refreshFromUi();
                    _saveManifoldConfigNow();
                  },
                ),
                if (!bm.heightSensorMode)
                  _buildSwitch(
                    'Height levelling',
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
                      activeColor: AppTheme.accent(context),
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
                      activeColor: AppTheme.accent(context),
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
                const SizedBox(height: 8),
                OutlinedButton(
                  style: _actionButtonStyle(context),
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
                const SizedBox(height: 8),
                OutlinedButton(
                  style: _actionButtonStyle(context),
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
                // AI learn progress + reset live at the end of Basic settings,
                // matching the Wireless_Controller. The old separate ML/AI
                // section (and its "Trained" Y/N grid) no longer exists there.
                Selector<BLEManager, int>(
                  selector: (_, m) => m.aiLearnPercent,
                  builder: (context, learnPct, _) => _readOnlyStatusRow(
                    'Sample Learn Progress:',
                    '$learnPct%',
                  ),
                ),
                const SizedBox(height: 8),
                OutlinedButton(
                  style: _actionButtonStyle(context),
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
          ),
        ],
      ),
    );
  }

  /// Switching sensor mode makes the manifold clear its learned data
  /// (setheightSensorMode -> clearPressureData), so confirm first. Mirrors the
  /// Wireless_Controller, which asks in both directions and cannot be
  /// dismissed by tapping away (forceButtonPress). Nothing is mutated until
  /// the user confirms, so Cancel simply leaves the radio where it was.
  Future<void> _confirmSensorModeChange(BLEManager bm, bool? wantHeightMode) async {
    if (wantHeightMode == null || wantHeightMode == bm.heightSensorMode) return;
    final go = await showDialog<bool>(
      context: context,
      barrierDismissible: false,
      builder: (ctx) => AlertDialog(
        title: const Text('Change Sensor Mode?'),
        content: const Text('Learned data will be deleted.'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('OK'),
          ),
        ],
      ),
    );
    if (go != true || !mounted) return;
    bm.heightSensorMode = wantHeightMode;
    bm.refreshFromUi();
    _saveManifoldConfigNow();
  }

  Widget _buildLevellingSection(BLEManager bm) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 24.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: AppTheme.accent(context), width: 2),
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
                  onChanged: (v) => _confirmSensorModeChange(bm, v),
                  activeColor: AppTheme.accent(context),
                ),
                RadioListTile<bool>(
                  title: const Text('Level Sensor',
                      style: TextStyle(color: Colors.white)),
                  value: true,
                  groupValue: bm.heightSensorMode,
                  onChanged: (v) => _confirmSensorModeChange(bm, v),
                  activeColor: AppTheme.accent(context),
                ),
                if (bm.heightSensorMode) ...[
                  const SizedBox(height: 8),
                  const SizedBox(height: 8),
                  OutlinedButton(
                    style: _actionButtonStyle(context),
                    onPressed: () => _showConfirm(
                      title: 'Calibrate Min Height?',
                      message:
                          'Please air out your car to the lowest it goes before you click ok',
                      confirmLabel: 'OK',
                      onConfirm: () {
                        bm.sendCalibrateHeightSensors(
                            HeightCalibrationType.min);
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
                  const SizedBox(height: 8),
                  OutlinedButton(
                    style: _actionButtonStyle(context),
                    onPressed: () => _showConfirm(
                      title: 'Calibrate Max Height?',
                      message:
                          'Raise your vehicle as high as it can go before you click ok',
                      confirmLabel: 'OK',
                      onConfirm: () {
                        bm.sendCalibrateHeightSensors(
                            HeightCalibrationType.max);
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
                  const SizedBox(height: 8),
                  OutlinedButton(
                    style: _actionButtonStyle(context),
                    onPressed: () => _showConfirm(
                      title: 'Calibrate Minimum Ride Height?',
                      message:
                          'Set your vehicle to the lowest ride height you want to allow before you click ok. This is used for maintain height (height sensor only). Only use this after calibrating min and max.',
                      confirmLabel: 'OK',
                      onConfirm: () {
                        bm.sendCalibrateHeightSensors(
                            HeightCalibrationType.minRideHeight);
                        if (mounted) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(
                              content: Text('Calibrated min ride height'),
                              duration: Duration(seconds: 2),
                            ),
                          );
                        }
                      },
                    ),
                    child: const Text('Calibrate Min Ride Height'),
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
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 16.0),
                decoration: BoxDecoration(
                  border: Border(
                    left: BorderSide(color: AppTheme.accent(context), width: 2),
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
                          activeColor: AppTheme.accent(context),
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

  /// Scan for nearby networks and populate the SSID dropdown. Mirrors the
  /// Wireless_Controller, which scans when its SSID dropdown is opened.
  Future<void> _scanWifiNetworks() async {
    if (_wifiScanning) return;
    setState(() {
      _wifiScanning = true;
      _wifiScanError = null;
    });
    try {
      final can = await WiFiScan.instance.canStartScan();
      if (can != CanStartScan.yes) {
        // Most often the permission prompt has not been granted yet; ask, then
        // re-check once before giving up.
        await Permission.locationWhenInUse.request();
        final retry = await WiFiScan.instance.canStartScan();
        if (retry != CanStartScan.yes) {
          if (mounted) {
            setState(() {
              _wifiScanning = false;
              _wifiScanError = 'Cannot scan for networks on this device';
            });
          }
          return;
        }
      }
      await WiFiScan.instance.startScan();
      final canGet = await WiFiScan.instance.canGetScannedResults();
      if (canGet != CanGetScannedResults.yes) {
        if (mounted) {
          setState(() {
            _wifiScanning = false;
            _wifiScanError = 'Cannot read scan results on this device';
          });
        }
        return;
      }
      final results = await WiFiScan.instance.getScannedResults();
      // Drop hidden networks (blank SSID) and duplicates from multi-band APs.
      final names = <String>{
        for (final r in results)
          if (r.ssid.trim().isNotEmpty) r.ssid,
      }.toList()
        ..sort((a, b) => a.toLowerCase().compareTo(b.toLowerCase()));
      if (!mounted) return;
      setState(() {
        _wifiNetworks = names;
        _wifiScanning = false;
        _wifiScanError = names.isEmpty ? 'No networks found' : null;
      });
    } catch (e) {
      debugPrint('Wi-Fi scan failed: $e');
      if (!mounted) return;
      setState(() {
        _wifiScanning = false;
        _wifiScanError = 'Wi-Fi scan failed';
      });
    }
  }

  /// SSID picker. The saved SSID is always an option so the current value
  /// survives a scan that no longer sees that network; if scanning is refused
  /// outright the user can still type the name in by hand.
  Widget _buildSsidRow(BuildContext context) {
    final saved = wifiSsidController.text.trim();
    final options = <String>[
      if (saved.isNotEmpty) saved,
      ..._wifiNetworks.where((n) => n != saved),
    ];
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              // Label kept to its intrinsic width so the dropdown - and with
              // it the popup menu, which inherits the button's width - gets as
              // much room as possible for long SSIDs.
              const Text(
                'SSID',
                style: TextStyle(color: Colors.white, fontSize: 16),
              ),
              IconButton(
                icon: const Icon(Icons.help_outline,
                    size: 20, color: Colors.grey),
                onPressed: () => showInfoDialog(
                  context,
                  'Wi-Fi SSID',
                  'Network the manifold uses to download updates. Pick one of the networks near you. Saved on this phone when you save settings.',
                ),
              ),
              Expanded(
                child: DropdownButton<String>(
                  isExpanded: true,
                  value: saved.isNotEmpty ? saved : null,
                  hint: Text(
                    _wifiScanning ? 'Scanning…' : 'Select network',
                    style: const TextStyle(color: Colors.white54),
                  ),
                  dropdownColor: Colors.grey[850],
                  style: const TextStyle(color: Colors.white),
                  // Variable item height: a long SSID wraps onto extra lines
                  // instead of being clipped. Fixed-height items would force
                  // a single truncated line.
                  itemHeight: null,
                  items: [
                    for (final n in options)
                      DropdownMenuItem(
                        value: n,
                        child: Padding(
                          padding: const EdgeInsets.symmetric(vertical: 8),
                          // No ellipsis here - the open menu must show the
                          // whole name so networks are distinguishable.
                          child: Text(n, softWrap: true),
                        ),
                      ),
                  ],
                  // Only the collapsed button abbreviates; it has one line.
                  selectedItemBuilder: (context) => [
                    for (final n in options)
                      Align(
                        alignment: Alignment.centerLeft,
                        child: Text(
                          n,
                          maxLines: 1,
                          overflow: TextOverflow.ellipsis,
                        ),
                      ),
                  ],
                  onChanged: (v) {
                    if (v == null) return;
                    wifiSsidController.text = v;
                    _persistPhoneSettings();
                    setState(() {});
                  },
                ),
              ),
              IconButton(
                icon: _wifiScanning
                    ? const SizedBox(
                        width: 18,
                        height: 18,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : const Icon(Icons.refresh, color: Colors.grey),
                tooltip: 'Scan for networks',
                onPressed: _wifiScanning ? null : _scanWifiNetworks,
              ),
            ],
          ),
          if (_wifiScanError != null) ...[
            Text(
              _wifiScanError!,
              style: const TextStyle(color: Colors.white54, fontSize: 12),
            ),
            // Fall back to manual entry so a refused scan can't lock the user
            // out of updating.
            _buildKeyboardInputRow(
              'Enter manually',
              wifiSsidController,
              onChanged: (_) => setState(() {}),
              limitChar: 49,
              saveWhenKeyboardDone: true,
            ),
          ],
        ],
      ),
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
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16.0),
            decoration: BoxDecoration(
              border: Border(
                left: BorderSide(color: AppTheme.accent(context), width: 2),
              ),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                _buildSsidRow(context),
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
                // The controller shows the connected manifold's address on this
                // page too (ble_getMAC), which is what builders read back when
                // asking for support.
                _readOnlyStatusRow(
                  'Manifold MAC:',
                  bm.connectedDevice?.remoteId.str ?? 'Not connected',
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
              // Was a help dialog on the section title; the title now lives in
              // the header, so the same guidance is shown inline instead.
              const Text(
                'Choose which pressure unit you prefer. Default is PSI.',
                style: TextStyle(color: Colors.white54, fontSize: 13),
              ),
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 16.0),
                decoration: BoxDecoration(
                  border: Border(
                    left: BorderSide(color: AppTheme.accent(context), width: 2),
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
                      activeColor: AppTheme.accent(context),
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
                      activeColor: AppTheme.accent(context),
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
        // Builder: this is a top-level function, so it has no context of its
        // own to read the theme accent from.
        Builder(
          builder: (context) => Switch(
              value: value,
              onChanged: onChanged,
              activeColor: AppTheme.accent(context)),
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
