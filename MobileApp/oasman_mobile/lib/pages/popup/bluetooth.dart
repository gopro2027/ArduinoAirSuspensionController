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

    return LayoutBuilder(
      builder: (context, constraints) {
        final screenHeight = MediaQuery.of(context).size.height;
        final screenWidth = MediaQuery.of(context).size.width;

        return Dialog(
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
          backgroundColor: Colors.black,
          child: ConstrainedBox(
            constraints: BoxConstraints(
              maxWidth: screenWidth * 0.9,
              maxHeight: screenHeight * 0.8,
            ),
            child: SingleChildScrollView(
              child: IntrinsicHeight(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    _buildHeader(bleManager),
                    const SizedBox(height: 20),
                    Flexible(
                      child: SizedBox(
                        height: screenHeight * 0.5, // Dynamisk højde baseret på skærmstørrelse
                        child: bleManager.connectedDevice == null
                            ? _buildDeviceList(bleManager)
                            : _buildConnectedDevice(bleManager),
                      ),
                    ),
                    const SizedBox(height: 10),
                    if (bleManager.connectedDevice == null)
                      ElevatedButton(
                        onPressed: () async {
                          if (bleManager.isScanning) {
                            await bleManager.stopScan();
                          }
                          await bleManager.startScan();
                          setState(() {});
                        },
                        style: ElevatedButton.styleFrom(
                          backgroundColor: const Color(0xFFBB86FC),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(12),
                          ),
                          padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 24),
                        ),
                        child: const Text(
                          "Refresh",
                          style: TextStyle(color: Colors.white, fontSize: 16),
                        ),
                      ),
                  ],
                ),
              ),
            ),
          ),
        );
      },
    );
  }

  Widget _buildHeader(BLEManager bleManager) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        const Text(
          "Connect to the controller",
          style: TextStyle(
            color: Colors.white,
            fontSize: 18,
            fontWeight: FontWeight.bold,
          ),
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

  Widget _buildDeviceList(BLEManager bleManager) {
    return StreamBuilder<List<ScanResult>>(
      stream: FlutterBluePlus.scanResults,
      initialData: const [],
      builder: (context, snapshot) {
        final scanResults = snapshot.data ?? [];
        final filteredResults = scanResults
            .where((result) => result.device.name.isNotEmpty)
            .toList();

        if (filteredResults.isEmpty) {
          return const Center(
            child: Text(
              "No devices found. Please refresh.",
              style: TextStyle(color: Colors.grey),
            ),
          );
        }

        return ListView.separated(
          shrinkWrap: true,
          physics: const ClampingScrollPhysics(),
          itemCount: filteredResults.length,
          separatorBuilder: (context, index) => const Divider(color: Colors.grey),
          itemBuilder: (context, index) {
            final device = filteredResults[index].device;
            return _buildDeviceTile(device, bleManager);
          },
        );
      },
    );
  }

  Widget _buildConnectedDevice(BLEManager bleManager) {
    return Container(
      margin: const EdgeInsets.symmetric(vertical: 6),
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            bleManager.connectedDevice!.name,
            style: const TextStyle(
              color: Colors.white,
              fontSize: 16,
              fontWeight: FontWeight.bold,
            ),
          ),
          const SizedBox(height: 4),
          const Text(
            "Connected",
            style: TextStyle(
              color: Colors.green,
              fontSize: 14,
              fontWeight: FontWeight.w400,
            ),
          ),
          const SizedBox(height: 10),
          ElevatedButton(
            onPressed: () async {
              await bleManager.disconnectDevice();
            },
            style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFFBB86FC),
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(12),
              ),
              padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 24),
            ),
            child: const Text(
              "Disconnect",
              style: TextStyle(color: Colors.white, fontSize: 14),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildDeviceTile(BluetoothDevice device, BLEManager bleManager) {
    return Container(
      margin: const EdgeInsets.symmetric(vertical: 6),
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Expanded(
            child: Text(
              device.name,
              style: const TextStyle(
                color: Colors.white,
                fontSize: 16,
                fontWeight: FontWeight.bold,
              ),
            ),
          ),
          Container(
            decoration: BoxDecoration(
              color: const Color(0xFFBB86FC),
              shape: BoxShape.circle,
            ),
            child: IconButton(
              icon: const Icon(Icons.keyboard_arrow_right, color: Color.fromARGB(255, 0, 0, 0)),
              onPressed: () async {
                await bleManager.connectToDevice(device);
              },
            ),
          ),
        ],
      ),
    );
  }
}
