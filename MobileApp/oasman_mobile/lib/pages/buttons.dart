import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../ble_manager.dart';
import 'popup/nobt.dart';
import 'popup/bluetooth.dart';
import '../theme/app_theme.dart';

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

class _ButtonsPageState extends State<ButtonsPage> with WidgetsBindingObserver {
  late BLEManager bleManager;
  int _selectedPreset = 3;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    // Leaving the page with a valve held would otherwise leave it open - the
    // release callback never fires once the widget is gone.
    closeAllValves();
    super.dispose();
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    super.didChangeAppLifecycleState(state);
    // Backgrounding, an incoming call, or the notification shade can swallow
    // the touch release. Fail closed rather than leave air moving.
    if (state != AppLifecycleState.resumed) {
      closeAllValves();
    }
  }

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
                      child: Text('Connect',
                          style: TextStyle(color: AppTheme.accent(context))),
                    ),
                  ],
                ),
              ),
            Expanded(
              // Normally everything fits and the grid is centred in the space
              // left above the presets bar. On a short screen the content
              // would previously overflow and paint over the presets; instead
              // it now scrolls. minHeight keeps the fill-the-viewport layout
              // whenever there is room.
              //
              // The scroll view stays OUTSIDE the IgnorePointer: while
              // disconnected the banner makes the content taller, and if
              // IgnorePointer wrapped the scrolling too, the user could not
              // reach the presets bar - it was simply clipped by the nav bar.
              child: LayoutBuilder(
                builder: (context, constraints) {
                  final scale = _uiScale(
                    constraints,
                    MediaQuery.of(context).orientation ==
                        Orientation.landscape,
                  );
                  return SingleChildScrollView(
                    child: ConstrainedBox(
                      constraints:
                          BoxConstraints(minHeight: constraints.maxHeight),
                      child: IntrinsicHeight(
                        child: Opacity(
                          opacity: connected ? 1.0 : 0.4,
                          child: IgnorePointer(
                            ignoring: !connected,
                            child: Column(
                              children: [
                                Expanded(
                                  child: Center(
                                    child: _buildValveGrid(context, scale),
                                  ),
                                ),
                                _buildPresetsBar(context, bleManager, scale),
                              ],
                            ),
                          ),
                        ),
                      ),
                    ),
                  );
                },
              ),
            ),
          ],
        );
      },
    );
  }

  /// How much to enlarge the valve/preset controls for the space available.
  ///
  /// The control sizes are authored for a phone. On a tablet or an in-dash head
  /// unit there is far more room than those sizes use, which left the arrows and
  /// preset buttons tiny in the middle of a large screen. The base figures below
  /// are the natural (unscaled) footprint of the grid plus the presets bar, so
  /// the ratio is simply how many times over the content fits.
  ///
  /// Never returns less than 1: shrinking is not this method's job, and a screen
  /// too small for the base layout is handled by the surrounding scroll view.
  double _uiScale(BoxConstraints c, bool landscape) {
    // Widths come from the widest fixed row in each orientation, not a guess.
    // Portrait: 5 preset circles (5 x 36) + 32 padding = 212, rounded up for the
    // Save/Load row whose width depends on text metrics.
    // Landscape: Save/Load sidebar 76 + 12 gap + circles 180 + 24 padding = 292.
    // Heights: grid (2 pill rows + the ALL row + gaps) plus the presets bar.
    final baseW = landscape ? 300.0 : 240.0;
    final baseH = landscape ? 250.0 : 350.0;
    final fit = math.min(c.maxWidth / baseW, c.maxHeight / baseH);
    // 0.96 keeps a sliver of slack so rounding in the estimates above can never
    // push the content past the viewport and force a needless scrollbar.
    return (fit * 0.96).clamp(1.0, 2.6);
  }

  Widget _buildValveGrid(BuildContext context, double scale) {
    return Padding(
      padding: EdgeInsets.symmetric(horizontal: 16 * scale),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _valve(context, SOLENOID_INDEX.FRONT_DRIVER_IN.index,
                  SOLENOID_INDEX.FRONT_DRIVER_OUT.index, false, scale),
              _valve2(
                  context,
                  SOLENOID_INDEX.FRONT_DRIVER_IN.index,
                  SOLENOID_INDEX.FRONT_PASSENGER_IN.index,
                  SOLENOID_INDEX.FRONT_DRIVER_OUT.index,
                  SOLENOID_INDEX.FRONT_PASSENGER_OUT.index, scale),
              _valve(context, SOLENOID_INDEX.FRONT_PASSENGER_IN.index,
                  SOLENOID_INDEX.FRONT_PASSENGER_OUT.index, false, scale),
            ],
          ),
          SizedBox(height: 8 * scale),
          _valveAll(context, scale),
          SizedBox(height: 8 * scale),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _valve(context, SOLENOID_INDEX.REAR_DRIVER_IN.index,
                  SOLENOID_INDEX.REAR_DRIVER_OUT.index, false, scale),
              _valve2(
                  context,
                  SOLENOID_INDEX.REAR_DRIVER_IN.index,
                  SOLENOID_INDEX.REAR_PASSENGER_IN.index,
                  SOLENOID_INDEX.REAR_DRIVER_OUT.index,
                  SOLENOID_INDEX.REAR_PASSENGER_OUT.index, scale),
              _valve(context, SOLENOID_INDEX.REAR_PASSENGER_IN.index,
                  SOLENOID_INDEX.REAR_PASSENGER_OUT.index, false, scale),
            ],
          ),
        ],
      ),
    );
  }

  Widget _valve(
      BuildContext ctx, int inBit, int outBit, bool large, double scale) {
    return OvalControlButton(
      iconUp: Icons.keyboard_arrow_up,
      iconDown: Icons.keyboard_arrow_down,
      isLarge: large,
      scale: scale,
      onUpPressed: () => openValve(ctx, inBit),
      onUpReleased: () => closeValve(inBit),
      onDownPressed: () => openValve(ctx, outBit),
      onDownReleased: () => closeValve(outBit),
    );
  }

  /// All four corners at once, as two separate side-by-side buttons. Each
  /// carries four arrows, one per wheel. Momentary hold like every other valve
  /// control - never a latch - and release clears only that button's bits.
  Widget _valveAll(BuildContext ctx, double scale) {
    final upMask = (1 << SOLENOID_INDEX.FRONT_PASSENGER_IN.index) |
        (1 << SOLENOID_INDEX.REAR_PASSENGER_IN.index) |
        (1 << SOLENOID_INDEX.FRONT_DRIVER_IN.index) |
        (1 << SOLENOID_INDEX.REAR_DRIVER_IN.index);
    final downMask = (1 << SOLENOID_INDEX.FRONT_PASSENGER_OUT.index) |
        (1 << SOLENOID_INDEX.REAR_PASSENGER_OUT.index) |
        (1 << SOLENOID_INDEX.FRONT_DRIVER_OUT.index) |
        (1 << SOLENOID_INDEX.REAR_DRIVER_OUT.index);
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        AllWheelsButton(
          scale: scale,
          pointingUp: true,
          onPressed: () => openValvesMask(ctx, upMask),
          onReleased: () => closeValveMask(upMask),
        ),
        SizedBox(width: 12 * scale),
        AllWheelsButton(
          scale: scale,
          pointingUp: false,
          onPressed: () => openValvesMask(ctx, downMask),
          onReleased: () => closeValveMask(downMask),
        ),
      ],
    );
  }

  Widget _valve2(BuildContext ctx, int in1, int in2, int out1, int out2,
      double scale) {
    final upMask = (1 << in1) | (1 << in2);
    final downMask = (1 << out1) | (1 << out2);
    return OvalControlButton(
      iconUp: Icons.keyboard_double_arrow_up,
      iconDown: Icons.keyboard_double_arrow_down,
      isLarge: true,
      scale: scale,
      onUpPressed: () => openValvesMask(ctx, upMask),
      onUpReleased: () => closeValveMask(upMask),
      onDownPressed: () => openValvesMask(ctx, downMask),
      onDownReleased: () => closeValveMask(downMask),
    );
  }

  Widget _buildPresetsBar(
      BuildContext context, BLEManager bleManager, double scale) {
    final canUsePresetActions =
        bleManager.connectedDevice != null && _selectedPreset >= 1;

    // Portrait keeps the original stacked layout. Landscape moves Save/Load to
    // a left sidebar, mirroring the Wireless_Controller's landscape presets
    // screen, which also buys back the height that layout needs.
    final landscape =
        MediaQuery.of(context).orientation == Orientation.landscape;
    return landscape
        ? _buildPresetsBarLandscape(context, canUsePresetActions, scale)
        : _buildPresetsBarPortrait(context, canUsePresetActions, scale);
  }

  Widget _buildPresetsBarPortrait(
      BuildContext context, bool canUse, double scale) {
    return Padding(
      padding: EdgeInsets.fromLTRB(16 * scale, 4 * scale, 16 * scale, 12 * scale),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          _buildPresetCircles(context, scale),
          SizedBox(height: 8 * scale),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              _buildSaveButton(context, canUse, compact: false, scale: scale),
              SizedBox(width: 16 * scale),
              _buildLoadButton(context, canUse, compact: false, scale: scale),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildPresetsBarLandscape(
      BuildContext context, bool canUse, double scale) {
    return Padding(
      padding: EdgeInsets.fromLTRB(12 * scale, 4 * scale, 12 * scale, 8 * scale),
      child: Row(
        children: [
          Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              _buildSaveButton(context, canUse, compact: true, scale: scale),
              SizedBox(height: 6 * scale),
              _buildLoadButton(context, canUse, compact: true, scale: scale),
            ],
          ),
          SizedBox(width: 12 * scale),
          Expanded(child: _buildPresetCircles(context, scale)),
        ],
      ),
    );
  }

  Widget _buildPresetCircles(BuildContext context, double scale) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
      children: [
        for (int i = 1; i <= 5; i++)
          GestureDetector(
            onTap: () => _onPresetTapped(context, i),
            child: CircleAvatar(
              radius: 18 * scale,
              backgroundColor: i == _selectedPreset
                  ? AppTheme.accent(context)
                  : Colors.grey[800],
              child: Text(
                '$i',
                style: TextStyle(
                  fontSize: 14 * scale,
                  color:
                      i == _selectedPreset ? Colors.white : Colors.grey[400],
                ),
              ),
            ),
          ),
      ],
    );
  }

  /// [compact] trims the padding and drops the framework's 48px minimum tap
  /// target, used only where landscape height is scarce.
  Widget _buildSaveButton(BuildContext context, bool canUse,
      {required bool compact, double scale = 1.0}) {
    return OutlinedButton(
      onPressed: canUse ? () => _confirmSavePreset(context) : null,
      style: OutlinedButton.styleFrom(
        foregroundColor: AppTheme.accent(context),
        side: BorderSide(color: AppTheme.accent(context)),
        disabledForegroundColor: Colors.white60,
        backgroundColor: const Color(0xFF1E1E1E),
        padding: compact
            ? EdgeInsets.zero
            : EdgeInsets.symmetric(horizontal: 20 * scale, vertical: 8 * scale),
        minimumSize: compact ? Size(76 * scale, 30 * scale) : null,
        textStyle: TextStyle(fontSize: 14 * scale),
        tapTargetSize: compact ? MaterialTapTargetSize.shrinkWrap : null,
      ),
      child: const Text('Save'),
    );
  }

  Widget _buildLoadButton(BuildContext context, bool canUse,
      {required bool compact, double scale = 1.0}) {
    return ElevatedButton(
      onPressed: canUse ? () => _confirmLoadPreset(context) : null,
      style: ElevatedButton.styleFrom(
        backgroundColor: AppTheme.accent(context),
        foregroundColor: Colors.white,
        disabledBackgroundColor: const Color(0xFF2A2A2A),
        disabledForegroundColor: Colors.white60,
        padding: compact
            ? EdgeInsets.zero
            : EdgeInsets.symmetric(horizontal: 20 * scale, vertical: 8 * scale),
        minimumSize: compact ? Size(76 * scale, 30 * scale) : null,
        textStyle: TextStyle(fontSize: 14 * scale),
        tapTargetSize: compact ? MaterialTapTargetSize.shrinkWrap : null,
      ),
      child: const Text('Load'),
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

  /// Overwrites a stored preset, so confirm first - the Wireless_Controller
  /// guards its Save button the same way.
  void _confirmSavePreset(BuildContext context) {
    if (bleManager.connectedDevice == null || _selectedPreset < 1) return;
    showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text('Save current height to preset $_selectedPreset?'),
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
      if (confirmed == true) _saveSelectedPreset();
    });
  }

  void _saveSelectedPreset() {
    if (bleManager.connectedDevice == null || _selectedPreset < 1) return;
    bleManager.sendRestCommand(bleManager.buildRestPacket(
        BTOasIdentifier.SAVECURRENTPRESSURESTOPROFILE,
        [BLEInt(_selectedPreset - 1)]));
    bleManager.sendRestCommand(bleManager.buildConfigReadPacket());
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

  /// Release closes only the valve(s) that button holds, so a second finger
  /// holding another corner keeps working. Matches unsetValveBit() on the
  /// Wireless_Controller. No popup on release - a release must always be
  /// allowed to go through.
  void closeValve(int bit) {
    closeValveMask(1 << bit);
  }

  void closeValveMask(int mask) {
    if (bleManager.connectedDevice != null) {
      bleManager.unsetValveMask(mask);
    }
  }

  /// Close everything, unconditionally. Used for the safety paths (leaving the
  /// page, app backgrounded) rather than for ordinary button releases - the
  /// manifold does not close valves on its own if the link drops.
  void closeAllValves() {
    if (bleManager.connectedDevice != null) {
      bleManager.closeValves();
    }
  }
}

/// Draws a tight stack of chevrons, one per wheel.
///
/// These are painted rather than composed from `keyboard_arrow_*` icons: the
/// ink inside those glyphs is neither centred in its em box nor a predictable
/// fraction of the font size, so stacking them tightly left the arrows visibly
/// off-centre and made the spacing impossible to control. Painting gives exact
/// pitch and exact centring.
class _ChevronStackPainter extends CustomPainter {
  const _ChevronStackPainter({
    required this.color,
    required this.pointingUp,
    this.scale = 1.0,
  });

  final Color color;
  final bool pointingUp;

  /// Enlarges the whole stack on roomier screens; see _uiScale.
  final double scale;

  /// One chevron per wheel.
  static const int count = 4;

  /// Vertical distance between successive chevrons. Smaller than
  /// [chevronHeight], so each tip nests inside the one before it.
  static const double pitch = 4;
  static const double chevronWidth = 15;
  static const double chevronHeight = 5;
  static const double stroke = 2;

  double get _pitch => pitch * scale;
  double get _chevronWidth => chevronWidth * scale;
  double get _chevronHeight => chevronHeight * scale;
  double get _stroke => stroke * scale;

  @override
  void paint(Canvas canvas, Size size) {
    final paint = Paint()
      ..color = color
      ..style = PaintingStyle.stroke
      ..strokeWidth = _stroke
      ..strokeCap = StrokeCap.round
      ..strokeJoin = StrokeJoin.round;

    final totalHeight = _chevronHeight + _pitch * (count - 1);
    final top = (size.height - totalHeight) / 2;
    final cx = size.width / 2;

    for (var i = 0; i < count; i++) {
      final y = top + i * _pitch;
      final path = Path();
      if (pointingUp) {
        path.moveTo(cx - _chevronWidth / 2, y + _chevronHeight);
        path.lineTo(cx, y);
        path.lineTo(cx + _chevronWidth / 2, y + _chevronHeight);
      } else {
        path.moveTo(cx - _chevronWidth / 2, y);
        path.lineTo(cx, y + _chevronHeight);
        path.lineTo(cx + _chevronWidth / 2, y);
      }
      canvas.drawPath(path, paint);
    }
  }

  @override
  bool shouldRepaint(_ChevronStackPainter old) =>
      old.color != color ||
      old.pointingUp != pointingUp ||
      old.scale != scale;
}

/// One direction of the all-corners control: a single momentary-hold button
/// showing one arrow per wheel.
class AllWheelsButton extends StatelessWidget {
  final bool pointingUp;
  final VoidCallback onPressed;
  final VoidCallback onReleased;
  final double scale;

  const AllWheelsButton({
    super.key,
    required this.pointingUp,
    required this.onPressed,
    required this.onReleased,
    this.scale = 1.0,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      behavior: HitTestBehavior.opaque,
      onTapDown: (_) => onPressed(),
      onTapUp: (_) => onReleased(),
      onTapCancel: onReleased,
      child: Container(
        width: 56 * scale,
        height: 40 * scale,
        decoration: BoxDecoration(
          color: const Color(0xFF1E1E1E),
          borderRadius: BorderRadius.circular(20 * scale),
          border: Border.all(
            color: AppTheme.accent(context).withOpacity(0.15),
            width: 1,
          ),
        ),
        // The painter centres the stack within these bounds, so the arrows sit
        // dead centre regardless of how tightly they are packed.
        child: CustomPaint(
          painter: _ChevronStackPainter(
            color: AppTheme.accent(context),
            pointingUp: pointingUp,
            scale: scale,
          ),
        ),
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
  // Separate release callbacks: each half closes only its own valve(s), so
  // holding two controls at once and lifting one finger leaves the other open.
  final VoidCallback? onUpReleased;
  final VoidCallback? onDownReleased;

  /// Enlarges the pill on roomier screens; see _uiScale.
  final double scale;

  const OvalControlButton({
    super.key,
    required this.iconUp,
    required this.iconDown,
    this.isLarge = false,
    this.scale = 1.0,
    this.onUpPressed,
    this.onDownPressed,
    this.onUpReleased,
    this.onDownReleased,
  });

  @override
  Widget build(BuildContext context) {
    // Landscape uses horizontal pills (left = up, right = down), matching
    // calculatePillDimensions() on the Wireless_Controller. Stacking them
    // vertically in landscape made the grid taller than the available space,
    // so it overflowed and painted over the presets bar.
    final landscape =
        MediaQuery.of(context).orientation == Orientation.landscape;
    final long = (isLarge ? 96.0 : 72.0) * scale;
    final short = (isLarge ? 56.0 : 48.0) * scale;

    return Container(
      width: landscape ? long : short,
      height: landscape ? short : long,
      decoration: BoxDecoration(
        color: const Color(0xFF1E1E1E),
        borderRadius: BorderRadius.circular(short / 2),
        border: Border.all(
          color: AppTheme.accent(context).withOpacity(0.15),
          width: 1,
        ),
      ),
      child: Flex(
        direction: landscape ? Axis.horizontal : Axis.vertical,
        children: [
          // Each half fills its side of the pill so the whole surface is
          // tappable, rather than only the icon glyph.
          Expanded(
            child: ControlButton(
              icon: iconUp,
              iconSize: 22 * scale,
              onPressed: onUpPressed,
              onReleased: onUpReleased,
            ),
          ),
          Expanded(
            child: ControlButton(
              icon: iconDown,
              iconSize: 22 * scale,
              onPressed: onDownPressed,
              onReleased: onDownReleased,
            ),
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
  final double iconSize;

  const ControlButton({
    super.key,
    required this.icon,
    this.onPressed,
    this.onReleased,
    this.iconSize = 22,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      // Opaque so the entire half of the pill responds, not just the glyph.
      behavior: HitTestBehavior.opaque,
      onTapDown: (_) => onPressed?.call(),
      onTapUp: (_) => onReleased?.call(),
      onTapCancel: () => onReleased?.call(),
      child: Center(
        child: Icon(
          icon,
          color: AppTheme.accent(context),
          size: iconSize,
        ),
      ),
    );
  }
}
