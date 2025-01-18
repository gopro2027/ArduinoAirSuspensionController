import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:provider/provider.dart';
import 'popup/bluetooth.dart';
import '../ble_manager.dart';
import '../provider/unit_provider.dart';

class Header extends StatelessWidget {
  const Header({super.key});

  @override
  Widget build(BuildContext context) {
    final size = MediaQuery.of(context).size;

    return Consumer2<BLEManager, UnitProvider>(
      builder: (context, bleManager, unitProvider, child) {
        return Stack(
          children: [
            Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Container(
                  margin: const EdgeInsets.only(top: 30),
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
                        rawPressure: double.tryParse(
                                bleManager.pressureValues["frontLeft"] ?? "0") ??
                            0.0,
                        percentage: "- %",
                        asset: 'assets/Group2.svg',
                        unitProvider: unitProvider,
                      ),
                      _buildPositionedInfo(
                        top: size.height * 0.04,
                        right: size.width * 0.1,
                        rawPressure: double.tryParse(
                                bleManager.pressureValues["frontRight"] ?? "0") ??
                            0.0,
                        percentage: "- %",
                        asset: 'assets/Group2.svg',
                        alignRight: true,
                        flipSvg: true,
                        unitProvider: unitProvider,
                      ),
                      _buildPositionedInfo(
                        bottom: size.height * 0.07,
                        left: size.width * 0.1,
                        rawPressure: double.tryParse(
                                bleManager.pressureValues["rearLeft"] ?? "0") ??
                            0.0,
                        percentage: "- %",
                        asset: 'assets/Group1.svg',
                        flipSvg: true,
                        unitProvider: unitProvider,
                      ),
                      _buildPositionedInfo(
                        bottom: size.height * 0.07,
                        right: size.width * 0.1,
                        rawPressure: double.tryParse(
                                bleManager.pressureValues["rearRight"] ?? "0") ??
                            0.0,
                        percentage: "- %",
                        asset: 'assets/Group1.svg',
                        alignRight: true,
                        unitProvider: unitProvider,
                      ),

                      // Centered Tank Pressure
                      Positioned(
                        bottom: size.height * 0.02,
                        left: size.width / 2 - 22,
                        child: Text(
                          "${unitProvider.unit == 'Bar' ? unitProvider.convertToBar(double.tryParse(bleManager.pressureValues["tankPressure"] ?? "0") ?? 0.0).toStringAsFixed(2) : (double.tryParse(bleManager.pressureValues["tankPressure"] ?? "0") ?? 0.0).toStringAsFixed(2)} ${unitProvider.unit}",
                          style: TextStyle(
                            color: Colors.white,
                            fontSize: size.width * 0.035,
                          ),
                          textAlign: TextAlign.center,
                        ),
                      ),
                    ],
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
                  showDialog(
                    context: context,
                    builder: (_) => const BluetoothPopup(),
                  );
                },
                child: Icon(
                  Icons.bluetooth,
                  color: bleManager.connectedDevice != null
                      ? Colors.green
                      : Colors.pink,
                  size: size.width * 0.07,
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
    required double rawPressure,
    required String percentage,
    required String asset,
    required UnitProvider unitProvider,
    bool alignRight = false,
    bool flipSvg = false,
  }) {
    final convertedPressure = unitProvider.unit == 'Bar'
        ? unitProvider.convertToBar(rawPressure).toStringAsFixed(2)
        : rawPressure.toStringAsFixed(2);

    return Positioned(
      top: top,
      bottom: bottom,
      left: left,
      right: right,
      child: _buildPressureInfo(
        "$convertedPressure ${unitProvider.unit}",
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
        Text(
          pressure,
          style: const TextStyle(
            color: Colors.white,
            fontSize: 14,
          ),
        ),
        const SizedBox(height: 10),
        Transform(
          transform: flipSvg
              ? (Matrix4.identity()..scale(-1.0, 1.0))
              : Matrix4.identity(),
          alignment: Alignment.center,
          child: SvgPicture.asset(
            asset,
            width: 20,
            height: 20,
            placeholderBuilder: (BuildContext context) =>
                const CircularProgressIndicator(),
          ),
        ),
        const SizedBox(height: 4),
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
