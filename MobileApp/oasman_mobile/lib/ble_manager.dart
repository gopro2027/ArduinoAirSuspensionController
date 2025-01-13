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
  };

  Map<String, String> percentValues = {
    "frontLeft": "-",
    "frontRight": "-",
    "rearLeft": "-",
    "rearRight": "-",
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
          if (characteristic.properties.write) {
            writeCharacteristic = characteristic;
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
        await writeCharacteristic!.write(command.codeUnits, withoutResponse: true);
        print("Command sent: $command");
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
      final decodedData = String.fromCharCodes(data);
      print("Received data: $decodedData");

      final parts = decodedData.split(',');
      if (parts.length == 4) {
        pressureValues["frontLeft"] = parts[0];
        pressureValues["frontRight"] = parts[1];
        pressureValues["rearLeft"] = parts[2];
        pressureValues["rearRight"] = parts[3];
        notifyListeners();
      }
    } catch (e) {
      print("Error handling incoming data: $e");
    }
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