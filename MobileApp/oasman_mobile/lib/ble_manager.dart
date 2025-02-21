import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

class BLEManager extends ChangeNotifier {
  final FlutterBluePlus flutterBlue = FlutterBluePlus(); // BLE instance
  BluetoothDevice? connectedDevice; // Currently connected device
  BluetoothCharacteristic? writeCharacteristic; // Characteristic for writing data
  BluetoothCharacteristic? notifyCharacteristic; // Characteristic for notifications
  List<BluetoothDevice> devicesList = []; // List of discovered devices

  Map<String, String> pressureValues = {
    "frontLeft": "-",
    "frontRight": "-",
    "rearLeft": "-",
    "rearRight": "-",
    "tankPressure": "-",
  };

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
  Future<void> connectToDevice(BluetoothDevice device) async {
    try {
      print("Connecting to device: \${device.name} (\${device.id})");
      await device.connect(autoConnect: false);

      connectedDevice = device;
      notifyListeners();

      await discoverServices(device);

      print("Successfully connected to \${device.name} (\${device.id})");
    } catch (e) {
      print("Error connecting to device: \$e");
      await disconnectDevice();
    }
  }

  /// Disconnect from the device
  Future<void> disconnectDevice() async {
    if (connectedDevice != null) {
      try {
        await connectedDevice!.disconnect();
        print("Disconnected from device: \${connectedDevice!.name}");
      } catch (e) {
        print("Error disconnecting: \$e");
      } finally {
        connectedDevice = null;
        writeCharacteristic = null;
        notifyCharacteristic = null;
        notifyListeners();
      }
    }
  }

  /// Discover services and characteristics
  Future<void> discoverServices(BluetoothDevice device) async {
    try {
      List<BluetoothService> services = await device.discoverServices();
      for (BluetoothService service in services) {
        for (BluetoothCharacteristic characteristic in service.characteristics) {
          if (characteristic.uuid.toString().toLowerCase() == "f573f13f-b38e-415e-b8f0-59a6a19a4e02") {
            writeCharacteristic = characteristic;
            print("Write characteristic found: \${characteristic.uuid}");
          }

          if (characteristic.properties.notify) {
            notifyCharacteristic = characteristic;
            await characteristic.setNotifyValue(true);
            characteristic.value.listen((value) {
              _handleIncomingData(value);
            });
            print("Notify characteristic found: \${characteristic.uuid}");
          }
        }
      }
      notifyListeners();
      print("Services and characteristics discovered successfully.");
    } catch (e) {
      print("Error discovering services: \$e");
    }
  }

  /// Send a command to the connected device
  Future<void> sendCommand(String command) async {
    if (writeCharacteristic != null) {
      try {
        if (writeCharacteristic!.properties.write) {
          await writeCharacteristic!.write(command.codeUnits, withoutResponse: false);
          print("Command sent successfully: \$command");
        } else {
          print("Write characteristic does not support write operations.");
        }
      } catch (e) {
        print("Error sending command: \$e");
      }
    } else {
      print("No write characteristic available.");
    }
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

        pressureValues = {
          "frontLeft": wheelPressures[2].toString(),
          "frontRight": wheelPressures[0].toString(),
          "rearLeft": wheelPressures[3].toString(),
          "rearRight": wheelPressures[1].toString(),
          "tankPressure": tankPressure.toString(),
        };

        print("Packet ID: \$packetId");
        print("Updated Pressure Values: \$pressureValues");
        notifyListeners();
      } else {
        print("Received data is too short: \$data");
      }
    } catch (e) {
      print("Error handling incoming data: \$e");
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
