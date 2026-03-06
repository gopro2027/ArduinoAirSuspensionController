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
  int _selectedPreset = -1;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    bleManager = Provider.of<BLEManager>(context);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF121212),
      body: Consumer<BLEManager>(
        builder: (context, bleManager, _) {
          return Stack(
            children: [
              if (!bleManager.isConnected())
                Positioned(
                  top: 0,
                  left: 0,
                  right: 0,
                  child: Material(
                    color: Colors.black87,
                    child: Padding(
                      padding: const EdgeInsets.symmetric(vertical: 12.0),
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          const Icon(Icons.bluetooth_disabled, color: Colors.white54, size: 20),
                          const SizedBox(width: 8),
                          const Text('Connect a manifold to control', style: TextStyle(color: Colors.white70, fontSize: 14)),
                          const SizedBox(width: 12),
                          TextButton(
                            onPressed: () => showDialog(context: context, builder: (_) => const BluetoothPopup()),
                            child: const Text('Connect', style: TextStyle(color: Color(0xFFBB86FC))),
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
              Opacity(
                opacity: bleManager.isConnected() ? 1.0 : 0.5,
                child: IgnorePointer(
                  ignoring: !bleManager.isConnected(),
                  child: LayoutBuilder(
                    builder: (context, constraints) {
                      return SingleChildScrollView(
                child: ConstrainedBox(
                  constraints: BoxConstraints(
                    minHeight: constraints.maxHeight,
                    minWidth: constraints.maxWidth,
                  ),
                  child: IntrinsicHeight(
                    child: Column(
                      children: [
                        // Control Buttons Section
                        Expanded(
                          child: Padding(
                            padding: const EdgeInsets.symmetric(vertical: 16.0),
                            child: Column(
                              mainAxisAlignment: MainAxisAlignment.center,
                              children: [
                                Row(
                              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                              children: [
                                OvalControlButton(
                                    iconUp: Icons.keyboard_arrow_up,
                                    iconDown: Icons.keyboard_arrow_down,
                                    onUpPressed: () => openValve(context,
                                        SOLENOID_INDEX.FRONT_DRIVER_IN.index),
                                    onDownPressed: () => openValve(context,
                                        SOLENOID_INDEX.FRONT_DRIVER_OUT.index),
                                    onReleasedButton: () =>
                                        closeValves(context)),
                                OvalControlButton(
                                  iconUp: Icons.keyboard_double_arrow_up,
                                  iconDown: Icons.keyboard_double_arrow_down,
                                  isLarge: true,
                                  onUpPressed: () => {
                                    openValve(context,
                                        SOLENOID_INDEX.FRONT_DRIVER_IN.index),
                                    openValve(context,
                                        SOLENOID_INDEX.FRONT_PASSENGER_IN.index)
                                  },
                                  onDownPressed: () => {
                                    openValve(context,
                                        SOLENOID_INDEX.FRONT_DRIVER_OUT.index),
                                    openValve(
                                        context,
                                        SOLENOID_INDEX
                                            .FRONT_PASSENGER_OUT.index)
                                  },
                                  onReleasedButton: () => closeValves(context),
                                ),
                                OvalControlButton(
                                  iconUp: Icons.keyboard_arrow_up,
                                  iconDown: Icons.keyboard_arrow_down,
                                  onUpPressed: () => openValve(context,
                                      SOLENOID_INDEX.FRONT_PASSENGER_IN.index),
                                  onDownPressed: () => openValve(context,
                                      SOLENOID_INDEX.FRONT_PASSENGER_OUT.index),
                                  onReleasedButton: () => closeValves(context),
                                ),
                              ],
                            ),
                            const SizedBox(height: 15),
                            Row(
                              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                              children: [
                                OvalControlButton(
                                  iconUp: Icons.keyboard_arrow_up,
                                  iconDown: Icons.keyboard_arrow_down,
                                  onUpPressed: () => openValve(context,
                                      SOLENOID_INDEX.REAR_DRIVER_IN.index),
                                  onDownPressed: () => openValve(context,
                                      SOLENOID_INDEX.REAR_DRIVER_OUT.index),
                                  onReleasedButton: () => closeValves(context),
                                ),
                                OvalControlButton(
                                  iconUp: Icons.keyboard_double_arrow_up,
                                  iconDown: Icons.keyboard_double_arrow_down,
                                  isLarge: true,
                                  onUpPressed: () => {
                                    openValve(context,
                                        SOLENOID_INDEX.REAR_DRIVER_IN.index),
                                    openValve(context,
                                        SOLENOID_INDEX.REAR_PASSENGER_IN.index)
                                  },
                                  onDownPressed: () => {
                                    openValve(context,
                                        SOLENOID_INDEX.REAR_DRIVER_OUT.index),
                                    openValve(context,
                                        SOLENOID_INDEX.REAR_PASSENGER_OUT.index)
                                  },
                                  onReleasedButton: () => closeValves(context),
                                ),
                                OvalControlButton(
                                  iconUp: Icons.keyboard_arrow_up,
                                  iconDown: Icons.keyboard_arrow_down,
                                  onUpPressed: () => openValve(context,
                                      SOLENOID_INDEX.REAR_PASSENGER_IN.index),
                                  onDownPressed: () => openValve(context,
                                      SOLENOID_INDEX.REAR_PASSENGER_OUT.index),
                                  onReleasedButton: () => closeValves(context),
                                ),
                              ],
                            ),
                          ],
                        ),
                      ),
                    ),

                    // Presets Section
                    Padding(
                      padding: const EdgeInsets.symmetric(vertical: 15.0),
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
                                    backgroundColor: i == _selectedPreset
                                        ? const Color(0xFFBB86FC)
                                        : Colors.grey[800],
                                    child: Text(
                                      '$i',
                                      style: TextStyle(
                                        color: i == _selectedPreset
                                            ? Colors.white
                                            : Colors.grey[400],
                                      ),
                                    ),
                                  ),
                                ),
                            ],
                          ),
                          const SizedBox(height: 12),
                          Row(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              OutlinedButton(
                                onPressed: bleManager.connectedDevice != null &&
                                        _selectedPreset >= 1
                                    ? () => _confirmSavePreset(context)
                                    : null,
                                style: OutlinedButton.styleFrom(
                                  foregroundColor: const Color(0xFFBB86FC),
                                  side: const BorderSide(
                                      color: Color(0xFFBB86FC)),
                                ),
                                child: const Text('Save'),
                              ),
                              const SizedBox(width: 16),
                              ElevatedButton(
                                onPressed: bleManager.connectedDevice != null &&
                                        _selectedPreset >= 1
                                    ? () => _confirmLoadPreset(context)
                                    : null,
                                style: ElevatedButton.styleFrom(
                                  backgroundColor: const Color(0xFFBB86FC),
                                ),
                                child: const Text('Load'),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
            ),
          );
        },
                  ),
                ),
              ),
            ],
          );
        },
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
    setState(() => _selectedPreset = presetNum);
    if (presetNum == 1) {
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
        if (confirmed == true && context.mounted) {
          _sendLoadPreset(presetNum);
        }
      });
    } else {
      _sendLoadPreset(presetNum);
    }
  }

  void _sendLoadPreset(int presetNum) {
    bleManager.sendRestCommand(bleManager.buildRestPacket(
        BTOasIdentifier.AIRUPQUICK, [BLEInt(presetNum - 1)]));
    print('Preset $presetNum Load sent');
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

  void _confirmSavePreset(BuildContext context) {
    if (bleManager.connectedDevice == null || _selectedPreset < 1) return;
    showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Save preset'),
        content: Text(
          'Save current height to preset $_selectedPreset?',
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
      if (confirmed == true) {
        bleManager.sendRestCommand(bleManager.buildRestPacket(
            BTOasIdentifier.SAVECURRENTPRESSURESTOPROFILE,
            [BLEInt(_selectedPreset - 1)]));
        bleManager.sendRestCommand([BTOasIdentifier.GETCONFIGVALUES]);
        print('Save preset $_selectedPreset sent');
      }
    });
  }

  void openValve(BuildContext context, int bit) {
    if (bleManager.connectedDevice != null) {
      bleManager.setValveBit(bit);
      print('Valve set');
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
      print('Valve cleared');
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

  const OvalControlButton(
      {super.key,
      required this.iconUp,
      required this.iconDown,
      this.isLarge = false,
      this.onUpPressed,
      this.onDownPressed,
      this.onReleasedButton});

  @override
  Widget build(BuildContext context) {
    return Container(
      width: isLarge ? 60 : 50,
      height: isLarge ? 110 : 80,
      decoration: BoxDecoration(
        color: const Color(0xFF121212),
        borderRadius: BorderRadius.circular(isLarge ? 40 : 30),
        boxShadow: [
          BoxShadow(
            color: const Color(0xFFBB86FC).withOpacity(0.1),
            blurRadius: 15,
            spreadRadius: 2,
          ),
        ],
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          ControlButton(
            icon: iconUp,
            alignment: Alignment.topCenter,
            onPressed: onUpPressed,
            onReleased: onReleasedButton,
          ),
          ControlButton(
            icon: iconDown,
            alignment: Alignment.bottomCenter,
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
  final bool isLarge;
  final double? iconSize;
  final AlignmentGeometry alignment;
  final VoidCallback? onPressed;
  final VoidCallback? onReleased;

  const ControlButton({
    super.key,
    required this.icon,
    this.isLarge = false,
    this.iconSize,
    this.alignment = Alignment.center,
    this.onPressed,
    this.onReleased,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: (_) {
        onPressed!();
      },
      onTapUp: (_) {
        onReleased!();
      },
      child: Container(
        width: isLarge ? 40 : 40,
        height: isLarge ? 40 : 40,
        decoration: BoxDecoration(
          color: const Color(0xFF121212),
          borderRadius: BorderRadius.circular(isLarge ? 30 : 20),
        ),
        child: Align(
          alignment: alignment,
          child: Icon(
            icon,
            color: const Color(0xFFBB86FC),
            size: iconSize ?? (isLarge ? 22 : 20),
          ),
        ),
      ),
    );
  }
}
