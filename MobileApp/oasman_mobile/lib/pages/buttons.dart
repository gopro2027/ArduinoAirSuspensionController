import 'package:flutter/material.dart';
import 'header.dart';

class ButtonsPage extends StatefulWidget {
  const ButtonsPage({super.key});

  @override
  _ButtonsPageState createState() => _ButtonsPageState();
}

class _ButtonsPageState extends State<ButtonsPage> {
  int _selectedPreset = -1;

  @override
  Widget build(BuildContext context) {
    final size = MediaQuery.of(context).size;

    return Scaffold(
      backgroundColor: Color(0xFF121212),
      body: Column(
        children: [
          // Header Section
          Header(),

          // Control Buttons Section
          Expanded(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    OvalControlButton(
                      iconUp: Icons.keyboard_arrow_up,
                      iconDown: Icons.keyboard_arrow_down,
                      onUpPressed: () => print('Left Front Up Pressed'),
                      onDownPressed: () => print('Left Front Down Pressed'),
                    ),
                    OvalControlButton(
                      iconUp: Icons.keyboard_double_arrow_up,
                      iconDown: Icons.keyboard_double_arrow_down,
                      isLarge: true,
                      onUpPressed: () => print('Front Up Pressed'),
                      onDownPressed: () => print('Front Down Pressed'),
                    ),
                    OvalControlButton(
                      iconUp: Icons.keyboard_arrow_up,
                      iconDown: Icons.keyboard_arrow_down,
                      onUpPressed: () => print('Right Front Up Pressed'),
                      onDownPressed: () => print('Right Front Down Pressed'),
                    ),
                  ],
                ),
                SizedBox(height: 15),
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    OvalControlButton(
                      iconUp: Icons.keyboard_arrow_up,
                      iconDown: Icons.keyboard_arrow_down,
                      onUpPressed: () => print('Left Back Up Pressed'),
                      onDownPressed: () => print('Left Back Down Pressed'),
                    ),
                    OvalControlButton(
                      iconUp: Icons.keyboard_double_arrow_up,
                      iconDown: Icons.keyboard_double_arrow_down,
                      isLarge: true,
                      onUpPressed: () => print('Back Up Pressed'),
                      onDownPressed: () => print('Back Down Pressed'),
                    ),
                    OvalControlButton(
                      iconUp: Icons.keyboard_arrow_up,
                      iconDown: Icons.keyboard_arrow_down,
                      onUpPressed: () => print('Right Back Up Pressed'),
                      onDownPressed: () => print('Right Back Down Pressed'),
                    ),
                  ],
                ),
              ],
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
                      setState(() {
                        _selectedPreset = i;
                      });
                       print('Preset $i Selected');
                    },
                    child: CircleAvatar(
                      backgroundColor: i == _selectedPreset ? Color(0xFFBB86FC) : Colors.grey[800],
                      
                      child: Text(
                        '$i',
                        style: TextStyle(
                          color: i == _selectedPreset ? Colors.white : Colors.grey[400],
                        ),
                      ),
                    ),
                  ),
              ],
            ),
          ),
        ],
      ),
    );
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
        color: Color(0xFF121212),
        borderRadius: BorderRadius.circular(isLarge ? 40 : 30),
        boxShadow: [
          BoxShadow(
            color: Color(0xFFBB86FC).withOpacity(0.1),
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
          color: Color(0xFF121212),
          borderRadius: BorderRadius.circular(isLarge ? 30 : 20),
        ),
        child: Align(
          alignment: alignment,
          child: Icon(
            icon,
            color: Color(0xFFBB86FC),
            size: iconSize ?? (isLarge ? 22 : 20),
          ),
        ),
      ),
    );
  }
}
