import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

class BLEManager extends ChangeNotifier {
  final FlutterBluePlus flutterBlue = FlutterBluePlus(); // BLE instans
  BluetoothDevice? connectedDevice; // Den aktuelt forbundne enhed
  BluetoothCharacteristic? writeCharacteristic; // Karakteristik til at skrive data
  BluetoothCharacteristic? notifyCharacteristic; // Karakteristik til notifikationer
  List<BluetoothDevice> devicesList = []; // Liste over fundne enheder

  Map<String, String> pressureValues = {
    "frontLeft": "-",
    "frontRight": "-",
    "rearLeft": "-",
    "rearRight": "-",
    "tankPressure": "-",
  };

 

  bool isScanning = false;

  /// Anmod om nødvendige tilladelser
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

  /// Start scanning efter BLE-enheder
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
            notifyListeners(); // Opdaterer UI
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

  /// Forbind til en valgt enhed
  Future<void> connectToDevice(BluetoothDevice device) async {
    try {
      print("Connecting to device: ${device.name} (${device.id})");
      await device.connect(autoConnect: false);

      connectedDevice = device;
      notifyListeners();

      // Opdag tjenester og karakteristika
      await discoverServices(device);

      print("Successfully connected to ${device.name} (${device.id})");
    } catch (e) {
      print("Error connecting to device: $e");
      await disconnectDevice();
    }
  }

  /// Afbryd forbindelsen til enheden
  Future<void> disconnectDevice() async {
    if (connectedDevice != null) {
      try {
        await connectedDevice!.disconnect();
        print("Disconnected from device: ${connectedDevice!.name}");
      } catch (e) {
        print("Error disconnecting: $e");
      } finally {
        connectedDevice = null;
        writeCharacteristic = null;
        notifyCharacteristic = null;
        notifyListeners();
      }
    }
  }

  /// Opdag tjenester og karakteristika for enheden
Future<void> discoverServices(BluetoothDevice device) async {
  try {
    List<BluetoothService> services = await device.discoverServices();
    for (BluetoothService service in services) {
      for (BluetoothCharacteristic characteristic in service.characteristics) {
        // Check for the specific characteristic UUID
        if (characteristic.uuid.toString() == "f573f13f-b38e-415e-b8f0-59a6a19a4e02") {
          writeCharacteristic = characteristic;
          print("Write characteristic found: ${characteristic.uuid}");
        }

        if (characteristic.properties.notify) {
          notifyCharacteristic = characteristic;
          await characteristic.setNotifyValue(true);
          characteristic.value.listen((value) {
            _handleIncomingData(value);
          });
          print("Notify characteristic found: ${characteristic.uuid}");
        }
      }
    }
    notifyListeners();
    print("Services and characteristics discovered successfully.");
  } catch (e) {
    print("Error discovering services: $e");
  }
}



/// Send en kommando til den forbundne enhed
Future<void> sendCommand(String command) async {
  if (writeCharacteristic != null) {
    try {
      if (writeCharacteristic!.properties.write) {
        await writeCharacteristic!.write(command.codeUnits, withoutResponse: true);
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

  /// Håndter indkommende data fra enheden
void _handleIncomingData(List<int> data) {
  try {
    if (data.length >= 16) {
      // Decode Packet ID (32-bit integer)
      final packetId = _decodeInt32(data, 0);

      // Decode wheel pressures (16-bit short values)
      final wheelPressures = [
        _decodeShort(data, 4),  // WHEEL_FRONT_PASSENGER
        _decodeShort(data, 6),  // WHEEL_REAR_PASSENGER
        _decodeShort(data, 8),  // WHEEL_FRONT_DRIVER
        _decodeShort(data, 10), // WHEEL_REAR_DRIVER
      ];

      // Decode tank pressure (16-bit short value)
      final tankPressure = _decodeShort(data, 12);

      // Update the `pressureValues` map
      pressureValues = {
        "frontLeft": wheelPressures[2].toString(),  // FRONT_DRIVER
        "frontRight": wheelPressures[0].toString(), // FRONT_PASSENGER
        "rearLeft": wheelPressures[3].toString(),   // REAR_DRIVER
        "rearRight": wheelPressures[1].toString(),  // REAR_PASSENGER
        "tankPressure":tankPressure.toString(), // TANK_PRESSURE 
      };

      print("Packet ID: $packetId");
      print("Updated Pressure Values: $pressureValues");
      print("Tank Pressure: $tankPressure");

      // Notify listeners to update the UI
      notifyListeners();
    } else {
      print("Received data is too short: $data");
    }
  } catch (e) {
    print("Error handling incoming data: $e");
  }
}


int _decodeShort(List<int> data, int offset) {
  // Læs 2 bytes som en lille endian kort værdi
  return data[offset] | (data[offset + 1] << 8);
}

int _decodeInt32(List<int> data, int offset) {
  return data[offset] |
      (data[offset + 1] << 8) |
      (data[offset + 2] << 16) |
      (data[offset + 3] << 24);
}



  /// Tjek forbindelsesstatus
  bool isConnected() {
    return connectedDevice != null;
  }

  /// Genstart forbindelse
  Future<void> reconnect() async {
    if (connectedDevice != null) {
      await disconnectDevice();
      await connectToDevice(connectedDevice!);
    }
  }
}