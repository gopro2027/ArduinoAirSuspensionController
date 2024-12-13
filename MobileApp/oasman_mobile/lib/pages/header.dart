import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'popup/bluetooth.dart'; // Import Bluetooth popup file

class Header extends StatelessWidget {
  const Header({super.key});

  @override
  Widget build(BuildContext context) {
    final size = MediaQuery.of(context).size;

    return Stack(
      children: [
        Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            // Car Section with Pressure and Percentage
            Container(
              height: size.height * 0.35, // Reduced height
              child: Stack(
                children: [
                  Center(
                    child: Image.asset(
                      'assets/car_black-transformed1.png',
                      width: size.width * 0.6,
                      height: size.height * 0.3, // Reduced height
                    ),
                  ),

                  // Pressure Info Widgets
                  _buildPositionedInfo(
                    context,
                    top: size.height * 0.04,
                    left: size.width * 0.1,
                    pressure: "-Bar",
                    percentage: "- %",
                    asset: 'assets/Group2.svg',
                  ),
                  _buildPositionedInfo(
                    context,
                    top: size.height * 0.04,
                    right: size.width * 0.1,
                    pressure: "-Bar",
                    percentage: "- %",
                    asset: 'assets/Group2.svg',
                    alignRight: true,
                    flipSvg: true,
                  ),
                  _buildPositionedInfo(
                    context,
                    bottom: size.height * 0.07,
                    left: size.width * 0.1,
                    pressure: "-Bar",
                    percentage: "- %",
                    asset: 'assets/Group1.svg',
                    flipSvg: true,
                  ),
                  _buildPositionedInfo(
                    context,
                    bottom: size.height * 0.07,
                    right: size.width * 0.1,
                    pressure: "-Bar",
                    percentage: "- %",
                    asset: 'assets/Group1.svg',
                    alignRight: true,
                  ),
                ],
              ),
            ),
          ],
        ),

        // Bluetooth Icon
        Positioned(
          top: 22, // Reduced top margin
          left: 10,
          child: GestureDetector(
            onTap: () {
              // Show popup
              showDialog(
                context: context,
                builder: (_) => BluetoothPopup(),
              );
            },
            child: Icon(
              Icons.bluetooth,
              color: Colors.pink,
              size: 24,
            ),
          ),
        ),

        // Centered Text "Bar"
        Positioned(
          top: 250, // Reduced height to minimize empty space
          left: size.width / 2 - 20, // Center horizontally
          child: Text(
            '- Bar',
            style: TextStyle(
              color: Colors.white,
              fontSize: size.width * 0.035, // Make responsive
            ),
          ),
        ),
      ],
    );
  }

  // Helper method for positioned info
  Widget _buildPositionedInfo(
    BuildContext context, {
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
  Widget _buildPressureInfo(String pressure, String percentage, String asset,
      {bool alignRight = false, bool flipSvg = false}) {
    return Column(
      crossAxisAlignment:
          alignRight ? CrossAxisAlignment.end : CrossAxisAlignment.start,
      children: [
        // Text before SVG
        Text(
          pressure,
          style: TextStyle(
            color: Colors.white,
            fontSize: 14,
          ),
        ),
        SizedBox(height: 10), // Reduced spacing

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
                CircularProgressIndicator(), // Fallback if SVG fails to load
          ),
        ),
        SizedBox(height: 4), // Spacing between SVG and text

        // Text after SVG
        Text(
          percentage,
          style: TextStyle(
            color: Colors.white,
            fontSize: 14,
          ),
        ),
      ],
    );
  }
}
