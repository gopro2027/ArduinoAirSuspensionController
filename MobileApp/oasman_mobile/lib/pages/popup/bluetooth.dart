import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:provider/provider.dart';
import '../../ble_manager.dart';
import '../../models/appSettings.dart';
import '../../theme/app_theme.dart';

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
          shape:
              RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
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
                        height: screenHeight *
                            0.5, // Dynamisk højde baseret på skærmstørrelse
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
                          backgroundColor: AppTheme.accent(context),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(12),
                          ),
                          padding: const EdgeInsets.symmetric(
                              vertical: 12, horizontal: 24),
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
        final oasmanServiceGuid = Guid(oasmanServiceUuid);
        final showAll = globalSettings?.showAllBluetoothDevices ?? false;
        bool isOasman(ScanResult r) =>
            r.advertisementData.serviceUuids.contains(oasmanServiceGuid);

        final List<ScanResult> filteredResults;
        if (showAll) {
          // Keep known OASMan devices pinned at the top of the full list.
          filteredResults = [
            ...scanResults.where(isOasman),
            ...scanResults.where((r) => !isOasman(r)),
          ];
        } else {
          filteredResults = scanResults.where(isOasman).toList();
        }
        final seen = <String>{};
        final deduped = filteredResults
            .where((r) => seen.add(r.device.remoteId.str))
            .toList();

        if (deduped.isEmpty) {
          return _buildEmptyState(bleManager, showAll);
        }

        return ListView.separated(
          shrinkWrap: true,
          physics: const ClampingScrollPhysics(),
          itemCount: deduped.length,
          separatorBuilder: (context, index) =>
              const Divider(color: Colors.grey),
          itemBuilder: (context, index) {
            final device = deduped[index].device;
            return _buildDeviceTile(device, bleManager, showSubtitleId: showAll);
          },
        );
      },
    );
  }

  /// Shown when a scan turns up nothing. If [BLEManager.scanDiagnostic] worked
  /// out why (permission blocked, location off, adapter off, Android refused
  /// the scan) show that instead of the generic "nothing found" text - on some
  /// head units and older Android builds that reason is the whole story.
  Widget _buildEmptyState(BLEManager bleManager, bool showAll) {
    final diagnostic = bleManager.scanDiagnostic;
    final details = bleManager.scanDetails;

    return Center(
      child: SingleChildScrollView(
        padding: const EdgeInsets.symmetric(horizontal: 16),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            if (diagnostic != null) ...[
              const Icon(Icons.error_outline, color: Colors.orangeAccent),
              const SizedBox(height: 8),
              Text(
                diagnostic,
                style: const TextStyle(color: Colors.orangeAccent),
                textAlign: TextAlign.center,
              ),
            ] else
              Text(
                showAll
                    ? "No devices found. Tap Refresh to scan."
                    : "No OASMan devices found. Tap Refresh to scan.\n\nIf your phone never lists the manifold, turn on \"Show all bluetooth devices\" in Settings.",
                style: const TextStyle(color: Colors.grey),
                textAlign: TextAlign.center,
              ),

            const SizedBox(height: 16),
            _buildFixItButtons(bleManager),

            // What Android actually reported. Shown even when nothing looks
            // wrong, since on a device with no adb this is the only way to see
            // it. "Copy" puts it on the clipboard to paste into a bug report.
            if (details != null) ...[
              const SizedBox(height: 16),
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(10),
                decoration: BoxDecoration(
                  color: Colors.grey[900],
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      details,
                      style: TextStyle(
                        color: Colors.grey[400],
                        fontSize: 12,
                        height: 1.4,
                      ),
                    ),
                    const SizedBox(height: 4),
                    TextButton.icon(
                      onPressed: () {
                        Clipboard.setData(ClipboardData(text: details));
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(
                            content: Text('Diagnostics copied'),
                            duration: Duration(seconds: 2),
                          ),
                        );
                      },
                      icon: const Icon(Icons.copy, size: 16),
                      label: const Text('Copy diagnostics'),
                    ),
                  ],
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }

  /// Shortcuts into the system screens that actually fix a failed scan. These
  /// stay visible whenever a scan comes up empty - not only when a specific
  /// fault was detected - because some head units have no reachable Bluetooth
  /// page in their own settings app, and the scan can fail with the adapter
  /// looking fine from here.
  Widget _buildFixItButtons(BLEManager bleManager) {
    Widget button(String label, IconData icon, VoidCallback onPressed) {
      return OutlinedButton.icon(
        onPressed: onPressed,
        icon: Icon(icon, size: 18),
        label: Text(label),
        style: OutlinedButton.styleFrom(
          foregroundColor: AppTheme.accent(context),
          side: BorderSide(color: AppTheme.accent(context)),
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12),
          ),
        ),
      );
    }

    Future<void> openOrWarn(Future<bool> Function() open, String what) async {
      final opened = await open();
      if (!opened && mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Could not open $what on this device'),
            duration: const Duration(seconds: 3),
          ),
        );
      }
    }

    final adapterOff =
        FlutterBluePlus.adapterStateNow != BluetoothAdapterState.on;

    return Wrap(
      spacing: 8,
      runSpacing: 8,
      alignment: WrapAlignment.center,
      children: [
        // Most direct fix when the adapter is simply off: ask Android to turn
        // it on, then rescan. Falls back to the settings buttons if the ROM
        // refuses the request.
        if (adapterOff)
          button('Turn on Bluetooth', Icons.bluetooth_disabled, () async {
            final on = await bleManager.turnOnBluetooth();
            if (on) {
              await bleManager.startScan();
            } else if (mounted) {
              ScaffoldMessenger.of(context).showSnackBar(
                const SnackBar(
                  content: Text('Could not turn Bluetooth on - try Bluetooth '
                      'settings below'),
                  duration: Duration(seconds: 3),
                ),
              );
            }
            if (mounted) setState(() {});
          }),
        button('Bluetooth settings', Icons.bluetooth, () {
          openOrWarn(bleManager.openBluetoothSettings, 'Bluetooth settings');
        }),
        button('Location settings', Icons.location_on, () {
          openOrWarn(bleManager.openLocationSettings, 'Location settings');
        }),
        button('App permissions', Icons.lock_open, () {
          bleManager.openPermissionSettings();
        }),
      ],
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
            bleManager.connectedDevice!.name.isNotEmpty
                ? bleManager.connectedDevice!.name
                : bleManager.connectedDevice!.remoteId.str,
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
          const SizedBox(height: 6),
          Text(
            "Tap Disconnect to switch to another device.",
            style: TextStyle(
              color: Colors.grey[400],
              fontSize: 12,
            ),
          ),
          const SizedBox(height: 10),
          ElevatedButton(
            onPressed: () async {
              await bleManager.disconnectDevice();
            },
            style: ElevatedButton.styleFrom(
              backgroundColor: AppTheme.accent(context),
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

  Widget _buildDeviceTile(BluetoothDevice device, BLEManager bleManager,
      {bool showSubtitleId = false}) {
    final hasName = device.name.isNotEmpty;
    return Container(
      margin: const EdgeInsets.symmetric(vertical: 6),
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisSize: MainAxisSize.min,
              children: [
                Text(
                  hasName ? device.name : device.remoteId.str,
                  style: const TextStyle(
                    color: Colors.white,
                    fontSize: 16,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                if (showSubtitleId && hasName)
                  Text(
                    device.remoteId.str,
                    style: TextStyle(color: Colors.grey[400], fontSize: 12),
                  ),
              ],
            ),
          ),
          Container(
            decoration: BoxDecoration(
              color: AppTheme.accent(context),
              shape: BoxShape.circle,
            ),
            child: IconButton(
              icon: const Icon(Icons.keyboard_arrow_right,
                  color: Color.fromARGB(255, 0, 0, 0)),
              onPressed: () async {
                await bleManager.connectToDevice(device, context);
              },
            ),
          ),
        ],
      ),
    );
  }
}
