import 'dart:async';
import 'dart:convert'; // for utf8.encode

import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:oasman_mobile/pages/popup/invalidkey.dart';
import 'package:permission_handler/permission_handler.dart';
import "dart:typed_data";
import 'models/appSettings.dart';

/// OASMan BLE service UUID; filter scans to only show manifold devices.
const String oasmanServiceUuid = '679425c8-d3b4-4491-9eb2-3e3d15b625f0';

/// Full [BTOasPacket] wire size (cmd + sender + recipient + args[100]).
const int btoasPacketSize = 104;

/// Args payload size inside a [BTOasPacket].
const int btoasArgsSize = 100;

class BTOasIdentifier {
  static const int IDLE = 0;
  static const int STATUSREPORT = 1;
  static const int AIRUP = 2;
  static const int AIROUT = 3;
  static const int AIRSM = 4;
  static const int SAVETOPROFILE = 5;
  static const int READPROFILE = 6;
  static const int AIRUPQUICK = 7;
  static const int BASEPROFILE = 8;
  static const int SETAIRHEIGHT = 9;
  static const int RAISEONPRESSURESET = 11;
  static const int REBOOT = 12;
  static const int CALIBRATE = 13;
  static const int STARTWEB = 14;
  static const int ASSIGNRECEPIENT = 15;
  static const int MESSAGE = 16;
  static const int SAVECURRENTPRESSURESTOPROFILE = 17;
  static const int PRESETREPORT = 18;
  static const int GETCONFIGVALUES = 21;
  static const int AUTHPACKET = 22;
  static const int COMPRESSORSTATUS = 24;
  static const int TURNOFF = 25;
  static const int DETECTPRESSURESENSORS = 27;
  static const int RESETAIPKT = 29;
  static const int BP32PKT = 30;
  static const int BROADCASTNAME = 35;
  static const int UPDATESTATUSREQUEST = 36;
  static const int RFCOMMAND = 37;
  static const int AUXILLARYOUTPUTCONTROL = 38;
  static const int CALIBRATEHEIGHTSENSORS = 39;
}

/// Which per-wheel height calibration point CALIBRATEHEIGHTSENSORS captures.
/// Mirrors HeightCalibrationType in BTOas.h.
class HeightCalibrationType {
  static const int min = 0;
  static const int max = 1;
  static const int minRideHeight = 2;
}

/// GETCONFIGVALUES read request (cmd only, args zeroed). Reused on every connect.
final List<int> kConfigReadPacket = List<int>.filled(btoasPacketSize, 0)
  ..[0] = BTOasIdentifier.GETCONFIGVALUES & 0xFF
  ..[1] = BTOasIdentifier.GETCONFIGVALUES >> 8;

/// Live status bits carried in STATUSREPORT's bittset. Mirrors
/// StatusPacketBittset in BTOas.h - these are live state, not config.
class StatusPacketBittset {
  static const int COMPRESSOR_FROZEN = 0;
  static const int COMPRESSOR_STATUS_ON = 1;
  static const int ACC_STATUS_ON = 2;
  static const int TIMER_STATUS_EXPIRED = 3; // not really used
  static const int CLOCK = 4; // not really used
  static const int EBRAKE_STATUS_ON = 5;
  /// isAnyWheelActive() on the manifold: a corner is actively filling/dumping
  /// to a target. The Wireless_Controller shows an "Adjusting" label for this.
  static const int ADJUSTMENT_IN_PROGRESS = 6;
}

/// Config flags in ConfigValuesPacket.configFlagsBits (GETCONFIGVALUES).
class ConfigFlagsBit {
  static const int CONFIG_MAINTAIN_PRESSURE = 0;
  static const int CONFIG_RISE_ON_START = 1;
  static const int CONFIG_AIR_OUT_ON_SHUTOFF = 2;
  static const int CONFIG_HEIGHT_SENSOR_MODE = 3;
  static const int CONFIG_SAFETY_MODE = 4;
  static const int CONFIG_AI_STATUS_ENABLED = 5; // retired: reserved on the wire, no longer used by app/manifold. Do not reuse.
  static const int CONFIG_SENSORLESS_LEVELING = 6;
}

class BLEByte {
  BLEByte(this.value);
  int value;

  BLEByte from(int value) {
    return BLEByte(value);
  }

  int toByte() => value;
}

class BLEShort {
  BLEShort(this.value);
  int value;

  BLEShort from(int value) {
    return BLEShort(value);
  }

  int toShort() => value;
}

class BLEInt {
  BLEInt(this.value);
  int value;

  BLEInt from(int value) {
    return BLEInt(value);
  }

  int toInt() => value;
}

class BLEManager extends ChangeNotifier {
  final FlutterBluePlus flutterBlue = FlutterBluePlus(); // BLE instance
  BluetoothDevice? connectedDevice; // Currently connected device
  BluetoothCharacteristic?
      restCharacteristic; // Characteristic for writing data
  BluetoothCharacteristic?
      statusCharacteristic; // Characteristic for notifications
  BluetoothCharacteristic?
      valveControlCharacteristic; // Characteristic for notifications
  StreamSubscription<List<int>>? restStream;
  StreamSubscription<List<int>>? statusStream;
  StreamSubscription<OnConnectionStateChangedEvent>? _globalConnSub;
  Timer? _reconnectTimer;
  bool _autoReconnectEnabled = false;

  /// How long we wait for the manifold's AUTHPACKET reply before giving up on
  /// the link. Mirrors AUTH_TIMEOUT in OASMan_ESP32/src/bluetooth/ble.cpp - the
  /// manifold drops an un-authed client on the same budget. Ours is measured
  /// from when the auth packet is sent (i.e. after service discovery) so a slow
  /// Android discovery doesn't eat into it.
  static const Duration authTimeout = Duration(seconds: 5);

  /// True once the manifold has answered our auth packet with AUTHRESULT_SUCCESS.
  bool authenticated = false;
  Timer? _authTimer;

  /// Arm the auth watchdog. Called right after the auth packet goes out; a peer
  /// that never answers (wrong device, dead firmware) gets dropped instead of
  /// leaving the app stuck on a connection that will never work.
  void _startAuthWatchdog() {
    // The reply can already be in when we get here - the rest notify listener
    // is attached before the auth packet goes out, so a fast manifold can
    // answer while we're still awaiting the write. Never re-arm on a live link.
    if (authenticated) return;
    _authTimer?.cancel();
    _authTimer = Timer(authTimeout, () {
      _authTimer = null;
      if (authenticated || connectedDevice == null) return;
      debugPrint(
          'No auth response within ${authTimeout.inMilliseconds}ms - disconnecting');
      disconnectDevice();
      _scheduleReconnectScan();
    });
  }

  void _cancelAuthWatchdog() {
    _authTimer?.cancel();
    _authTimer = null;
  }

  /// Whether service discovery found the OASMan GATT characteristics, i.e. the
  /// peer is a manifold rather than some unrelated bluetooth device.
  bool get _hasManifoldCharacteristics =>
      restCharacteristic != null || statusCharacteristic != null;

  /// Start the background reconnect loop. Safe to call multiple times.
  void enableAutoReconnect() {
    _autoReconnectEnabled = true;
    _startGlobalConnListener();
    _scheduleReconnectScan();
  }

  /// Stop the background reconnect loop (e.g. when manually disconnecting).
  void disableAutoReconnect() {
    _autoReconnectEnabled = false;
    _reconnectTimer?.cancel();
    _reconnectTimer = null;
  }

  void _scheduleReconnectScan() {
    _reconnectTimer?.cancel();
    if (!_autoReconnectEnabled) return;
    if (connectedDevice != null) return;

    final pairedId = globalSettings?.pairedManifoldId ?? '';
    if (pairedId.isEmpty) return;

    _reconnectTimer = Timer(const Duration(seconds: 5), () {
      if (connectedDevice == null && _autoReconnectEnabled) {
        debugPrint('Auto-reconnect: starting scan...');
        startScan();
      }
    });
  }

  void _startGlobalConnListener() {
    _globalConnSub?.cancel();
    _globalConnSub =
        FlutterBluePlus.events.onConnectionStateChanged.listen((event) {
      debugPrint(
          'Global conn event: ${event.device.id} -> ${event.connectionState}');
      if (connectedDevice?.id == event.device.id &&
          event.connectionState == BluetoothConnectionState.disconnected) {
        debugPrint('Manifold disconnected!');
        _cancelAuthWatchdog();
        authenticated = false;
        connectedDevice = null;
        // Drop any held valve bits. The manifold does not close valves on
        // disconnect, so a stale mask here would be OR'd into the next press
        // after reconnect and re-open a valve the user never touched.
        valveControlValue = 0;
        vehicleOn = false;
        restCharacteristic = null;
        statusCharacteristic = null;
        valveControlCharacteristic = null;
        notifyListeners();
        _scheduleReconnectScan();
      }
    });
  }

  List<BluetoothDevice> devicesList = []; // List of discovered devices

  int passkey = int.parse(globalSettings!.passkeyText);

  Map<String, String> pressureValues = {
    "frontLeft": "-",
    "frontRight": "-",
    "rearLeft": "-",
    "rearRight": "-",
    "tankPressure": "-",
  };

  bool compressorOn = false;
  bool compressorFrozen = false;
  bool vehicleOn = false;
  bool ebrakeOn = false;

  /// A corner is actively filling/dumping toward a target (manifold's
  /// isAnyWheelActive). Drives the "Adjusting" indicator, same as the
  /// Wireless_Controller status bar.
  bool adjustmentInProgress = false;
  bool riseOnStart = false;
  bool maintainPressure = false;
  bool sensorlessLeveling = false;
  bool airOutOnShutoff = false;
  bool safetyMode = true;
  /// Mirrors ConfigFlagsBit::CONFIG_HEIGHT_SENSOR_MODE (preserved on save).
  bool heightSensorMode = false;
  String bleBroadcastName = '';

  /// Incremented when a GETCONFIGVALUES packet updates local config fields.
  int configRevision = 0;
  int compressorOnPSI = 0;
  int compressorOffPSI = 0;
  int systemShutoffTimeM = 15;
  int pressureSensorMax = 0;
  int bagMaxPressure = 0;
  int AirUpBagStretchTriggerBelowPressure = 0;
  int AirUpBagStretchPressure = 0;

  /// Seconds the manifold holds the compressor off after accessory power arrives.
  int compressorCrankOffset = 5;

  /// From STATUSREPORT args (AI learning UI).
  int aiLearnPercent = 0;
  // args8()[11] (byte 15) is reserved on the wire - it used to carry an
  // "AI ready" bittset per solenoid, but the manifold now always writes 0 and
  // neither client displays it. Do not resurrect it without a firmware change.


  /// RF key fob button preset indices on manifold (0–4 = presets 1–5).
  int rfButtonAPreset = 0;
  int rfButtonBPreset = 0;
  int rfButtonCPreset = 0;
  int rfButtonDPreset = 0;

  /// Auxillary output config (`AuxillaryOutputModePayload` at args[24..27], matches BTOas / Wireless_Controller).
  static const int _auxStartupTimedEnum = 1;
  static const int _auxShutdownTimedEnum = 2;
  static const int auxStartupTimedMask = 1 << _auxStartupTimedEnum; // 2
  static const int auxShutdownTimedMask = 1 << _auxShutdownTimedEnum; // 4

  int auxModeByte = 0;
  int auxTimeUnit = 0;
  int auxPulseDuration = 1;
  int auxIntervalCycles = 0;

  bool get auxStartupTimed =>
      (auxModeByte & auxStartupTimedMask) != 0;
  bool get auxShutdownTimed =>
      (auxModeByte & auxShutdownTimedMask) != 0;

  void setAuxStartupTimed(bool on) {
    if (on) {
      auxModeByte |= auxStartupTimedMask;
    } else {
      auxModeByte &= ~auxStartupTimedMask;
    }
  }

  void setAuxShutdownTimed(bool on) {
    if (on) {
      auxModeByte |= auxShutdownTimedMask;
    } else {
      auxModeByte &= ~auxShutdownTimedMask;
    }
  }

  /// Saved preset pressures: presetPressures[profileIndex] = [FP, RP, FD, RD]
  Map<int, List<int>> presetPressures = {};

  /// Firmware update status string returned by the manifold.
  String updateStatus = '';

  /// Last received GETCONFIGVALUES args (100 bytes) for echoing back when saving.
  List<int>? _lastConfigArgs;

  int valveControlValue = 0; // uint32
  int getValveControlValue() {
    return valveControlValue;
  }

  /// ORs [mask] into the valve bitmask and sends a single BLE write.
  void setValveMask(int mask) {
    valveControlValue |= mask;
    writeValveValue(valveControlValue);
  }

  void setValveBit(int bit) {
    setValveMask(1 << bit);
  }

  /// Clears [mask] from the valve bitmask and sends a single BLE write.
  /// Mirrors unsetValveBit() on the Wireless_Controller: releasing one control
  /// must only close that valve, so multi-touch holds stay independent.
  void unsetValveMask(int mask) {
    valveControlValue &= ~mask;
    writeValveValue(valveControlValue);
  }

  void unsetValveBit(int bit) {
    unsetValveMask(1 << bit);
  }

  void closeValves() {
    valveControlValue = 0;
    writeValveValue(valveControlValue);
  }

  /// Valve control is a 4-byte little-endian uint32 (matches Wireless_Controller).
  Future<void> writeValveValue(int value) async {
    if (valveControlCharacteristic != null) {
      try {
        if (valveControlCharacteristic!.properties.write) {
          final bytes = [
            value & 0xFF,
            (value >> 8) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 24) & 0xFF,
          ];
          await valveControlCharacteristic!
              .write(bytes, withoutResponse: false);
          print("Command sent successfully: $value");
        } else {
          print("Write characteristic does not support write operations.");
        }
      } catch (e) {
        print("Error sending command: $e");
      }
    } else {
      print("No write characteristic available.");
    }
  }

  bool isScanning = false;
  StreamSubscription<List<ScanResult>>? _scanSub;
  StreamSubscription<bool>? _isScanningStateSub;

  /// Why the last scan turned up nothing, in words the user can act on.
  /// Null means "no known problem" (either the scan worked, or it genuinely
  /// found nothing). Set after a scan finishes empty, or if the scan threw.
  String? scanDiagnostic;

  /// True when [scanDiagnostic] is something the user fixes in the app's own
  /// permission screen, so the UI can offer a button straight to it.
  bool scanNeedsAppSettings = false;

  /// Raw values behind [scanDiagnostic] (adapter state, each permission status,
  /// location services, how many devices the scan actually saw). Shown on screen
  /// because some head units can't be attached to adb - this is the substitute
  /// for reading logcat.
  String? scanDetails;

  /// Request necessary permissions.
  ///
  /// On Android 12+ (S23 etc) the "Nearby devices" prompt comes from
  /// bluetoothScan/bluetoothConnect and location isn't needed for scanning.
  /// On Android 11 and older those two are no-ops inside permission_handler and
  /// BLE scanning depends entirely on location permission *and* the system
  /// Location toggle - see [_diagnoseEmptyScan].
  Future<void> requestPermissions() async {
    final statuses = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.location,
    ].request();

    statuses.forEach((permission, status) {
      debugPrint('Permission $permission -> $status');
    });

    if (statuses.values.any((status) => status.isDenied)) {
      debugPrint("Required permissions are denied. Scanning may not work.");
    }
  }

  /// Work out why a scan came back with nothing, returning a reason to show the
  /// user (or null if nothing looks wrong). Only called on an empty or failed
  /// scan, so it never interferes with a device that is working.
  ///
  /// Also fills [scanDetails] with the raw values every time, whether or not a
  /// specific fault is identified - on head units that can't be attached to
  /// adb, that on-screen block is the only way to see what Android reported.
  Future<String?> _diagnoseScanProblem() async {
    scanNeedsAppSettings = false;
    try {
      final supported = await FlutterBluePlus.isSupported;
      final adapter = FlutterBluePlus.adapterStateNow;
      final btScan = await Permission.bluetoothScan.status;
      final btConnect = await Permission.bluetoothConnect.status;
      final locationStatus = await Permission.location.status;
      final locationService = await Permission.location.serviceStatus;
      final filtered = !(globalSettings?.showAllBluetoothDevices ?? false);

      scanDetails = [
        'BLE supported: $supported',
        'Adapter: ${adapter.name}',
        'Permission bluetoothScan: ${btScan.name}',
        'Permission bluetoothConnect: ${btConnect.name}',
        'Permission location: ${locationStatus.name}',
        'Location services: ${locationService.name}',
        'Service UUID filter: ${filtered ? "on" : "off"}',
        'Devices seen this scan: ${devicesList.length}',
      ].join('\n');
      debugPrint('Scan diagnostics:\n$scanDetails');

      if (!supported) {
        return 'This device reports no Bluetooth LE support.';
      }

      if (adapter != BluetoothAdapterState.on) {
        return 'Bluetooth is turned off. Turn it on and scan again.';
      }

      // Android 11 and older can only scan for BLE with location permission
      // granted AND location services switched on. Android 12+ scans without
      // either (we declare BLUETOOTH_SCAN with neverForLocation), so a denied
      // location permission there is not a fault.
      if (locationService.isDisabled) {
        return 'Location services are turned off. Android needs Location '
            'switched on to scan for Bluetooth devices - turn it on in system '
            'settings, then scan again.';
      }

      if (locationStatus.isPermanentlyDenied || locationStatus.isRestricted) {
        scanNeedsAppSettings = true;
        return 'Location permission is blocked for OASMan. On Android 11 and '
            'older it is required to scan for Bluetooth devices.';
      }

      if (locationStatus.isDenied) {
        scanNeedsAppSettings = true;
        return 'Location permission was not granted. On Android 11 and older it '
            'is required to scan for Bluetooth devices.';
      }

      return null; // nothing obviously wrong - likely nothing nearby
    } catch (e) {
      debugPrint('Scan diagnostic failed: $e');
      scanDetails = 'Diagnostic failed: $e';
      return null;
    }
  }

  /// Opens the OS permission screen for OASMan, for when a permission is
  /// blocked and the in-app prompt will never appear again.
  Future<void> openPermissionSettings() => openAppSettings();

  /// Native side of the system-settings shortcuts (see MainActivity.kt).
  static const MethodChannel _settingsChannel =
      MethodChannel('dev.oasman.oasman_mobile/settings');

  /// Open the system Bluetooth settings screen. Some head units have no
  /// reachable Bluetooth page in their own settings app, so this is the only
  /// way in. Returns false if no settings screen could be opened at all.
  Future<bool> openBluetoothSettings() =>
      _invokeSettingsChannel('openBluetoothSettings');

  /// Open the system Location settings screen (the master Location toggle,
  /// which Android 11 and older require for BLE scanning).
  Future<bool> openLocationSettings() =>
      _invokeSettingsChannel('openLocationSettings');

  Future<bool> _invokeSettingsChannel(String method) async {
    try {
      return await _settingsChannel.invokeMethod<bool>(method) ?? false;
    } catch (e) {
      debugPrint('$method failed: $e');
      return false;
    }
  }

  /// Ask Android to enable the Bluetooth adapter (shows the system prompt).
  /// Returns false if the request was refused or unavailable.
  Future<bool> turnOnBluetooth() async {
    try {
      await FlutterBluePlus.turnOn();
      return true;
    } catch (e) {
      debugPrint('turnOn failed: $e');
      return false;
    }
  }

  /// Start scanning for BLE devices
  Future<void> startScan() async {
    await requestPermissions();

    if (isScanning) return;

    _scanSub?.cancel();
    _isScanningStateSub?.cancel();
    devicesList.clear();
    scanDiagnostic = null;
    scanDetails = null;
    scanNeedsAppSettings = false;
    isScanning = true;
    notifyListeners();

    _isScanningStateSub = FlutterBluePlus.isScanning.listen((scanning) {
      if (!scanning && isScanning) {
        isScanning = false;
        _scanSub?.cancel();
        _isScanningStateSub?.cancel();
        // A scan that ends with nothing at all is the symptom users actually
        // report ("I press refresh and nothing happens"). Say why.
        if (devicesList.isEmpty && scanDiagnostic == null) {
          _diagnoseScanProblem().then((reason) {
            scanDiagnostic = reason;
            notifyListeners();
          });
        }
        notifyListeners();
        _scheduleReconnectScan();
      }
    });

    _scanSub = FlutterBluePlus.scanResults.listen((results) {
      for (ScanResult result in results) {
        if (!devicesList.contains(result.device)) {
          devicesList.add(result.device);
          notifyListeners();
        }
        if (result.device.remoteId.str == globalSettings!.pairedManifoldId) {
          debugPrint("paired device found");
          FlutterBluePlus.stopScan();
          _connectToDevice(result.device);
          break;
        }
      }
    });

    // Some Android devices never surface the advertised service UUIDs, so a
    // filtered scan finds nothing on them. The "show all bluetooth devices"
    // setting drops the filter and lists everything that advertises.
    final showAll = globalSettings?.showAllBluetoothDevices ?? false;
    try {
      // Must be awaited inside a try: when Android refuses the scan (missing
      // permission, location off, adapter off) flutter_blue_plus throws here.
      // Unawaited, that became an invisible async error and the user just saw
      // an empty list with no explanation.
      await FlutterBluePlus.startScan(
        timeout: const Duration(seconds: 5),
        withServices: showAll ? const [] : [Guid(oasmanServiceUuid)],
      );
      debugPrint(
          "ble scan started, paired ID: ${globalSettings!.pairedManifoldId}");
    } catch (e) {
      debugPrint("startScan failed: $e");
      isScanning = false;
      _scanSub?.cancel();
      _isScanningStateSub?.cancel();
      // Prefer the specific cause; fall back to whatever Android said.
      scanDiagnostic = await _diagnoseScanProblem() ??
          'Bluetooth scan was refused by Android:\n$e';
      notifyListeners();
    }
  }

  /// Stop scanning
  Future<void> stopScan() async {
    await FlutterBluePlus.stopScan();
    _scanSub?.cancel();
    _isScanningStateSub?.cancel();
    isScanning = false;
    notifyListeners();
  }

  /// Connect to a selected device
  Future<void> connectToDevice(
      BluetoothDevice device, BuildContext context) async {
    try {
      _startGlobalConnListener(); // ensure listener is active
      print("Connecting to device: ${device.name} (${device.id})");
      authenticated = false;
      await device.connect(autoConnect: false);

      connectedDevice = device;
      notifyListeners();

      await discoverServices(device, context);

      // With "show all bluetooth devices" on, the list can contain anything -
      // don't remember a device that isn't a manifold, it would poison
      // auto-reconnect.
      if (!_hasManifoldCharacteristics) {
        debugPrint("Not an OASMan manifold: ${device.name} (${device.id})");
        await disconnectDevice();
        if (context.mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('That device is not an OASMan manifold'),
              duration: Duration(seconds: 3),
            ),
          );
        }
        return;
      }

      await _onConnectionCompleted();

      print("Successfully connected to ${device.name} (${device.id})");
      bleBroadcastName = device.name;
      globalSettings!.pairedManifoldId = device.id.toString();
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString('_pairedManifoldId', device.id.toString());
    } catch (e) {
      print("Error connecting to device: $e");
      await disconnectDevice();
    }
  }

  void _connectToDevice(BluetoothDevice device, [BuildContext? context]) async {
    try {
      _startGlobalConnListener(); // ensure listener is active
      print("Connecting to device: ${device.name} (${device.id})");
      authenticated = false;
      await device.connect(autoConnect: false);

      connectedDevice = device;
      notifyListeners();

      await discoverServices(device, context);

      // Same guard as the manual connect path: the saved paired ID could point
      // at something that isn't a manifold any more.
      if (!_hasManifoldCharacteristics) {
        debugPrint("Not an OASMan manifold: ${device.name} (${device.id})");
        await disconnectDevice();
        _scheduleReconnectScan();
        return;
      }

      await _onConnectionCompleted();

      print("Successfully connected to ${device.name} (${device.id})");
      bleBroadcastName = device.name;
      globalSettings?.pairedManifoldId = device.id.toString();
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString('_pairedManifoldId', device.id.toString());
    } catch (e) {
      debugPrint("Error connecting to device: $e");
      await disconnectDevice();
      _scheduleReconnectScan();
    }
  }

  /// Disconnect from the device
  Future<void> disconnectDevice() async {
    _cancelAuthWatchdog();
    authenticated = false;
    if (connectedDevice != null) {
      try {
        await connectedDevice!.disconnect();
        await _globalConnSub?.cancel();
        restStream?.cancel();
        statusStream?.cancel();

        print("Disconnected from device: ${connectedDevice!.name}");
      } catch (e) {
        print("Error disconnecting: $e");
      } finally {
        connectedDevice = null;
        restCharacteristic = null;
        statusCharacteristic = null;
        valveControlCharacteristic = null;
        notifyListeners();
      }
    }
  }

  List<int> buildRestPacket(int cmd, List<Object> data) {
    final ret = List<int>.filled(btoasPacketSize, 0);
    ret[0] = cmd & 0xFF;
    ret[1] = (cmd >> 8) & 0xFF;

    var offset = 4;
    for (final obj in data) {
      if (obj is BLEInt) {
        _writeInt32Le(ret, offset, obj.toInt());
        offset += 4;
      } else if (obj is BLEShort) {
        _writeUint16Le(ret, offset, obj.toShort());
        offset += 2;
      }
    }
    return ret;
  }

  /// Read-only GETCONFIGVALUES request (setValues = 0), full 104-byte packet.
  List<int> buildConfigReadPacket() => kConfigReadPacket;

  static void _writeInt32Le(List<int> buf, int offset, int value) {
    buf[offset] = value & 0xFF;
    buf[offset + 1] = (value >> 8) & 0xFF;
    buf[offset + 2] = (value >> 16) & 0xFF;
    buf[offset + 3] = (value >> 24) & 0xFF;
  }

  static void _writeUint16Le(List<int> buf, int offset, int value) {
    buf[offset] = value & 0xFF;
    buf[offset + 1] = (value >> 8) & 0xFF;
  }

  static void _writeCmdLe(List<int> packet, int cmd) {
    packet[0] = cmd & 0xFF;
    packet[1] = (cmd >> 8) & 0xFF;
  }

  List<int> _buildGetConfigValuesPacket(List<int> args) {
    final packet = List<int>.filled(btoasPacketSize, 0);
    _writeCmdLe(packet, BTOasIdentifier.GETCONFIGVALUES);
    packet.setRange(4, btoasPacketSize, args);
    return packet;
  }

  int _restPacketCmd(List<int> data) {
    if (data.length < 2) return -1;
    return data[0] | (data[1] << 8);
  }

  Future<void> authCheck() async {
    sendRestCommand(buildRestPacket(22 /*AUTHPACKET id*/,
        [BLEInt(passkey), BLEInt(0 /*AuthResult::AUTHRESULT_WAITING*/)]));
  }

  /// Send compressor on/off (CompressorStatusPacket, cmd 24). When connected only.
  void sendCompressorStatus(bool on) {
    sendRestCommand(buildRestPacket(
        BTOasIdentifier.COMPRESSORSTATUS, [BLEInt(on ? 1 : 0)]));
  }

  /// Manual aux output on/off (AuxillaryOutputControlPacket, cmd 38).
  void sendAuxillaryOutputControl(bool on) {
    sendRestCommand(buildRestPacket(
        BTOasIdentifier.AUXILLARYOUTPUTCONTROL, [BLEInt(on ? 1 : 0)]));
  }

  /// Capture the current raw height sensor reading on all 4 wheels as the
  /// per-wheel calibration point selected by [calibrationType]
  /// (a HeightCalibrationType value).
  void sendCalibrateHeightSensors(int calibrationType) {
    sendRestCommand(buildRestPacket(
        BTOasIdentifier.CALIBRATEHEIGHTSENSORS, [BLEInt(calibrationType)]));
  }

  /// RfCommandType / chip / button numbers match [BTOas.h].
  static const int rfCommandChipCmd = 1;
  static const int rfCommandButtonAssign = 2;
  static const int rfCmdDelete = 1;
  static const int rfCmdLearnMomentary = 2;
  static const int rfButtonA = 1;
  static const int rfButtonB = 2;
  static const int rfButtonC = 3;
  static const int rfButtonD = 4;

  static const int bp32EnableNewConn = 0;
  static const int bp32ForgetDevices = 1;
  static const int bp32DisconnectDevices = 2;

  void sendBp32Command(int cmd, {bool value = true}) {
    sendRestCommand(buildRestPacket(BTOasIdentifier.BP32PKT,
        [BLEShort(cmd), BLEShort(value ? 1 : 0)]));
  }

  void sendRfCommand(int commandType, int valueOne, int valueTwo) {
    sendRestCommand(buildRestPacket(BTOasIdentifier.RFCOMMAND, [
      BLEInt(commandType),
      BLEInt(valueOne),
      BLEInt(valueTwo),
    ]));
  }

  /// Assign key fob button to preset [presetOneToFive] (1–5).
  void sendRfButtonPresetAssign(int rfButtonNumber, int presetOneToFive) {
    final p = presetOneToFive.clamp(1, 5);
    sendRfCommand(
        rfCommandButtonAssign, rfButtonNumber, p - 1);
  }

  void sendDetectPressureSensors() {
    sendRestCommand(
        buildRestPacket(BTOasIdentifier.DETECTPRESSURESENSORS, []));
  }

  void sendResetAi() {
    sendRestCommand(buildRestPacket(BTOasIdentifier.RESETAIPKT, []));
  }

  void sendTurnOffManifold() {
    sendRestCommand(buildRestPacket(BTOasIdentifier.TURNOFF, []));
  }

  void sendRebootManifold() {
    sendRestCommand(buildRestPacket(BTOasIdentifier.REBOOT, []));
  }

  /// Unlearn key fob (RF_COMMAND_CHIP_CMD + RF_CMD_DELETE).
  void sendRfUnlearnFob() {
    sendRfCommand(rfCommandChipCmd, rfCmdDelete, 0);
  }

  /// Enter learn mode for momentary key fob.
  void sendRfLearnFobMomentary() {
    sendRfCommand(rfCommandChipCmd, rfCmdLearnMomentary, 0);
  }

  /// OTA / Wi-Fi download (StartwebPacket): SSID in args[0..49], password in args[50..99].
  void sendStartWebUpdate(String ssid, String password) {
    final args = List<int>.filled(100, 0);
    final s = utf8.encode(ssid);
    final p = utf8.encode(password);
    for (var i = 0; i < s.length && i < 49; i++) {
      args[i] = s[i];
    }
    for (var i = 0; i < p.length && i < 49; i++) {
      args[50 + i] = p[i];
    }
    sendRestCommand(
        [..._encodeInt32(BTOasIdentifier.STARTWEB), ...args]);
  }

  /// Mirrors Wireless_Controller's onBLEConnectionCompleted():
  ///   sendConfigValuesPacket(false) + requestPreset() + sendUpdateStatusRequestPacket()
  Future<void> _onConnectionCompleted() async {
    await sendRestCommand(buildConfigReadPacket());
    requestPresetData(2); // default preset 3 → 0-based index 2
    sendUpdateStatusRequest();
  }

  /// Request the manifold's saved pressures for a preset (0-based index).
  void requestPresetData(int presetIndex) {
    sendRestCommand(buildRestPacket(BTOasIdentifier.PRESETREPORT, [
      BLEShort(0),
      BLEShort(0),
      BLEShort(0),
      BLEShort(0),
      BLEShort(presetIndex),
    ]));
  }

  /// Ask the manifold for its current update status string.
  void sendUpdateStatusRequest() {
    sendRestCommandString(
        _encodeInt32(BTOasIdentifier.UPDATESTATUSREQUEST), 'UNKNOWN');
  }

  /// Discover services and characteristics. [context] optional for auth-fail dialog.
  Future<void> discoverServices(
      BluetoothDevice device, BuildContext? context) async {
    try {
      List<BluetoothService> services = await device.discoverServices();
      for (BluetoothService service in services) {
        for (BluetoothCharacteristic characteristic
            in service.characteristics) {
          if (characteristic.uuid.toString().toLowerCase() ==
              "f573f13f-b38e-415e-b8f0-59a6a19a4e02") {
            restCharacteristic = characteristic;
            await characteristic.setNotifyValue(true);
            restStream = characteristic.onValueReceived.listen((value) {
              _handleIncomingRestData(value, context);
            });
            print("doing auth check");
            await authCheck();
            _startAuthWatchdog();
            print("Write characteristic found: ${characteristic.uuid}");
          }

          if (characteristic.uuid.toString().toLowerCase() ==
              "66fda100-8972-4ec7-971c-3fd30b3072ac") {
            statusCharacteristic = characteristic;
            await characteristic.setNotifyValue(true);
            statusStream = characteristic.onValueReceived.listen((value) {
              _handleIncomingData(value);
            });
            print("Notify characteristic found: ${characteristic.uuid}");
          }

          if (characteristic.uuid.toString().toLowerCase() ==
              "e225a15a-e816-4e9d-99b7-c384f91f273b") {
            valveControlCharacteristic = characteristic;
            print("Valve control characteristic found: ${characteristic.uuid}");
          }
        }
      }
      notifyListeners();
      print("Services and characteristics discovered successfully.");
    } catch (e) {
      print('Error discovering services: $e');
    }
  }

  /// Send a command to the connected device
  Future<void> sendCommand(String command) async {
    await sendRestCommand(command.codeUnits);
  }

  /// Send a command to the connected device
  Future<void> sendRestCommand(List<int> command) async {
    if (restCharacteristic != null) {
      try {
        if (restCharacteristic!.properties.write) {
          await restCharacteristic!.write(command, withoutResponse: false);
        } else {
          print("Write characteristic does not support write operations.");
        }
      } catch (e) {
        print("Error sending command: $e");
      }
    } else {
      print("No write characteristic available.");
    }
  }

  Future<void> sendRestCommandString(List<int> address, String text) async {
    if (restCharacteristic != null) {
      try {
        if (restCharacteristic!.properties.write) {
          // Convert the string into a list of bytes
          List<int> command = utf8.encode(text);

          await restCharacteristic!
              .write(address + command, withoutResponse: false);
          print("String command sent successfully: $text");
        } else {
          print("Write characteristic does not support write operations.");
        }
      } catch (e) {
        print("Error sending string command: $e");
      }
    } else {
      print("No write characteristic available.");
    }
  }

  void _handleIncomingRestData(List<int> data, BuildContext? context) {
    try {
      final packetCmd = _restPacketCmd(data);

      switch (packetCmd) {
        case BTOasIdentifier.AUTHPACKET:
          final authResult = data.length >= 12 ? _decodeInt32(data, 8) : -1;
          if (authResult == 1 /*AuthResult::AUTHRESULT_SUCCESS*/) {
            // Manifold accepted the passkey - the link is live, stand the
            // watchdog down.
            authenticated = true;
            _cancelAuthWatchdog();
          } else if (authResult == 2 /*AuthResult::AUTHRESULT_FAIL*/) {
            disconnectDevice();
            if (context != null && context.mounted) {
              showDialog(
                context: context,
                builder: (_) => const InvalidPasskeyPopup(),
              );
            }
          }
          break;
        case BTOasIdentifier.GETCONFIGVALUES:
          if (data.length < btoasPacketSize) {
            debugPrint(
                'GETCONFIGVALUES ignored: expected $btoasPacketSize bytes, got ${data.length}');
            break;
          }
          systemShutoffTimeM = _decodeInt32(data, 4); // args32()[0]
          final configFlagsBits = _decodeInt32(data, 8); // args32()[1]
          pressureSensorMax = _decodeShort(data, 12); // args16()[4]
          bagMaxPressure = data[16];
          compressorOnPSI = data[17];
          compressorOffPSI = data[18];

          riseOnStart =
              (configFlagsBits & (1 << ConfigFlagsBit.CONFIG_RISE_ON_START)) !=
                  0;
          maintainPressure = (configFlagsBits &
                  (1 << ConfigFlagsBit.CONFIG_MAINTAIN_PRESSURE)) !=
              0;
          sensorlessLeveling = (configFlagsBits &
                  (1 << ConfigFlagsBit.CONFIG_SENSORLESS_LEVELING)) !=
              0;
          airOutOnShutoff = (configFlagsBits &
                  (1 << ConfigFlagsBit.CONFIG_AIR_OUT_ON_SHUTOFF)) !=
              0;
          safetyMode =
              (configFlagsBits & (1 << ConfigFlagsBit.CONFIG_SAFETY_MODE)) != 0;
          heightSensorMode = (configFlagsBits &
                  (1 << ConfigFlagsBit.CONFIG_HEIGHT_SENSOR_MODE)) !=
              0;

          configRevision++;

          rfButtonAPreset = data[20] & 0xFF;
          rfButtonBPreset = data[21] & 0xFF;
          rfButtonCPreset = data[22] & 0xFF;
          rfButtonDPreset = data[23] & 0xFF;
          AirUpBagStretchTriggerBelowPressure = data[24] & 0xFF;
          AirUpBagStretchPressure = data[25] & 0xFF;
          compressorCrankOffset = data[26] & 0xFF;

          auxModeByte = data[28] & 0xFF;
          final tu = data[29] & 0xFF;
          auxTimeUnit = tu > 3 ? 0 : tu;
          auxPulseDuration = data[30] & 0xFF;
          auxIntervalCycles = data[31] & 0xFF;

          _lastConfigArgs = List<int>.from(data.sublist(4, btoasPacketSize));

          break;
        case BTOasIdentifier.PRESETREPORT:
          // args16[0..3] = wheel pressures, args16[4] = profile index
          if (data.length >= 14) {
            final fp = _decodeShort(data, 4);
            final rp = _decodeShort(data, 6);
            final fd = _decodeShort(data, 8);
            final rd = _decodeShort(data, 10);
            final profileIndex = _decodeShort(data, 12);
            presetPressures[profileIndex] = [fp, rp, fd, rd];
            debugPrint(
                'PRESETREPORT preset=$profileIndex pressures=[$fp, $rp, $fd, $rd]');
          }
          break;
        case BTOasIdentifier.UPDATESTATUSREQUEST:
          // args contain a null-terminated C string starting at byte 4
          if (data.length > 4) {
            final strBytes = data.sublist(4);
            final nullIdx = strBytes.indexOf(0);
            updateStatus = String.fromCharCodes(
                nullIdx >= 0 ? strBytes.sublist(0, nullIdx) : strBytes);
            debugPrint('UPDATESTATUSREQUEST status=$updateStatus');
          }
          break;
      }

      notifyListeners();
      //} else {
      //print("Received data is too short: $data");
      //}
    } catch (e) {
      print("Error handling incoming data: $e");
    }
  }

  int toUint32(List<int> bytes, [int startIndex = 0]) {
    return (bytes[startIndex] & 0xFF) |
        ((bytes[startIndex + 1] & 0xFF) << 8) |
        ((bytes[startIndex + 2] & 0xFF) << 16) |
        ((bytes[startIndex + 3] & 0xFF) << 24);
  }

  void handleStatusBittset(List<int> statusBytes) {
    if (statusBytes.length != 4) {
      throw ArgumentError('StatusBittset must be exactly 4 bytes long');
    }

    // Convert bytes to Uint32 (little endian to match C++)
    final byteData = ByteData.sublistView(Uint8List.fromList(statusBytes));
    final statusBittset = byteData.getUint32(0, Endian.little);

    // Live status only. Config toggles come from GETCONFIGVALUES.
    compressorFrozen =
        (statusBittset & (1 << StatusPacketBittset.COMPRESSOR_FROZEN)) != 0;
    compressorOn =
        (statusBittset & (1 << StatusPacketBittset.COMPRESSOR_STATUS_ON)) != 0;
    vehicleOn =
        (statusBittset & (1 << StatusPacketBittset.ACC_STATUS_ON)) != 0;
    ebrakeOn =
        (statusBittset & (1 << StatusPacketBittset.EBRAKE_STATUS_ON)) != 0;
    adjustmentInProgress =
        (statusBittset & (1 << StatusPacketBittset.ADJUSTMENT_IN_PROGRESS)) !=
            0;
  }

  void _handleIncomingData(List<int> data) {
    try {
      if (data.length < 4) {
        debugPrint("Received status data is too short: ${data.length} bytes");
        return;
      }
      final packetId = _decodeInt32(data, 0);
      if (packetId != BTOasIdentifier.STATUSREPORT) {
        return;
      }
      if (data.length < 14) return;

      final prevFrozen = compressorFrozen;
      final prevCompOn = compressorOn;
      final prevVeh = vehicleOn;
      final prevEb = ebrakeOn;
      final prevAdjusting = adjustmentInProgress;
      final prevAiLearn = aiLearnPercent;
      final prevFl = pressureValues['frontLeft'];
      final prevFr = pressureValues['frontRight'];
      final prevRl = pressureValues['rearLeft'];
      final prevRr = pressureValues['rearRight'];
      final prevTank = pressureValues['tankPressure'];

      final wheelPressures = [
        _decodeShort(data, 4),
        _decodeShort(data, 6),
        _decodeShort(data, 8),
        _decodeShort(data, 10),
      ];
      final tankPressure = _decodeShort(data, 12);
      if (data.length >= 15) {
        aiLearnPercent = data[14] & 0xFF; // args8()[10]
      }
      if (data.length >= 20) {
        handleStatusBittset(data.sublist(16, 20));
      }

      pressureValues = {
        "frontLeft": wheelPressures[2].toString(),
        "frontRight": wheelPressures[0].toString(),
        "rearLeft": wheelPressures[3].toString(),
        "rearRight": wheelPressures[1].toString(),
        "tankPressure": tankPressure.toString(),
      };

      final changed = prevFrozen != compressorFrozen ||
          prevCompOn != compressorOn ||
          prevVeh != vehicleOn ||
          prevEb != ebrakeOn ||
          prevAdjusting != adjustmentInProgress ||
          prevAiLearn != aiLearnPercent ||
          prevFl != pressureValues['frontLeft'] ||
          prevFr != pressureValues['frontRight'] ||
          prevRl != pressureValues['rearLeft'] ||
          prevRr != pressureValues['rearRight'] ||
          prevTank != pressureValues['tankPressure'];

      if (changed) {
        notifyListeners();
      }
    } catch (e) {
      debugPrint("Error handling incoming data: $e");
    }
  }

  int _decodeShort(List<int> data, int offset) {
    return data[offset] | (data[offset + 1] << 8);
  }

  int _decodeInt32(List<int> data, int offset) {
    return data[offset] |
        (data[offset + 1] << 8) |
        (data[offset + 2] << 16) |
        (data[offset + 3] << 24);
  }

  List<int> _encodeInt32(int value) {
    return [
      value & 0xFF, // byte 0 (LSB)
      (value >> 8) & 0xFF, // byte 1
      (value >> 16) & 0xFF, // byte 2
      (value >> 24) & 0xFF, // byte 3 (MSB)
    ];
  }

  bool isConnected() => connectedDevice != null;

  /// Call after mutating config fields from the UI so [Consumer]s rebuild.
  void refreshFromUi() => notifyListeners();

  int _buildConfigFlagsBits() {
    int bits = 0;
    if (maintainPressure)
      bits |= (1 << ConfigFlagsBit.CONFIG_MAINTAIN_PRESSURE);
    if (sensorlessLeveling)
      bits |= (1 << ConfigFlagsBit.CONFIG_SENSORLESS_LEVELING);
    if (riseOnStart) bits |= (1 << ConfigFlagsBit.CONFIG_RISE_ON_START);
    if (airOutOnShutoff)
      bits |= (1 << ConfigFlagsBit.CONFIG_AIR_OUT_ON_SHUTOFF);
    if (safetyMode) bits |= (1 << ConfigFlagsBit.CONFIG_SAFETY_MODE);
    if (heightSensorMode) {
      bits |= (1 << ConfigFlagsBit.CONFIG_HEIGHT_SENSOR_MODE);
    }
    return bits;
  }

  void saveConfigToManifold() {
    final args = List<int>.filled(btoasArgsSize, 0);
    if (_lastConfigArgs != null && _lastConfigArgs!.length >= btoasArgsSize) {
      args.setAll(0, _lastConfigArgs!);
    }
    _writeInt32Le(args, 0, systemShutoffTimeM);
    _writeInt32Le(args, 4, _buildConfigFlagsBits());
    _writeUint16Le(args, 8, pressureSensorMax);
    args[12] = bagMaxPressure;
    args[13] = compressorOnPSI;
    args[14] = compressorOffPSI;
    args[15] = 1; // setValues = true
    args[16] = rfButtonAPreset.clamp(0, 255);
    args[17] = rfButtonBPreset.clamp(0, 255);
    args[18] = rfButtonCPreset.clamp(0, 255);
    args[19] = rfButtonDPreset.clamp(0, 255);
    args[20] = AirUpBagStretchTriggerBelowPressure.clamp(0, 255);
    args[21] = AirUpBagStretchPressure.clamp(0, 255);
    args[22] = compressorCrankOffset.clamp(0, 255);
    args[24] = auxModeByte.clamp(0, 255);
    args[25] = auxTimeUnit.clamp(0, 3);
    args[26] = auxPulseDuration.clamp(0, 255);
    args[27] = auxIntervalCycles.clamp(0, 255);

    sendRestCommand(_buildGetConfigValuesPacket(args));

    sendRestCommandString(
        _encodeInt32(BTOasIdentifier.BROADCASTNAME), bleBroadcastName);
  }
}
