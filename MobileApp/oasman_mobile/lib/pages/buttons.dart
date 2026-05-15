import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../ble_manager.dart';
import 'popup/nobt.dart';
import 'popup/bluetooth.dart';

class ButtonsPage extends StatefulWidget {
  const ButtonsPage({super.key});

  @override
  _ButtonsPageState createState() => _ButtonsPageState();
}

enum SOLENOID_INDEX {
  FRONT_PASSENGER_IN,
  FRONT_PASSENGER_OUT,
  REAR_PASSENGER_IN,
  REAR_PASSENGER_OUT,
  FRONT_DRIVER_IN,
  FRONT_DRIVER_OUT,
  REAR_DRIVER_IN,
  REAR_DRIVER_OUT
}

class _ButtonsPageState extends State<ButtonsPage> {
  late BLEManager bleManager;
  int _selectedPreset = 3;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    bleManager = Provider.of<BLEManager>(context);
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<BLEManager>(
      builder: (context, bleManager, _) {
        final connected = bleManager.isConnected();

        return Column(
          children: [
            if (!connected)
              Container(
                color: Colors.black87,
                padding:
                    const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                child: Row(
                  children: [
                    const Icon(Icons.bluetooth_disabled,
                        color: Colors.white54, size: 18),
                    const SizedBox(width: 8),
                    const Expanded(
                      child: Text(
                        'Connect a manifold to control',
                        style: TextStyle(color: Colors.white70, fontSize: 13),
                      ),
                    ),
                    TextButton(
                      onPressed: () => showDialog(
                          context: context,
                          builder: (_) => const BluetoothPopup()),
                      child: const Text('Connect',
                          style: TextStyle(color: Color(0xFFBB86FC))),
                    ),
                  ],
                ),
              ),
            Expanded(
              child: Opacity(
                opacity: connected ? 1.0 : 0.4,
                child: IgnorePointer(
                  ignoring: !connected,
                  child: Column(
                    children: [
                      Expanded(
                        child: Center(
                          child: _buildValveGrid(context),
                        ),
                      ),
                      _buildPresetsBar(context, bleManager),
                    ],
                  ),
                ),
              ),
            ),
          ],
        );
      },
    );
  }

  Widget _buildValveGrid(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _valve(context, SOLENOID_INDEX.FRONT_DRIVER_IN.index,
                  SOLENOID_INDEX.FRONT_DRIVER_OUT.index, false),
              _valve2(
                  context,
                  SOLENOID_INDEX.FRONT_DRIVER_IN.index,
                  SOLENOID_INDEX.FRONT_PASSENGER_IN.index,
                  SOLENOID_INDEX.FRONT_DRIVER_OUT.index,
                  SOLENOID_INDEX.FRONT_PASSENGER_OUT.index),
              _valve(context, SOLENOID_INDEX.FRONT_PASSENGER_IN.index,
                  SOLENOID_INDEX.FRONT_PASSENGER_OUT.index, false),
            ],
          ),
          const SizedBox(height: 10),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _valve(context, SOLENOID_INDEX.REAR_DRIVER_IN.index,
                  SOLENOID_INDEX.REAR_DRIVER_OUT.index, false),
              _valve2(
                  context,
                  SOLENOID_INDEX.REAR_DRIVER_IN.index,
                  SOLENOID_INDEX.REAR_PASSENGER_IN.index,
                  SOLENOID_INDEX.REAR_DRIVER_OUT.index,
                  SOLENOID_INDEX.REAR_PASSENGER_OUT.index),
              _valve(context, SOLENOID_INDEX.REAR_PASSENGER_IN.index,
                  SOLENOID_INDEX.REAR_PASSENGER_OUT.index, false),
            ],
          ),
        ],
      ),
    );
  }

  Widget _valve(BuildContext ctx, int inBit, int outBit, bool large) {
    return OvalControlButton(
      iconUp: Icons.keyboard_arrow_up,
      iconDown: Icons.keyboard_arrow_down,
      isLarge: large,
      onUpPressed: () => openValve(ctx, inBit),
      onDownPressed: () => openValve(ctx, outBit),
      onReleasedButton: () => closeValves(ctx),
    );
  }

  Widget _valve2(BuildContext ctx, int in1, int in2, int out1, int out2) {
    return OvalControlButton(
      iconUp: Icons.keyboard_double_arrow_up,
      iconDown: Icons.keyboard_double_arrow_down,
      isLarge: true,
      onUpPressed: () => openValvesMask(ctx, (1 << in1) | (1 << in2)),
      onDownPressed: () => openValvesMask(ctx, (1 << out1) | (1 << out2)),
      onReleasedButton: () => closeValves(ctx),
    );
  }

  Widget _buildPresetsBar(BuildContext context, BLEManager bleManager) {
    final canUsePresetActions =
        bleManager.connectedDevice != null && _selectedPreset >= 1;

    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 4, 16, 12),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              for (int i = 1; i <= 5; i++)
                GestureDetector(
                  onTap: () => _onPresetTapped(context, i),
                  child: CircleAvatar(
                    radius: 18,
                    backgroundColor: i == _selectedPreset
                        ? const Color(0xFFBB86FC)
                        : Colors.grey[800],
                    child: Text(
                      '$i',
                      style: TextStyle(
                        fontSize: 14,
                        color: i == _selectedPreset
                            ? Colors.white
                            : Colors.grey[400],
                      ),
                    ),
                  ),
                ),
            ],
          ),
          const SizedBox(height: 8),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              OutlinedButton(
                onPressed:
                    canUsePresetActions ? () => _saveSelectedPreset() : null,
                style: OutlinedButton.styleFrom(
                  foregroundColor: const Color(0xFFBB86FC),
                  side: const BorderSide(color: Color(0xFFBB86FC)),
                  disabledForegroundColor: Colors.white60,
                  backgroundColor: const Color(0xFF1E1E1E),
                  padding:
                      const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
                ),
                child: const Text('Save'),
              ),
              const SizedBox(width: 16),
              ElevatedButton(
                onPressed: canUsePresetActions
                    ? () => _confirmLoadPreset(context)
                    : null,
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color(0xFFBB86FC),
                  foregroundColor: Colors.white,
                  disabledBackgroundColor: const Color(0xFF2A2A2A),
                  disabledForegroundColor: Colors.white60,
                  padding:
                      const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
                ),
                child: const Text('Load'),
              ),
            ],
          ),
        ],
      ),
    );
  }

  void _onPresetTapped(BuildContext context, int presetNum) {
    if (bleManager.connectedDevice == null) {
      showDialog(
        context: context,
        builder: (_) => const NoBluetoothPopup(),
      );
      return;
    }
    if (_selectedPreset == presetNum) {
      _showPresetDialog(context, presetNum);
      return;
    }

    setState(() => _selectedPreset = presetNum);
    bleManager.requestPresetData(presetNum - 1);
  }

  void _sendLoadPreset(int presetNum) {
    bleManager.sendRestCommand(bleManager
        .buildRestPacket(BTOasIdentifier.AIRUPQUICK, [BLEInt(presetNum - 1)]));
  }

  void _confirmLoadPreset(BuildContext context) {
    if (bleManager.connectedDevice == null || _selectedPreset < 1) return;
    if (_selectedPreset == 1) {
      showDialog<bool>(
        context: context,
        builder: (ctx) => AlertDialog(
          title: const Text('Air out?'),
          content: const Text(
            'Preset 1 is typically air out. Please verify your car is not moving.',
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(ctx).pop(false),
              child: const Text('Cancel'),
            ),
            TextButton(
              onPressed: () => Navigator.of(ctx).pop(true),
              child: const Text('Confirm'),
            ),
          ],
        ),
      ).then((confirmed) {
        if (confirmed == true) _sendLoadPreset(_selectedPreset);
      });
    } else {
      _sendLoadPreset(_selectedPreset);
    }
  }

  void _saveSelectedPreset() {
    if (bleManager.connectedDevice == null || _selectedPreset < 1) return;
    bleManager.sendRestCommand(bleManager.buildRestPacket(
        BTOasIdentifier.SAVECURRENTPRESSURESTOPROFILE,
        [BLEInt(_selectedPreset - 1)]));
    bleManager.sendRestCommand(
        bleManager.buildRestPacket(BTOasIdentifier.GETCONFIGVALUES, []));
    bleManager.requestPresetData(_selectedPreset - 1);
  }

  void _showPresetDialog(BuildContext context, int presetNum) {
    final data = bleManager.presetPressures[presetNum - 1];
    final hasData = data != null && data.length >= 4;

    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text('Preset $presetNum'),
        content: hasData
            ? Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text('Front Passenger: ${data[0]}'),
                  Text('Rear Passenger: ${data[1]}'),
                  Text('Front Driver: ${data[2]}'),
                  Text('Rear Driver: ${data[3]}'),
                ],
              )
            : const Text(
                'No preset data available yet.\nTap another preset and come back after data is received.',
              ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(ctx).pop(),
            child: const Text('Close'),
          ),
        ],
      ),
    );
  }

  void openValve(BuildContext context, int bit) {
    if (bleManager.connectedDevice != null) {
      bleManager.setValveBit(bit);
    } else {
      showDialog(
        context: context,
        builder: (_) => const NoBluetoothPopup(),
      );
    }
  }

  void openValvesMask(BuildContext context, int mask) {
    if (bleManager.connectedDevice != null) {
      bleManager.setValveMask(mask);
    } else {
      showDialog(
        context: context,
        builder: (_) => const NoBluetoothPopup(),
      );
    }
  }

  void closeValves(BuildContext context) {
    if (bleManager.connectedDevice != null) {
      bleManager.closeValves();
    } else {
      showDialog(
        context: context,
        builder: (_) => const NoBluetoothPopup(),
      );
    }
  }
}

class OvalControlButton extends StatelessWidget {
  final IconData iconUp;
  final IconData iconDown;
  final bool isLarge;
  final VoidCallback? onUpPressed;
  final VoidCallback? onDownPressed;
  final VoidCallback? onReleasedButton;

  const OvalControlButton({
    super.key,
    required this.iconUp,
    required this.iconDown,
    this.isLarge = false,
    this.onUpPressed,
    this.onDownPressed,
    this.onReleasedButton,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      width: isLarge ? 56 : 48,
      height: isLarge ? 96 : 72,
      decoration: BoxDecoration(
        color: const Color(0xFF1E1E1E),
        borderRadius: BorderRadius.circular(isLarge ? 28 : 24),
        border: Border.all(
          color: const Color(0xFFBB86FC).withOpacity(0.15),
          width: 1,
        ),
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          ControlButton(
            icon: iconUp,
            onPressed: onUpPressed,
            onReleased: onReleasedButton,
          ),
          ControlButton(
            icon: iconDown,
            onPressed: onDownPressed,
            onReleased: onReleasedButton,
          ),
        ],
      ),
    );
  }
}

class ControlButton extends StatelessWidget {
  final IconData icon;
  final VoidCallback? onPressed;
  final VoidCallback? onReleased;

  const ControlButton({
    super.key,
    required this.icon,
    this.onPressed,
    this.onReleased,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: (_) => onPressed?.call(),
      onTapUp: (_) => onReleased?.call(),
      onTapCancel: () => onReleased?.call(),
      child: SizedBox(
        width: 36,
        height: 32,
        child: Icon(
          icon,
          color: const Color(0xFFBB86FC),
          size: 22,
        ),
      ),
    );
  }
}
