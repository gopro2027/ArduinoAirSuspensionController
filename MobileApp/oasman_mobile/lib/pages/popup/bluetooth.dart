import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:provider/provider.dart';
import '../../ble_manager.dart';

class BluetoothPopup extends StatefulWidget {
  const BluetoothPopup({super.key});

  @override
  State<BluetoothPopup> createState() => _BluetoothPopupState();
}

class _BluetoothPopupState extends State<BluetoothPopup> {
  @override
  Widget build(BuildContext context) {
    final bleManager = Provider.of<BLEManager>(context);

    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      backgroundColor: Colors.black,
      child: Container(
        padding: const EdgeInsets.all(20),
        constraints: BoxConstraints(
          maxHeight: MediaQuery.of(context).size.height * 0.8,
        ),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            _buildHeader(bleManager),
            const SizedBox(height: 20),
            ElevatedButton(
              onPressed: bleManager.isScanning
                  ? null
                  : () {
                      print("Starting scan...");
                      bleManager.startScan();
                    },
              child: const Text("Scan for Devices"),
            ),
            const SizedBox(height: 20),
            Expanded(
              child: _buildDeviceLists(bleManager),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildHeader(BLEManager bleManager) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        Row(
          children: [
            Icon(
              Icons.bluetooth,
              color: bleManager.isConnected() ? Colors.green : Colors.grey,
            ),
            const SizedBox(width: 10),
            const Text(
              "Connect to the controller",
              style: TextStyle(
                color: Colors.white,
                fontSize: 18,
                fontWeight: FontWeight.bold,
              ),
            ),
          ],
        ),
        IconButton(
          icon: const Icon(Icons.close, color: Colors.white),
          onPressed: () {
            if (bleManager.isScanning) bleManager.stopScan();
            Navigator.of(context).pop();
          },
        ),
      ],
    );
  }

  Widget _buildDeviceLists(BLEManager bleManager) {
    return SingleChildScrollView(
      child: Column(
        children: [
          _buildConnectedDevices(bleManager),
          const SizedBox(height: 20),
          _buildScannedDevices(bleManager),
        ],
      ),
    );
  }

  Widget _buildConnectedDevices(BLEManager bleManager) {
    return StreamBuilder<List<BluetoothDevice>>(
      stream: Stream.periodic(const Duration(seconds: 5))
          .asyncMap((_) => FlutterBluePlus.connectedDevices),
      initialData: const [],
      builder: (context, snapshot) {
        final connectedDevices = snapshot.data ?? [];
        if (connectedDevices.isEmpty) {
          return const Text(
            "No connected devices",
            style: TextStyle(color: Colors.grey),
          );
        }
        return Column(
          children: connectedDevices.map((device) {
            return _buildDeviceTile(
              device: device,
              bleManager: bleManager,
              isConnected: true,
            );
          }).toList(),
        );
      },
    );
  }

  Widget _buildScannedDevices(BLEManager bleManager) {
    return StreamBuilder<List<ScanResult>>(
      stream: FlutterBluePlus.scanResults,
      initialData: const [],
      builder: (context, snapshot) {
        final scanResults = snapshot.data ?? [];
        final filteredResults = scanResults
            .where((result) => result.device.name.isNotEmpty)
            .toList();

        if (filteredResults.isEmpty) {
          return const Text(
            "No available devices found. Please scan again.",
            style: TextStyle(color: Colors.grey),
          );
        }

        return ListView.builder(
          shrinkWrap: true,
          itemCount: filteredResults.length,
          itemBuilder: (context, index) {
            return _buildDeviceTile(
              device: filteredResults[index].device,
              bleManager: bleManager,
              isConnected: false,
            );
          },
        );
      },
    );
  }

  Widget _buildDeviceTile({
    required BluetoothDevice device,
    required BLEManager bleManager,
    required bool isConnected,
  }) {
    return Column(
      children: [
        ListTile(
          title: Text(
            device.name.isEmpty ? "Unnamed Device" : device.name,
            style: const TextStyle(color: Color(0xFFEDEDED)),
          ),
          leading: Icon(
            Icons.devices,
            color: const Color(0xFFEDEDED).withOpacity(0.3),
          ),
          trailing: ElevatedButton(
            style: ElevatedButton.styleFrom(
              backgroundColor: isConnected ? Colors.green : Colors.orange,
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(8),
              ),
            ),
            onPressed: isConnected
                ? () async {
                    await bleManager.disconnectDevice();
                  }
                : () async {
                    await bleManager.connectToDevice(device);
                    setState(() {});
                  },
            child: Text(
              isConnected ? "Disconnect" : "Connect",
              style: const TextStyle(color: Color(0xFFEDEDED)),
            ),
          ),
        ),
        const Divider(),
      ],
    );
  }
}
