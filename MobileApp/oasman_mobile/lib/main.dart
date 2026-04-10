import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:image_picker_android/image_picker_android.dart';
import 'package:image_picker_platform_interface/image_picker_platform_interface.dart';
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

  if (defaultTargetPlatform == TargetPlatform.android) {
    await SystemChrome.setEnabledSystemUIMode(SystemUiMode.edgeToEdge);
    final impl = ImagePickerPlatform.instance;
    if (impl is ImagePickerAndroid) {
      impl.useAndroidPhotoPicker = true;
    }
  }

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
  bool _autoConnectTriggered = false;

  @override
  void initState() {
    super.initState();
    _initializeApp();
  }

  Future<void> _initializeApp() async {
    await loadGlobalSettings(); // Load settings at startup
    setState(() => _isReady = true);
  }

  @override
  Widget build(BuildContext context) {
    if (!_isReady) {
      // Show loading indicator while settings load
      return const MaterialApp(
        home: Scaffold(
          body: SafeArea(
            child: Center(child: CircularProgressIndicator()),
          ),
        ),
      );
    }

    // Enable background auto-reconnect once when app becomes ready
    if (!_autoConnectTriggered) {
      _autoConnectTriggered = true;
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (!context.mounted) return;
        Provider.of<BLEManager>(context, listen: false).enableAutoReconnect();
      });
    }

    // App is ready, use globalSettings
    return MaterialApp(
      title: 'OASMan',
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
      backgroundColor: const Color(0xFF121212),
      body: SafeArea(
        bottom: false,
        child: orientation == Orientation.portrait
            ? Column(
                children: [
                  if (_selectedIndex == 0) const Header(),
                  Expanded(child: _pages[_selectedIndex]),
                ],
              )
            : Row(
                children: [
                  SizedBox(
                    width: size.width * 0.40,
                    child: const Header(),
                  ),
                  Expanded(child: _pages[_selectedIndex]),
                ],
              ),
      ),
      bottomNavigationBar: SafeArea(
        top: false,
        maintainBottomViewPadding: true,
        child: CustomBottomNavigationBar(
          selectedIndex: _selectedIndex,
          onItemTapped: _onItemTapped,
          itemCount: _pages.length,
        ),
      ),
    );
  }
}
