import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'models/appSettings.dart';
import 'package:oasman_mobile/ble_manager.dart';
import 'package:oasman_mobile/provider/unit_provider.dart'; // Import UnitProvider
import 'package:oasman_mobile/pages/menu.dart';
import 'package:oasman_mobile/pages/buttons.dart';
import 'package:oasman_mobile/pages/setup.dart';
import 'package:oasman_mobile/pages/header.dart'; // Import your header

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  runApp(
    MultiProvider(
      providers: [
        ChangeNotifierProvider(
            create: (_) => BLEManager()), // BLEManager globally available
        ChangeNotifierProvider(
            create: (_) => UnitProvider()), // UnitProvider globally available
      ],
      child: MyApp(),
    ),
  );
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  bool _isReady = false;

  @override
  void initState() {
    super.initState();
    _initializeApp();
  }

  Future<void> _initializeApp() async {
    //final bleManager = BLEManager();
    await loadGlobalSettings(); // Load settings at startup
    //await bleManager.startScan(); // Auto-connect logic inside startScan

    setState(() => _isReady = true);
  }

  @override
  Widget build(BuildContext context) {
    if (!_isReady) {
      // Show loading indicator while settings load
      return const MaterialApp(
        home: Scaffold(
          body: Center(child: CircularProgressIndicator()),
        ),
      );
    }

    // App is ready, use globalSettings
    return MaterialApp(
      title: 'OAS-Man',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xFF000000)),
        useMaterial3: true,
      ),
      home: const MainPage(), // Main page
      routes: {
        '/buttons': (context) => const ButtonsPage(),
        '/setup': (context) => const SettingsPage(),
      },
    );
  }
}

class MainPage extends StatefulWidget {
  const MainPage({super.key});

  @override
  State<MainPage> createState() => _MainPageState();
}

class _MainPageState extends State<MainPage> {
  int _selectedIndex = 0;

  final GlobalKey<SettingsPageState> _settingsKey =
      GlobalKey<SettingsPageState>();
  // List of pages
  List<Widget> get _pages => <Widget>[
        const ButtonsPage(),
        SettingsPage(key: _settingsKey),
      ];

  void _onItemTapped(int index) {
    if (_selectedIndex == 1 && index != 1) {
      _settingsKey.currentState?.onLeavePage();
    }
    setState(() {
      _selectedIndex = index;
      print("Navigated to page index: $_selectedIndex");
    });
  }

  @override
  Widget build(BuildContext context) {
    final size = MediaQuery.of(context).size;
    final orientation = MediaQuery.of(context).orientation;

    return Scaffold(
      body: orientation == Orientation.portrait
          ? Column(
              children: [
                if (_selectedIndex == 0) const Header(), // Header at the top
                Expanded(child: _pages[_selectedIndex]), // Main page below
              ],
            )
          : Row(
              children: [
                // Left side (40% of screen) for header in landscape mode
                SizedBox(
                  width: size.width * 0.40,
                  child: const Header(), // Header on the left side in landscape
                ),
                // Right side (60% of screen) for the selected page
                Expanded(child: _pages[_selectedIndex]),
              ],
            ),
      bottomNavigationBar: CustomBottomNavigationBar(
        selectedIndex: _selectedIndex,
        onItemTapped: _onItemTapped,
        itemCount: _pages.length,
      ),
    );
  }
}
