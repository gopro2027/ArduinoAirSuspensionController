import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
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
        ? size.width * 0.07
        : size.width * 0.04;

    return Consumer2<BLEManager, UnitProvider>(
      builder: (context, bleManager, unitProvider, child) {
        return Container(
          color: Colors.black,
          child: Stack(
            children: [
              orientation == Orientation.portrait
                  ? _buildPortraitLayout(size, bleManager, unitProvider)
                  : _buildLandscapeLayout(size, bleManager, unitProvider),
              Positioned(
                top: orientation == Orientation.portrait
                    ? 8
                    : size.height * 0.10,
                left: orientation == Orientation.portrait
                    ? size.width * 0.01
                    : size.width * 0.18,
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
                      Icon(Icons.key, color: Colors.green, size: iconSize)
                    else
                      Icon(Icons.key_off, color: Colors.pink, size: iconSize),
                  ],
                ),
              ),
            ],
          ),
        );
      },
    );
  }

  Widget _buildPortraitLayout(
      Size size, BLEManager bleManager, UnitProvider unitProvider) {
    // Cap the header height so it never exceeds 40% of available space,
    // but also keep a minimum so the car image remains visible.
    final headerHeight = (size.height * 0.30).clamp(180.0, 320.0);

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        SizedBox(
          height: headerHeight,
          child: Stack(
            children: [
              Center(
                child: CarImageWidget(
                  width: size.width * 0.55,
                  height: headerHeight * 0.7,
                ),
              ),
              _buildPositionedInfo(
                top: 0,
                left: size.width * 0.1,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["frontLeft"] ?? "0") ??
                    0.0,
                asset: 'assets/Group2.svg',
                unitProvider: unitProvider,
                heightSensorMode: bleManager.heightSensorMode,
              ),
              _buildPositionedInfo(
                top: 0,
                right: size.width * 0.1,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["frontRight"] ?? "0") ??
                    0.0,
                asset: 'assets/Group2.svg',
                alignRight: true,
                flipSvg: true,
                unitProvider: unitProvider,
                heightSensorMode: bleManager.heightSensorMode,
              ),
              _buildPositionedInfo(
                bottom: 24,
                left: size.width * 0.1,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["rearLeft"] ?? "0") ??
                    0.0,
                asset: 'assets/Group1.svg',
                flipSvg: true,
                unitProvider: unitProvider,
                heightSensorMode: bleManager.heightSensorMode,
              ),
              _buildPositionedInfo(
                bottom: 24,
                right: size.width * 0.1,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["rearRight"] ?? "0") ??
                    0.0,
                asset: 'assets/Group1.svg',
                alignRight: true,
                unitProvider: unitProvider,
                heightSensorMode: bleManager.heightSensorMode,
              ),
              Align(
                alignment: Alignment.bottomCenter,
                child: Padding(
                  padding: const EdgeInsets.only(bottom: 2),
                  child: _buildTankPressure(bleManager, unitProvider),
                ),
              ),
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
        SizedBox(
          height: size.height - 97,
          child: Stack(
            children: [
              Center(
                child: CarImageWidget(
                  width: size.width * 0.6,
                  height: size.height * 0.3,
                ),
              ),
              _buildPositionedInfo(
                top: size.height * 0.07,
                left: size.width * 0.01,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["frontLeft"] ?? "0") ??
                    0.0,
                asset: 'assets/Group2.svg',
                unitProvider: unitProvider,
                heightSensorMode: bleManager.heightSensorMode,
              ),
              _buildPositionedInfo(
                top: size.height * 0.07,
                right: size.width * 0.01,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["frontRight"] ?? "0") ??
                    0.0,
                asset: 'assets/Group2.svg',
                alignRight: true,
                flipSvg: true,
                unitProvider: unitProvider,
                heightSensorMode: bleManager.heightSensorMode,
              ),
              _buildPositionedInfo(
                bottom: size.height * 0.12,
                left: size.width * 0.01,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["rearLeft"] ?? "0") ??
                    0.0,
                asset: 'assets/Group1.svg',
                flipSvg: true,
                unitProvider: unitProvider,
                heightSensorMode: bleManager.heightSensorMode,
              ),
              _buildPositionedInfo(
                bottom: size.height * 0.12,
                right: size.width * 0.01,
                rawPressure: double.tryParse(
                        bleManager.pressureValues["rearRight"] ?? "0") ??
                    0.0,
                asset: 'assets/Group1.svg',
                alignRight: true,
                unitProvider: unitProvider,
                heightSensorMode: bleManager.heightSensorMode,
              ),
              Align(
                alignment: Alignment.bottomCenter,
                child: _buildTankPressure(bleManager, unitProvider),
              ),
            ],
          ),
        ),
      ],
    );
  }

  Widget _buildTankPressure(BLEManager bleManager, UnitProvider unitProvider) {
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        // Mirrors the Wireless_Controller status bar's "Adjusting" label
        // (StatusPacketBittset.ADJUSTMENT_IN_PROGRESS): a corner is actively
        // filling or dumping toward a target.
        if (bleManager.adjustmentInProgress)
          const Text(
            'Adjusting',
            style: TextStyle(
              fontFamily: 'Bebas Neue',
              color: Color(0xFF4ADE80),
              fontSize: 18,
            ),
          ),
        _buildTankPressureRow(bleManager, unitProvider),
      ],
    );
  }

  Widget _buildTankPressureRow(
      BLEManager bleManager, UnitProvider unitProvider) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      mainAxisSize: MainAxisSize.min,
      children: [
        if (bleManager.safetyMode)
          const Text('SAFETY MODE',
              style: TextStyle(
                color: Colors.pink,
                fontFamily: 'Bebas Neue',
                fontSize: 23,
              ))
        else ...[
          if (bleManager.compressorOn)
            const Icon(Icons.power_settings_new, color: Colors.pink, size: 22),
          if (bleManager.compressorFrozen)
            const Icon(Icons.ac_unit, color: Colors.pink, size: 22),
          const SizedBox(width: 6),
          Text(
            "${unitProvider.unit == 'Bar' ? unitProvider.convertToBar(double.tryParse(bleManager.pressureValues["tankPressure"] ?? "0") ?? 0.0).toStringAsFixed(2) : (double.tryParse(bleManager.pressureValues["tankPressure"] ?? "0") ?? 0.0).toStringAsFixed(2)} ${unitProvider.unit}",
            style: const TextStyle(
              fontFamily: 'Bebas Neue',
              color: Colors.pink,
              fontSize: 22,
            ),
            textAlign: TextAlign.center,
          ),
        ],
      ],
    );
  }

  Widget _buildPositionedInfo({
    double? top,
    double? bottom,
    double? left,
    double? right,
    required double rawPressure,
    required String asset,
    required UnitProvider unitProvider,
    required bool heightSensorMode,
    bool alignRight = false,
    bool flipSvg = false,
  }) {
    // In level sensor mode the manifold packs height percentage into the same
    // STATUSREPORT wheel fields (Wheel::getSelectedInputValue), so it must not
    // be run through the PSI/Bar conversion. Matches updatePressure() on the
    // Wireless_Controller, which renders "%u%%" for corners in this mode.
    // The tank reading is always a real pressure and is not affected.
    final String label;
    if (heightSensorMode) {
      label = "${rawPressure.toStringAsFixed(0)}%";
    } else {
      final convertedPressure = unitProvider.unit == 'Bar'
          ? unitProvider.convertToBar(rawPressure).toStringAsFixed(2)
          : rawPressure.toStringAsFixed(2);
      label = "$convertedPressure ${unitProvider.unit}";
    }

    return Positioned(
      top: top,
      bottom: bottom,
      left: left,
      right: right,
      child: _buildPressureInfo(
        label,
        asset,
        alignRight: alignRight,
        flipSvg: flipSvg,
      ),
    );
  }

  Widget _buildPressureInfo(
    String pressure,
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
              ? const Offset(0, 30)
              : const Offset(0, 10),
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
      ],
    );
  }
}
