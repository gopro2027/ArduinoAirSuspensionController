import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:provider/provider.dart';

import 'package:oasman_mobile/ble_manager.dart';
import 'package:oasman_mobile/models/appSettings.dart';
import 'package:oasman_mobile/pages/buttons.dart';

/// The controller page has overflowed its height twice (landscape valve pills,
/// then again after the ALL pill was added). These pump it at deliberately
/// cramped sizes and fail if anything overflows.
/// Stands in for a live link so the layout under test is the one users
/// actually operate - the disconnected state adds a "connect a manifold"
/// banner that is not present in normal use.
class _ConnectedBleManager extends BLEManager {
  @override
  bool isConnected() => true;
}

void main() {
  setUp(() {
    globalSettings = AppSettings(
      units: 'Psi',
      passkeyText: '202777',
      uploadedPicture: '',
      pairedManifoldId: '',
      showAllBluetoothDevices: false,
    );
  });

  Future<void> pumpAt(WidgetTester tester, Size size,
      {bool connected = true}) async {
    tester.view.devicePixelRatio = 1.0;
    tester.view.physicalSize = size;
    addTearDown(tester.view.reset);

    await tester.pumpWidget(
      ChangeNotifierProvider<BLEManager>(
        create: (_) => connected ? _ConnectedBleManager() : BLEManager(),
        child: const MaterialApp(
          home: Scaffold(body: ButtonsPage()),
        ),
      ),
    );
    await tester.pump();
  }

  double maxScrollExtent(WidgetTester tester) =>
      tester.state<ScrollableState>(find.byType(Scrollable)).position
          .maxScrollExtent;

  testWidgets('controller page fits in landscape without scrolling',
      (tester) async {
    // Roughly a 16:9 phone in landscape, minus the bottom nav bar.
    await pumpAt(tester, const Size(800, 300));
    expect(tester.takeException(), isNull);
    // Nothing to scroll means the content genuinely fit the viewport.
    expect(maxScrollExtent(tester), 0);
  });

  testWidgets('controller page fits in portrait without scrolling',
      (tester) async {
    await pumpAt(tester, const Size(360, 640));
    expect(tester.takeException(), isNull);
    expect(maxScrollExtent(tester), 0);
  });

  testWidgets('controller page does not overflow when disconnected',
      (tester) async {
    // The disconnected banner makes the content taller; it may scroll, but it
    // must never overflow.
    await pumpAt(tester, const Size(800, 300), connected: false);
    expect(tester.takeException(), isNull);
  });

  testWidgets('controller page scrolls instead of overflowing when very short',
      (tester) async {
    // Far shorter than the content needs; it must scroll rather than overflow.
    await pumpAt(tester, const Size(800, 180));
    expect(tester.takeException(), isNull);
    expect(maxScrollExtent(tester), greaterThan(0));
  });
}
