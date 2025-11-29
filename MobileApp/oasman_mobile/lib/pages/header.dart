import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:oasman_mobile/models/appSettings.dart';
import 'package:provider/provider.dart';
import 'popup/bluetooth.dart';
import '../ble_manager.dart';
import '../provider/unit_provider.dart';
import '../widgets/car_image_widget.dart';

class Header extends StatelessWidget {
  const Header({super.key});

  @override
  Widget build(BuildContext context) {
    final size = MediaQuery.of(context).size;
    final orientation = MediaQuery.of(context).orientation;
    final iconSize = orientation == Orientation.portrait
        ? size.width * 0.07 // Portrait size
        : size.width * 0.04; // Landscape size

    return Consumer2<BLEManager, UnitProvider>(
      builder: (context, bleManager, unitProvider, child) {
        return Container(
          color: Colors.black, // Set the background color to black
          child: Stack(
            children: [
              orientation == Orientation.portrait
                  ? _buildPortraitLayout(size, bleManager, unitProvider)
                  : _buildLandscapeLayout(size, bleManager, unitProvider),

              // Bluetooth Icon with dynamic positioning
              Positioned(
                  // Adjust position based on orientation
                  top: orientation == Orientation.portrait
                      ? size.height * 0.04 // Portrait top
                      : size.height * 0.10, // Landscape top (adjust as needed)
                  left: orientation == Orientation.portrait
                      ? size.width * 0.01 // Portrait left
                      : size.width * 0.18, // Landscape left (adjust as needed)
                  child: Column(
                    children: [
                      GestureDetector(
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
                          size: iconSize,
                        ),
                      ),
                      if (bleManager.vehicleOn)
                        Icon(
                          Icons.key,
                          color: Colors.green,
                          size: iconSize,
                        )
                      else
                        Icon(
                          Icons.key_off,
                          color: Colors.pink,
                          size: iconSize,
                        ),
                      if (globalSettings!.wifiHotspot)
                        Icon(
                          Icons.wifi,
                          color: Colors.green,
                          size: iconSize,
                        ),
                    ],
                  )),
            ],
          ),
        );
      },
    );
  }

  // Portrait layout
  Widget _buildPortraitLayout(
      Size size, BLEManager bleManager, UnitProvider unitProvider) {
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(
          margin: const EdgeInsets.only(top: 30),
          height: size.height * 0.35,
          child: Stack(
            children: [
              // Car Image
              Center(
                child: CarImageWidget(
                  width: size.width * 0.6,
                  height: size.height * 0.3,
                ),
              ),

              // Pressure Info Widgets
              _buildPositionedInfo(
                top: size.height * 0.00,
                left: size.width * 0.1,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["frontLeft"] ?? "0") ??
                    0.0,
                percentage: "- %",
                asset: 'assets/Group2.svg',
                unitProvider: unitProvider,
              ),
              _buildPositionedInfo(
                top: size.height * 0.00,
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
                bottom: size.height * 0.00,
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
                bottom: size.height * 0.00,
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
              Align(
                  alignment: Alignment
                      .bottomCenter, // center horizontally and stick to bottom
                  child: Row(
                    mainAxisAlignment:
                        MainAxisAlignment.center, // <-- center horizontally
                    crossAxisAlignment:
                        CrossAxisAlignment.center, // <-- center vertically
                    children: [
                      if (bleManager.safetyMode)
                        Text('SAFETY MODE',
                            style: TextStyle(
                              color: Colors.pink,
                              fontFamily: 'Bebas Neue',
                              fontSize: 23,
                            ))
                      else ...[
                        if (bleManager.compressorOn)
                          Icon(Icons.power_settings_new,
                              color: Colors.pink, size: 25),
                        if (bleManager.compressorFrozen)
                          Icon(Icons.ac_unit, color: Colors.pink, size: 25),

                        SizedBox(width: 8), // spacing between icon and text

                        Text(
                          "${unitProvider.unit == 'Bar' ? unitProvider.convertToBar(double.tryParse(bleManager.pressureValues["tankPressure"] ?? "0") ?? 0.0).toStringAsFixed(2) : (double.tryParse(bleManager.pressureValues["tankPressure"] ?? "0") ?? 0.0).toStringAsFixed(2)} ${unitProvider.unit}",
                          style: TextStyle(
                            fontFamily: 'Bebas Neue',
                            // TODO if compressor is low
                            color: false
                                ? Colors.white
                                : Colors.pink, // <-- conditional
                            fontSize: 25,
                          ),
                          textAlign: TextAlign.center,
                        ),
                      ],
                    ],
                  )),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildLandscapeLayout(
      Size size, BLEManager bleManager, UnitProvider unitProvider) {
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(
          margin: const EdgeInsets.only(top: 30),
          height: size.height - 97,
          child: Stack(
            children: [
              // Car Image
              Center(
                child: CarImageWidget(
                  width: size.width * 0.6,
                  height: size.height * 0.3,
                ),
              ),

              // Pressure Info Widgets
              _buildPositionedInfo(
                top: size.height * 0.07,
                left: size.width * 0.01,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["frontLeft"] ?? "0") ??
                    0.0,
                percentage: "- %",
                asset: 'assets/Group2.svg',
                unitProvider: unitProvider,
              ),
              _buildPositionedInfo(
                top: size.height * 0.07,
                right: size.width * 0.01,
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
                bottom: size.height * 0.12,
                left: size.width * 0.01,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["rearLeft"] ?? "0") ??
                    0.0,
                percentage: "- %",
                asset: 'assets/Group1.svg',
                flipSvg: true,
                unitProvider: unitProvider,
              ),
              _buildPositionedInfo(
                bottom: size.height * 0.12,
                right: size.width * 0.01,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["rearRight"] ?? "0") ??
                    0.0,
                percentage: "- %",
                asset: 'assets/Group1.svg',
                alignRight: true,
                unitProvider: unitProvider,
              ),

              // Centered Tank Pressure
              Align(
                  alignment: Alignment
                      .bottomCenter, // center horizontally and stick to bottom
                  child: Row(
                    mainAxisAlignment:
                        MainAxisAlignment.center, // <-- center horizontally
                    crossAxisAlignment:
                        CrossAxisAlignment.center, // <-- center vertically
                    children: [
                      if (bleManager.compressorOn)
                        Icon(Icons.power_settings_new,
                            color: Colors.pink, size: 25),
                      if (bleManager.compressorFrozen)
                        Icon(Icons.ac_unit, color: Colors.pink, size: 25),

                      SizedBox(width: 8), // spacing between icon and text

                      Text(
                        "${unitProvider.unit == 'Bar' ? unitProvider.convertToBar(double.tryParse(bleManager.pressureValues["tankPressure"] ?? "0") ?? 0.0).toStringAsFixed(2) : (double.tryParse(bleManager.pressureValues["tankPressure"] ?? "0") ?? 0.0).toStringAsFixed(2)} ${unitProvider.unit}",
                        style: TextStyle(
                          fontFamily: 'Bebas Neue',
                          // TODO if compressor is low
                          color: false
                              ? Colors.white
                              : Colors.pink, // <-- conditional
                          fontSize: 25,
                        ),
                        textAlign: TextAlign.center,
                      ),
                    ],
                  )),
            ],
          ),
        ),
      ],
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
        Transform.translate(
          offset: flipSvg && !alignRight || !flipSvg && alignRight
              ? Offset(0, 30)
              : Offset(0, 10), // x, y
          child: Text(
            pressure,
            style: const TextStyle(
              fontFamily: 'Bebas Neue',
              color: Colors.white,
              fontSize: 25,
            ),
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
        Transform.translate(
          offset: flipSvg && !alignRight || !flipSvg && alignRight
              ? Offset(0, 0)
              : Offset(0, -18),
          child: Text(
            percentage,
            style: const TextStyle(
              fontFamily: 'Bebas Neue',
              color: Colors.white,
              fontSize: 14,
            ),
          ),
        ),
      ],
    );
  }
}
