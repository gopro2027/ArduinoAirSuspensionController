import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:provider/provider.dart';
import 'popup/bluetooth.dart';
import '../ble_manager.dart';

class Header extends StatelessWidget {
  const Header({super.key});

  @override
  Widget build(BuildContext context) {
    final size = MediaQuery.of(context).size;

    return Consumer<BLEManager>(
      builder: (context, bleManager, child) {
        return Stack(
          children: [
            Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                // Car Section wrapped in a container for easy positioning
                Container(
                  margin: const EdgeInsets.only(top: 30),
                  child: Container(
                    height: size.height * 0.35,
                    child: Stack(
                      children: [
                        // Car Image
                        Center(
                          child: Image.asset(
                            'assets/car_black-transformed1.png',
                            width: size.width * 0.6,
                            height: size.height * 0.3,
                          ),
                        ),

                        // Pressure Info Widgets
                        _buildPositionedInfo(
                          top: size.height * 0.04,
                          left: size.width * 0.1,
                          pressure: "${bleManager.pressureValues["frontLeft"]} Bar",
                          percentage: "- %",
                          asset: 'assets/Group2.svg',
                        ),
                        _buildPositionedInfo(
                          top: size.height * 0.04,
                          right: size.width * 0.1,
                          pressure: "${bleManager.pressureValues["frontRight"]} Bar",
                          percentage: "- %",
                          asset: 'assets/Group2.svg',
                          alignRight: true,
                          flipSvg: true,
                        ),
                        _buildPositionedInfo(
                          bottom: size.height * 0.07,
                          left: size.width * 0.1,
                          pressure: "${bleManager.pressureValues["rearLeft"]} Bar",
                          percentage: "- %",
                          asset: 'assets/Group1.svg',
                          flipSvg: true,
                        ),
                        _buildPositionedInfo(
                          bottom: size.height * 0.07,
                          right: size.width * 0.1,
                          pressure: "${bleManager.pressureValues["rearRight"]} Bar",
                          percentage: "- %",
                          asset: 'assets/Group1.svg',
                          alignRight: true,
                        ),

                        // Centered Tank Pressure (under bilen)
                        Positioned(
                          bottom: size.height * 0.02,
                          left: size.width / 2 - 22, // Center horisontalt
                          child: Text(
                            "${bleManager.pressureValues["tankPressure"]} Bar",
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: size.width * 0.035, // Responsiv størrelse
                            ),
                            textAlign: TextAlign.center,
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ],
            ),

            // Bluetooth Icon
            Positioned(
              top: size.height * 0.05,
              left: size.width * 0.03,
              child: GestureDetector(
                onTap: () {
                  // Show popup
                  showDialog(
                    context: context,
                    builder: (_) => const BluetoothPopup(),
                  );
                },
                child: Icon(
                  Icons.bluetooth,
                  color: bleManager.connectedDevice != null ? Colors.green : Colors.pink, // Grøn, hvis forbundet
                  size: size.width * 0.07, // Responsiv størrelse
                ),
              ),
            ),
          ],
        );
      },
    );
  }

  // Helper method for positioned info
  Widget _buildPositionedInfo({
    double? top,
    double? bottom,
    double? left,
    double? right,
    required String pressure,
    required String percentage,
    required String asset,
    bool alignRight = false,
    bool flipSvg = false,
  }) {
    return Positioned(
      top: top,
      bottom: bottom,
      left: left,
      right: right,
      child: _buildPressureInfo(
        pressure,
        percentage,
        asset,
        alignRight: alignRight,
        flipSvg: flipSvg,
      ),
    );
  }

  // Helper method for pressure and percentage display
  Widget _buildPressureInfo(
    String pressure,
    String percentage,
    String asset, {
    bool alignRight = false,
    bool flipSvg = false,
  }) {
    return Column(
      crossAxisAlignment:
          alignRight ? CrossAxisAlignment.end : CrossAxisAlignment.start,
      children: [
        // Text before SVG
        Text(
          pressure,
          style: const TextStyle(
            color: Colors.white,
            fontSize: 14,
          ),
        ),
        const SizedBox(height: 10),

        // SVG (with optional flipping)
        Transform(
          transform: flipSvg
              ? (Matrix4.identity()..scale(-1.0, 1.0)) // Flip horizontally
              : Matrix4.identity(),
          alignment: Alignment.center,
          child: SvgPicture.asset(
            asset,
            width: 20,
            height: 20,
            placeholderBuilder: (BuildContext context) =>
                const CircularProgressIndicator(), // Fallback if SVG fails to load
          ),
        ),
        const SizedBox(height: 4),

        // Text after SVG
        Text(
          percentage,
          style: const TextStyle(
            color: Colors.white,
            fontSize: 14,
          ),
        ),
      ],
    );
  }
}
