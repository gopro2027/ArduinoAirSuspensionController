import 'dart:async';
import 'dart:convert'; // for utf8.encode

import 'package:shared_preferences/shared_preferences.dart';
import 'package:flutter/material.dart';
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
}

/// GETCONFIGVALUES read request (cmd only, args zeroed). Reused on every connect.
final List<int> kConfigReadPacket = List<int>.filled(btoasPacketSize, 0)
  ..[0] = BTOasIdentifier.GETCONFIGVALUES & 0xFF
  ..[1] = BTOasIdentifier.GETCONFIGVALUES >> 8;

/// Config flags in ConfigValuesPacket.configFlagsBits (GETCONFIGVALUES).
class ConfigFlagsBit {
  static const int CONFIG_MAINTAIN_PRESSURE = 0;
  static const int CONFIG_RISE_ON_START = 1;
  static const int CONFIG_AIR_OUT_ON_SHUTOFF = 2;
  static const int CONFIG_HEIGHT_SENSOR_MODE = 3;
  static const int CONFIG_SAFETY_MODE = 4;
  static const int CONFIG_AI_STATUS_ENABLED = 5;
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
        connectedDevice = null;
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
  bool riseOnStart = false;
  bool maintainPressure = false;
  bool airOutOnShutoff = false;
  bool safetyMode = true;
  bool aiStatusEnabled = false;
  /// Mirrors ConfigFlagsBit::CONFIG_HEIGHT_SENSOR_MODE (preserved on save).
  bool heightSensorMode = false;
  String bleBroadcastName = '';

  /// Incremented when a GETCONFIGVALUES packet updates local config fields.
  int configRevision = 0;
  int compressorOnPSI = 0;
  int compressorOffPSI = 0;
  int systemShutoffTimeM = 15;
  int pressureSensorMax = 0;
  int bagVolumePercentage = 0;
  int bagMaxPressure = 0;

  /// From STATUSREPORT args (AI learning UI).
  int aiLearnPercent = 0;
  int aiReadyBittset = 0;

  /// Height sensor invert flags (ConfigValuesPacket byte 20 / args offset 16+8).
  int heightSensorInvertBits = 0;

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

  /// Request necessary permissions
  Future<void> requestPermissions() async {
    final statuses = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.location,
    ].request();

    if (statuses.values.any((status) => status.isDenied)) {
      debugPrint("Required permissions are denied. Scanning may not work.");
    }
  }

  /// Start scanning for BLE devices
  Future<void> startScan() async {
    await requestPermissions();

    if (isScanning) return;

    _scanSub?.cancel();
    _isScanningStateSub?.cancel();
    devicesList.clear();
    isScanning = true;
    notifyListeners();

    _isScanningStateSub = FlutterBluePlus.isScanning.listen((scanning) {
      if (!scanning && isScanning) {
        isScanning = false;
        _scanSub?.cancel();
        _isScanningStateSub?.cancel();
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

    FlutterBluePlus.startScan(
      timeout: const Duration(seconds: 5),
      withServices: [Guid(oasmanServiceUuid)],
    );
    debugPrint(
        "ble scan started, paired ID: ${globalSettings!.pairedManifoldId}");
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
      await device.connect(autoConnect: false);

      connectedDevice = device;
      notifyListeners();

      await discoverServices(device, context);
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
      await device.connect(autoConnect: false);

      connectedDevice = device;
      notifyListeners();

      await discoverServices(device, context);
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

  /// Toggle one wheel bit in [heightSensorInvertBits] (0..3 = FP, RP, FD, RD order matches wireless labels).
  void setHeightInvertWheel(int wheelBitIndex, bool inverted) {
    if (wheelBitIndex < 0 || wheelBitIndex > 3) return;
    if (inverted) {
      heightSensorInvertBits |= (1 << wheelBitIndex);
    } else {
      heightSensorInvertBits &= ~(1 << wheelBitIndex);
    }
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
          if (data.length >= 12 &&
              _decodeInt32(data, 8) == 2 /*AuthResult::AUTHRESULT_FAIL*/) {
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
          bagVolumePercentage = _decodeShort(data, 14); // args16()[5]
          bagMaxPressure = data[16];
          compressorOnPSI = data[17];
          compressorOffPSI = data[18];

          riseOnStart =
              (configFlagsBits & (1 << ConfigFlagsBit.CONFIG_RISE_ON_START)) !=
                  0;
          maintainPressure = (configFlagsBits &
                  (1 << ConfigFlagsBit.CONFIG_MAINTAIN_PRESSURE)) !=
              0;
          airOutOnShutoff = (configFlagsBits &
                  (1 << ConfigFlagsBit.CONFIG_AIR_OUT_ON_SHUTOFF)) !=
              0;
          safetyMode =
              (configFlagsBits & (1 << ConfigFlagsBit.CONFIG_SAFETY_MODE)) != 0;
          aiStatusEnabled = (configFlagsBits &
                  (1 << ConfigFlagsBit.CONFIG_AI_STATUS_ENABLED)) !=
              0;
          heightSensorMode = (configFlagsBits &
                  (1 << ConfigFlagsBit.CONFIG_HEIGHT_SENSOR_MODE)) !=
              0;

          configRevision++;

          rfButtonAPreset = data[20] & 0xFF;
          rfButtonBPreset = data[21] & 0xFF;
          rfButtonCPreset = data[22] & 0xFF;
          rfButtonDPreset = data[23] & 0xFF;
          heightSensorInvertBits = data[24] & 0xFF;

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

    // Live status only (bits 0-5). Config toggles come from GETCONFIGVALUES.
    compressorFrozen = (statusBittset & (1 << 0)) != 0;
    compressorOn = (statusBittset & (1 << 1)) != 0;
    vehicleOn = (statusBittset & (1 << 2)) != 0;
    ebrakeOn = (statusBittset & (1 << 5)) != 0;
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
      final prevAiLearn = aiLearnPercent;
      final prevAiReady = aiReadyBittset;
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
      if (data.length >= 16) {
        aiLearnPercent = data[14] & 0xFF;
        aiReadyBittset = data[15] & 0xFF;
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
          prevAiLearn != aiLearnPercent ||
          prevAiReady != aiReadyBittset ||
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
    if (riseOnStart) bits |= (1 << ConfigFlagsBit.CONFIG_RISE_ON_START);
    if (airOutOnShutoff)
      bits |= (1 << ConfigFlagsBit.CONFIG_AIR_OUT_ON_SHUTOFF);
    if (safetyMode) bits |= (1 << ConfigFlagsBit.CONFIG_SAFETY_MODE);
    if (aiStatusEnabled) bits |= (1 << ConfigFlagsBit.CONFIG_AI_STATUS_ENABLED);
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
    _writeUint16Le(args, 10, bagVolumePercentage);
    args[12] = bagMaxPressure;
    args[13] = compressorOnPSI;
    args[14] = compressorOffPSI;
    args[15] = 1; // setValues = true
    args[16] = rfButtonAPreset.clamp(0, 255);
    args[17] = rfButtonBPreset.clamp(0, 255);
    args[18] = rfButtonCPreset.clamp(0, 255);
    args[19] = rfButtonDPreset.clamp(0, 255);
    args[20] = heightSensorInvertBits.clamp(0, 255);
    args[24] = auxModeByte.clamp(0, 255);
    args[25] = auxTimeUnit.clamp(0, 3);
    args[26] = auxPulseDuration.clamp(0, 255);
    args[27] = auxIntervalCycles.clamp(0, 255);

    sendRestCommand(_buildGetConfigValuesPacket(args));

    sendRestCommandString(
        _encodeInt32(BTOasIdentifier.BROADCASTNAME), bleBroadcastName);
  }
}
