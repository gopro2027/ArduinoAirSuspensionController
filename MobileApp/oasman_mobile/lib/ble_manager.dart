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
}

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

  void _startGlobalConnListener() {
    _globalConnSub?.cancel();
    _globalConnSub =
        FlutterBluePlus.events.onConnectionStateChanged.listen((event) {
      // fires for ALL devices
      debugPrint(
          'Global conn event: ${event.device.id} -> ${event.connectionState}');
      if (connectedDevice?.id == event.device.id &&
          event.connectionState == BluetoothConnectionState.disconnected) {
        print('Manifold disconnected!');
        connectedDevice = null;
        vehicleOn = false;
        restCharacteristic = null;
        statusCharacteristic = null;
        notifyListeners();
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
  String bleBroadcastName = '';
  int compressorOnPSI = 0;
  int compressorOffPSI = 0;
  int systemShutoffTimeM = 15;
  int pressureSensorMax = 0;
  int bagVolumePercentage = 0;
  int bagMaxPressure = 0;

  /// Last received GETCONFIGVALUES args (100 bytes) for echoing back when saving.
  List<int>? _lastConfigArgs;

  int valveControlValue = 0; // uint32
  int getValveControlValue() {
    return valveControlValue;
  }

  void setValveBit(int bit) {
    valveControlValue = valveControlValue | (1 << bit);
    writeValveValue(valveControlValue);
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

  /// Request necessary permissions
  Future<void> requestPermissions() async {
    final statuses = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.location,
    ].request();

    if (statuses.values.any((status) => status.isDenied)) {
      print("Required permissions are denied. Scanning may not work.");
    }
  }

  /// Start scanning for BLE devices
  Future<void> startScan() async {
    await requestPermissions();

    if (!isScanning) {
      devicesList.clear();
      isScanning = true;
      notifyListeners();

      FlutterBluePlus.startScan(
        timeout: const Duration(seconds: 5),
        withServices: [Guid(oasmanServiceUuid)],
      );
      print("ble scan started, paired ID: ${globalSettings!.pairedManifoldId}");
      FlutterBluePlus.scanResults.listen((results) {
        for (ScanResult result in results) {
          if (!devicesList.contains(result.device)) {
            devicesList.add(result.device);
            notifyListeners();
          }
          if (result.device.remoteId.str == globalSettings!.pairedManifoldId) {
            print("paired device found");
            FlutterBluePlus.stopScan();
            _connectToDevice(result.device);
            break;
          }
        }
      }).onDone(() {
        isScanning = false;
        notifyListeners();
      });
    }
  }

  /// Stop scanning
  Future<void> stopScan() async {
    await FlutterBluePlus.stopScan();
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
      await sendRestCommand(
          [BTOasIdentifier.GETCONFIGVALUES]); //ask for config from manifold

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
      await device.connect(autoConnect: true);

      connectedDevice = device;
      notifyListeners();

      await discoverServices(device, context);

      await sendRestCommand(
          [BTOasIdentifier.GETCONFIGVALUES]); //ask for config from manifold

      print("Successfully connected to ${device.name} (${device.id})");
      bleBroadcastName = device.name;
      globalSettings?.pairedManifoldId = device.id.toString();
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString('_pairedManifoldId', device.id.toString());
    } catch (e) {
      print("Error connecting to device: $e");
      await disconnectDevice();
      _retryConnection(device);
    }
  }

  void _retryConnection(BluetoothDevice device) async {
    try {
      await device.connect(autoConnect: true);
    } catch (e) {
      debugPrint("Reconnect failed: $e");
      Future.delayed(const Duration(seconds: 3), () {
        _retryConnection(device);
      });
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

// uses our versions of short and int
  Uint8List int32BigEndianBytes(BLEInt value) =>
      Uint8List(4)..buffer.asByteData().setInt32(0, value.toInt(), Endian.big);

  Uint8List int16BigEndianBytes(BLEShort value) => Uint8List(2)
    ..buffer.asByteData().setInt16(0, value.toShort(), Endian.big);

  List<int> buildRestPacket(int cmd, List<Object> data) {
    print("Building rest packet");
    List<int> ret = [];
    ret.add(cmd);
    ret.add(0);
    ret.add(0);
    ret.add(0);

    int carrier = 4;
    for (Object obj in data) {
      if (obj is BLEInt) {
        Uint8List intbytes = int32BigEndianBytes(obj);
        ret.add(intbytes[3]); //(obj >> 24) & 0xFF;
        ret.add(intbytes[2]); //(obj >> 16) & 0xFF;
        ret.add(intbytes[1]); //(obj >> 8) & 0xFF;
        ret.add(intbytes[0]); //obj & 0xFF;
        carrier = carrier + 4;
      }
      if (obj is BLEShort) {
        Uint8List shortbytes = int16BigEndianBytes(obj);
        ret.add(shortbytes[1]);
        ret.add(shortbytes[0]);
        carrier = carrier + 2;
      }
    }
    return ret;
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
          print("Command sent successfully: $command");
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
      //if (data.length >= 16) {
      final packetId = _decodeInt32(data, 0);
      print("Received data: $data");
      print("Rest Packet ID: $packetId");

      switch (packetId) {
        case BTOasIdentifier.AUTHPACKET: //handle incoming status packages
          print("Received auth result");
          //print(_decodeInt32(data, 4).toString());
          //print(_decodeInt32(data, 8).toString());
          if (_decodeInt32(data, 8) == 2 /*AuthResult::AUTHRESULT_FAIL*/) {
            disconnectDevice();
            if (context != null && context.mounted) {
              showDialog(
                context: context,
                builder: (_) => const InvalidPasskeyPopup(),
              );
            }
          }
          break;
        case BTOasIdentifier.GETCONFIGVALUES: // handle incoming config (args32[0], args32[1], args16[4], args16[5], args8[12+0..8])
          systemShutoffTimeM = _decodeInt32(data, 4); // args32()[0]
          final configFlagsBits = _decodeInt32(data, 8); // args32()[1]
          pressureSensorMax = _decodeShort(data, 12); // args16()[4]
          bagVolumePercentage = _decodeShort(data, 14); // args16()[5]
          bagMaxPressure = data.length > 16 ? data[16] : 0; // args8()[12+0]
          compressorOnPSI = data.length > 17 ? data[17] : 0;
          compressorOffPSI = data.length > 18 ? data[18] : 0;
          // setValues at data[19] - not needed for display

          riseOnStart = (configFlagsBits & (1 << ConfigFlagsBit.CONFIG_RISE_ON_START)) != 0;
          maintainPressure = (configFlagsBits & (1 << ConfigFlagsBit.CONFIG_MAINTAIN_PRESSURE)) != 0;
          airOutOnShutoff = (configFlagsBits & (1 << ConfigFlagsBit.CONFIG_AIR_OUT_ON_SHUTOFF)) != 0;
          safetyMode = (configFlagsBits & (1 << ConfigFlagsBit.CONFIG_SAFETY_MODE)) != 0;
          aiStatusEnabled = (configFlagsBits & (1 << ConfigFlagsBit.CONFIG_AI_STATUS_ENABLED)) != 0;

          // Store full args (100 bytes) for echoing back when saving config
          if (data.length >= 104) {
            _lastConfigArgs = List<int>.from(data.sublist(4, 104));
            if (_lastConfigArgs!.length < 100) {
              _lastConfigArgs!.addAll(List.filled(100 - _lastConfigArgs!.length, 0));
            }
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

    print(toUint32(statusBytes));
    //print(byteData);
    print(statusBittset);

    // Live status only (bits 0-5). Config toggles come from GETCONFIGVALUES.
    compressorFrozen = (statusBittset & (1 << 0)) != 0;
    compressorOn = (statusBittset & (1 << 1)) != 0;
    vehicleOn = (statusBittset & (1 << 2)) != 0;
    ebrakeOn = (statusBittset & (1 << 5)) != 0;
  }

  void _handleIncomingData(List<int> data) {
    try {
      if (data.length >= 16) {
        final packetId = _decodeInt32(data, 0);
        switch (packetId) {
          case BTOasIdentifier.STATUSREPORT: //handle incoming status packages
            final wheelPressures = [
              _decodeShort(data, 4),
              _decodeShort(data, 6),
              _decodeShort(data, 8),
              _decodeShort(data, 10),
            ];
            final tankPressure = _decodeShort(data, 12);

            final statusBittset = data.sublist(16, 20);
            print(data);
            print(statusBittset);
            handleStatusBittset(statusBittset);

            pressureValues = {
              "frontLeft": wheelPressures[2].toString(),
              "frontRight": wheelPressures[0].toString(),
              "rearLeft": wheelPressures[3].toString(),
              "rearRight": wheelPressures[1].toString(),
              "tankPressure": tankPressure.toString(),
            };
            break;
        }

        print("Packet ID: $packetId");
        //print("Updated Pressure Values: $pressureValues");
        notifyListeners();
      } else {
        print("Received data is too short: $data");
      }
    } catch (e) {
      print("Error handling incoming data: $e");
    }
  }

  int _decodeShort(List<int> data, int offset) {
    return data[offset] | (data[offset + 1] << 8);
  }

  List<int> _encodeShort(int value) {
    return [
      value & 0xFF, // low byte
      (value >> 8) & 0xFF, // high byte
    ];
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

  int _buildConfigFlagsBits() {
    int bits = 0;
    if (maintainPressure) bits |= (1 << ConfigFlagsBit.CONFIG_MAINTAIN_PRESSURE);
    if (riseOnStart) bits |= (1 << ConfigFlagsBit.CONFIG_RISE_ON_START);
    if (airOutOnShutoff) bits |= (1 << ConfigFlagsBit.CONFIG_AIR_OUT_ON_SHUTOFF);
    if (safetyMode) bits |= (1 << ConfigFlagsBit.CONFIG_SAFETY_MODE);
    if (aiStatusEnabled) bits |= (1 << ConfigFlagsBit.CONFIG_AI_STATUS_ENABLED);
    return bits;
  }

  void saveConfigToManifold() {
    final args = List<int>.filled(100, 0);
    if (_lastConfigArgs != null && _lastConfigArgs!.length >= 100) {
      args.setAll(0, _lastConfigArgs!);
    }
    // Overwrite with current state (ConfigValuesPacket layout)
    args.setAll(0, _encodeInt32(systemShutoffTimeM)); // args32()[0]
    args.setAll(4, _encodeInt32(_buildConfigFlagsBits())); // args32()[1]
    args.setAll(8, _encodeShort(pressureSensorMax)); // args16()[4]
    args.setAll(10, _encodeShort(bagVolumePercentage)); // args16()[5]
    args[12] = bagMaxPressure;
    args[13] = compressorOnPSI;
    args[14] = compressorOffPSI;
    args[15] = 1; // setValues = true

    final packet = [..._encodeInt32(BTOasIdentifier.GETCONFIGVALUES), ...args];
    sendRestCommand(packet);

    sendRestCommandString(
        _encodeInt32(BTOasIdentifier.BROADCASTNAME), bleBroadcastName);
    sendRestCommand([
      ..._encodeInt32(BTOasIdentifier.AUTHPACKET),
      ..._encodeInt32(passkey),
      ..._encodeInt32(3)
    ]);
  }
}
