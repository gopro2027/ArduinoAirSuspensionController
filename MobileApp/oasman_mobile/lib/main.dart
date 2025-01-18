import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:oasman_mobile/ble_manager.dart';
import 'package:oasman_mobile/provider/unit_provider.dart'; // Import UnitProvider
import 'package:oasman_mobile/pages/menu.dart';
import 'package:oasman_mobile/pages/buttons.dart';
import 'package:oasman_mobile/pages/setup.dart';

void main() {
  runApp(
    MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => BLEManager()), // BLEManager globally available
        ChangeNotifierProvider(create: (_) => UnitProvider()), // UnitProvider globally available
      ],
      child: const MyApp(),
    ),
  );
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
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

  // List of pages
  static const List<Widget> _pages = <Widget>[
    ButtonsPage(),
    SettingsPage(),
  ];

  void _onItemTapped(int index) {
    setState(() {
      _selectedIndex = index;
      print("Navigated to page index: $_selectedIndex");
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Column(
        children: [
          Expanded(
            child: _pages[_selectedIndex], // Switch page based on index
          ),
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
