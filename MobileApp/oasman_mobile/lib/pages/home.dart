import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:oasman_mobile/pages/popup/bluetooth.dart';
import 'package:provider/provider.dart';
import '../ble_manager.dart';

class HomePage extends StatelessWidget {
  const HomePage({super.key});

  @override
  Widget build(BuildContext context) {
    final size = MediaQuery.of(context).size;
    final bleManager = Provider.of<BLEManager>(context);

    return Scaffold(
      backgroundColor: const Color(0xFF121212),
      body: SingleChildScrollView(
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 8.0, vertical: 16.0),
          child: Column(
            children: [
              _buildHeader(context, bleManager),
              _buildCarSection(size, bleManager),
              _buildControlButtonsSection(size, bleManager),
              _buildPresetsSection(bleManager),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHeader(BuildContext context, BLEManager bleManager) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceBetween,
      children: [
        IconButton(
          icon: Icon(
            Icons.bluetooth,
            color: bleManager.connectedDevice != null ? Colors.blue : Colors.grey,
          ),
          onPressed: () {
            if (!bleManager.isScanning) {
              bleManager.startScan();
              showDialog(
                context: context,
                builder: (context) => const BluetoothPopup(),
              );
            }
          },
        ),
        Text(
          bleManager.connectedDevice != null
              ? 'Connected to ${bleManager.connectedDevice!.name}'
              : 'No device connected',
          style: const TextStyle(color: Colors.white, fontSize: 14),
        ),
      ],
    );
  }

  Widget _buildCarSection(Size size, BLEManager bleManager) {
    return Container(
      height: size.height * 0.4,
      child: Stack(
        children: [
          Center(
            child: Image.asset(
              'assets/car_black-transformed1.png',
              width: size.width * 0.6,
              height: size.height * 0.4,
            ),
          ),
          Positioned(
            top: size.height * 0.04,
            left: size.width * 0.05,
            child: _buildPressureIndicator(
              label: "Front Left",
              barValue: bleManager.pressureValues["frontLeft"] ?? "-",
              percentValue: bleManager.percentValues["frontLeft"] ?? "-",
            ),
          ),
          Positioned(
            top: size.height * 0.04,
            right: size.width * 0.05,
            child: _buildPressureIndicator(
              label: "Front Right",
              barValue: bleManager.pressureValues["frontRight"] ?? "-",
              percentValue: bleManager.percentValues["frontRight"] ?? "-",
              flip: true,
            ),
          ),
          Positioned(
            bottom: size.height * 0.09,
            left: size.width * 0.05,
            child: _buildPressureIndicator(
              label: "Rear Left",
              barValue: bleManager.pressureValues["rearLeft"] ?? "-",
              percentValue: bleManager.percentValues["rearLeft"] ?? "-",
            ),
          ),
          Positioned(
            bottom: size.height * 0.09,
            right: size.width * 0.05,
            child: _buildPressureIndicator(
              label: "Rear Right",
              barValue: bleManager.pressureValues["rearRight"] ?? "-",
              percentValue: bleManager.percentValues["rearRight"] ?? "-",
              flip: true,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildControlButtonsSection(Size size, BLEManager bleManager) {
    return Container(
      height: size.height * 0.4,
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const SizedBox(height: 16),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              OvalControlButton(
                iconUp: Icons.keyboard_arrow_up,
                iconDown: Icons.keyboard_arrow_down,
                onUpPressed: () => bleManager.sendCommand("8888"),
                onDownPressed: () => bleManager.sendCommand("2"),
              ),
              OvalControlButton(
                iconUp: Icons.keyboard_double_arrow_up,
                iconDown: Icons.keyboard_double_arrow_down,
                isLarge: true,
                onUpPressed: () => bleManager.sendCommand("8888"),
                onDownPressed: () => bleManager.sendCommand("2"),
              ),
              OvalControlButton(
                iconUp: Icons.keyboard_arrow_up,
                iconDown: Icons.keyboard_arrow_down,
                onUpPressed: () => bleManager.sendCommand("1"),
                onDownPressed: () => bleManager.sendCommand("2"),
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
                onUpPressed: () => bleManager.sendCommand("leftRearUp"),
                onDownPressed: () => bleManager.sendCommand("leftRearDown"),
              ),
              OvalControlButton(
                iconUp: Icons.keyboard_double_arrow_up,
                iconDown: Icons.keyboard_double_arrow_down,
                isLarge: true,
                onUpPressed: () => bleManager.sendCommand("rearUp"),
                onDownPressed: () => bleManager.sendCommand("rearDown"),
              ),
              OvalControlButton(
                iconUp: Icons.keyboard_arrow_up,
                iconDown: Icons.keyboard_arrow_down,
                onUpPressed: () => bleManager.sendCommand("rightRearUp"),
                onDownPressed: () => bleManager.sendCommand("rightRearDown"),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildPresetsSection(BLEManager bleManager) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 16.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          for (int i = 1; i <= 5; i++)
            GestureDetector(
              onTap: () => bleManager.sendCommand("preset$i"),
              child: CircleAvatar(
                backgroundColor: i == 3 ? Colors.purple : Colors.grey[800],
                child: Text(
                  '$i',
                  style: TextStyle(
                    color: i == 3 ? Colors.white : Colors.grey[400],
                  ),
                ),
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildPressureIndicator({
    required String label,
    required String barValue,
    required String percentValue,
    bool flip = false,
  }) {
    return Column(
      crossAxisAlignment: flip ? CrossAxisAlignment.end : CrossAxisAlignment.start,
      children: [
        Text(
          '$barValue Bar',
          style: const TextStyle(
            color: Colors.white,
            fontSize: 14,
          ),
        ),
        Transform(
          transform: Matrix4.identity()..scale(flip ? -1.0 : 1.0, 1.0),
          alignment: Alignment.center,
          child: SvgPicture.asset(
            'assets/Group1.svg',
            width: 20,
            height: 20,
          ),
        ),
        const SizedBox(height: 4),
        Text(
          '$percentValue %',
          style: const TextStyle(
            color: Colors.white,
            fontSize: 14,
          ),
        ),
      ],
    );
  }
}


  Widget _buildPressureIndicator({
    required String label,
    required String barValue,
    required String percentValue,
    bool flip = false,
  }) {
    return Column(
      crossAxisAlignment: flip ? CrossAxisAlignment.end : CrossAxisAlignment.start,
      children: [
        Text(
          '$barValue Bar',
          style: const TextStyle(
            color: Colors.white,
            fontSize: 14,
          ),
        ),
        Transform(
          transform: Matrix4.identity()..scale(flip ? -1.0 : 1.0, 1.0),
          alignment: Alignment.center,
          child: SvgPicture.asset(
            'assets/Group1.svg',
            width: 20,
            height: 20,
          ),
        ),
        const SizedBox(height: 4),
        Text(
          '$percentValue %',
          style: const TextStyle(
            color: Colors.white,
            fontSize: 14,
          ),
        ),
      ],
    );
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
      width: isLarge ? 50 : 50,
      height: isLarge ? 110 : 80,
      decoration: BoxDecoration(
        color: Colors.black,
        borderRadius: BorderRadius.circular(isLarge ? 40 : 30),
        boxShadow: [
          BoxShadow(
            color: Colors.black87.withOpacity(0.7),
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
          color: Colors.black,
          borderRadius: BorderRadius.circular(isLarge ? 30 : 20),
          boxShadow: [
            BoxShadow(
              color: Colors.black87.withOpacity(0.7),
              blurRadius: 15,
              spreadRadius: 2,
            ),
          ],
        ),
        child: Align(
          alignment: alignment,
          child: Icon(
            icon,
            color: Colors.purple,
            size: iconSize ?? (isLarge ? 22 : 20),
          ),
        ),
      ),
    );
  }
}