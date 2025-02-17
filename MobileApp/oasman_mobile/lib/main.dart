import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:oasman_mobile/ble_manager.dart';
import 'package:oasman_mobile/pages/menu.dart';
import 'package:oasman_mobile/pages/buttons.dart';
import 'package:oasman_mobile/pages/setup.dart';

void main() {
  runApp(
    MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => BLEManager()), // Gør BLEManager globalt tilgængelig
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
      home: const MainPage(), // Hovedsiden
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

  // Liste over sider
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
            child: _pages[_selectedIndex], // Vælg side baseret på index
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
