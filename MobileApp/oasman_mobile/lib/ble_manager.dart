import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:oasman_mobile/pages/popup/invalidkey.dart';
import 'package:permission_handler/permission_handler.dart';
import "dart:typed_data";
import 'models/appSettings.dart';

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

  Future<void> writeValveValue(int value) async {
    if (valveControlCharacteristic != null) {
      try {
        if (valveControlCharacteristic!.properties.write) {
          await valveControlCharacteristic!
              .write([value], withoutResponse: false);
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

      FlutterBluePlus.startScan(timeout: const Duration(seconds: 5));

      FlutterBluePlus.scanResults.listen((results) {
        for (ScanResult result in results) {
          if (!devicesList.contains(result.device)) {
            devicesList.add(result.device);
            notifyListeners();
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
      print("Connecting to device: ${device.name} (${device.id})");
      await device.connect(autoConnect: false);

      connectedDevice = device;
      notifyListeners();

      await discoverServices(device, context);

      print("Successfully connected to ${device.name} (${device.id})");
    } catch (e) {
      print("Error connecting to device: $e");
      await disconnectDevice();
    }
  }

  /// Disconnect from the device
  Future<void> disconnectDevice() async {
    if (connectedDevice != null) {
      try {
        await connectedDevice!.disconnect();
        restStream?.cancel();
        statusStream?.cancel();

        print("Disconnected from device: ${connectedDevice!.name}");
      } catch (e) {
        print("Error disconnecting: $e");
      } finally {
        connectedDevice = null;
        restCharacteristic = null;
        statusCharacteristic = null;
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

  /// Discover services and characteristics
  Future<void> discoverServices(
      BluetoothDevice device, BuildContext context) async {
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

  void _handleIncomingRestData(List<int> data, BuildContext context) {
    try {
      //if (data.length >= 16) {
      final packetId = _decodeInt32(data, 0);
      print("Received data: $data");
      print("Rest Packet ID: $packetId");
      if (packetId == 22) {
        print("Received auth result");
        //print(_decodeInt32(data, 4).toString());
        //print(_decodeInt32(data, 8).toString());
        if (_decodeInt32(data, 8) == 2 /*AuthResult::AUTHRESULT_FAIL*/) {
          disconnectDevice();
          showDialog(
            context: context,
            builder: (_) => const InvalidPasskeyPopup(),
          );
        }
      }

      notifyListeners();
      //} else {
      //print("Received data is too short: $data");
      //}
    } catch (e) {
      print("Error handling incoming data: $e");
    }
  }

void handleStatusBittset(List<int> statusBytes) {
  if (statusBytes.length != 4) {
    throw ArgumentError('StatusBittset must be exactly 4 bytes long');
  }

  // Convert bytes to Uint32 (little endian to match C++)
  final byteData = ByteData.sublistView(Uint8List.fromList(statusBytes));
  final statusBittset = byteData.getUint32(0, Endian.little);

  print("Raw statusBittset: $statusBittset");

  // Decode individual flags
  compressorFrozen    = (statusBittset & (1 << 0)) != 0;
  compressorOn        = (statusBittset & (1 << 1)) != 0;
  final vehicleOn           = (statusBittset & (1 << 2)) != 0;
  final timerExpired        = (statusBittset & (1 << 3)) != 0;
  final clock               = (statusBittset & (1 << 4)) != 0;
  final riseOnStart         = (statusBittset & (1 << 5)) != 0;
  final maintainPressure    = (statusBittset & (1 << 6)) != 0;
  final airOutOnShutoff     = (statusBittset & (1 << 7)) != 0;
  final heightSensorMode    = (statusBittset & (1 << 8)) != 0;
  final safetyMode          = (statusBittset & (1 << 9)) != 0;
  final aiStatusEnabled     = (statusBittset & (1 << 10)) != 0;

  print("""
  Compressor Frozen: $compressorFrozen
  Compressor On: $compressorOn
  Vehicle On: $vehicleOn
  Timer Expired: $timerExpired
  Clock: $clock
  Rise on Start: $riseOnStart
  Maintain Pressure: $maintainPressure
  Air out on Shutoff: $airOutOnShutoff
  Height Sensor Mode: $heightSensorMode
  Safety Mode: $safetyMode
  AI Enabled: $aiStatusEnabled
  """);
}

  void _handleIncomingData(List<int> data) {
    try {
      if (data.length >= 16) {
        final packetId = _decodeInt32(data, 0);
        final wheelPressures = [
          _decodeShort(data, 4),
          _decodeShort(data, 6),
          _decodeShort(data, 8),
          _decodeShort(data, 10),
        ];
        final tankPressure = _decodeShort(data, 12);

        final statusBittset =  data.sublist(13, 17);
        handleStatusBittset(statusBittset);

        pressureValues = {
          "frontLeft": wheelPressures[2].toString(),
          "frontRight": wheelPressures[0].toString(),
          "rearLeft": wheelPressures[3].toString(),
          "rearRight": wheelPressures[1].toString(),
          "tankPressure": tankPressure.toString(),
        };

        //print("Packet ID: $packetId");
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

  int _decodeInt32(List<int> data, int offset) {
    return data[offset] |
        (data[offset + 1] << 8) |
        (data[offset + 2] << 16) |
        (data[offset + 3] << 24);
  }

  bool isConnected() => connectedDevice != null;
}
