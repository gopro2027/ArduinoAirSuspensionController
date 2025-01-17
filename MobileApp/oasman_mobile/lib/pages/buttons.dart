import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'header.dart';
import '../ble_manager.dart';
import 'popup/nobt.dart'; // Import for NoBluetoothPopup

class ButtonsPage extends StatefulWidget {
  const ButtonsPage({super.key});

  @override
  _ButtonsPageState createState() => _ButtonsPageState();
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
      body: LayoutBuilder(
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
                    const Header(),

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
                                  onUpPressed: () => _handleCommand(context, "LEFT_FRONT_UP"),
                                  onDownPressed: () => _handleCommand(context, "LEFT_FRONT_DOWN"),
                                ),
                                OvalControlButton(
                                  iconUp: Icons.keyboard_double_arrow_up,
                                  iconDown: Icons.keyboard_double_arrow_down,
                                  isLarge: true,
                                  onUpPressed: () => _handleCommand(context, "8888"),
                                  onDownPressed: () => _handleCommand(context, "FRONT_DOWN"),
                                ),
                                OvalControlButton(
                                  iconUp: Icons.keyboard_arrow_up,
                                  iconDown: Icons.keyboard_arrow_down,
                                  onUpPressed: () => _handleCommand(context, "RIGHT_FRONT_UP"),
                                  onDownPressed: () => _handleCommand(context, "RIGHT_FRONT_DOWN"),
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
                                  onUpPressed: () => _handleCommand(context, "LEFT_BACK_UP"),
                                  onDownPressed: () => _handleCommand(context, "LEFT_BACK_DOWN"),
                                ),
                                OvalControlButton(
                                  iconUp: Icons.keyboard_double_arrow_up,
                                  iconDown: Icons.keyboard_double_arrow_down,
                                  isLarge: true,
                                  onUpPressed: () => _handleCommand(context, "BACK_UP"),
                                  onDownPressed: () => _handleCommand(context, "BACK_DOWN"),
                                ),
                                OvalControlButton(
                                  iconUp: Icons.keyboard_arrow_up,
                                  iconDown: Icons.keyboard_arrow_down,
                                  onUpPressed: () => _handleCommand(context, "RIGHT_BACK_UP"),
                                  onDownPressed: () => _handleCommand(context, "RIGHT_BACK_DOWN"),
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
                      child: Row(
                        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                        children: [
                          for (int i = 1; i <= 5; i++)
                            GestureDetector(
                              onTap: () {
                                if (bleManager.connectedDevice != null) {
                                  setState(() {
                                    _selectedPreset = i;
                                  });
                                  bleManager.sendCommand("PRESET_$i");
                                  print('Preset $i Command Sent');
                                } else {
                                  showDialog(
                                    context: context,
                                    builder: (_) => const NoBluetoothPopup(),
                                  );
                                }
                              },
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
                    ),
                  ],
                ),
              ),
            ),
          );
        },
      ),
    );
  }

  void _handleCommand(BuildContext context, String command) {
    if (bleManager.connectedDevice != null) {
      bleManager.sendCommand(command);
      print('$command Command Sent');
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

  const OvalControlButton({
    super.key,
    required this.iconUp,
    required this.iconDown,
    this.isLarge = false,
    this.onUpPressed,
    this.onDownPressed,
  });

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
          ),
          ControlButton(
            icon: iconDown,
            alignment: Alignment.bottomCenter,
            onPressed: onDownPressed,
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

  const ControlButton({
    super.key,
    required this.icon,
    this.isLarge = false,
    this.iconSize,
    this.alignment = Alignment.center,
    this.onPressed,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onPressed,
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
