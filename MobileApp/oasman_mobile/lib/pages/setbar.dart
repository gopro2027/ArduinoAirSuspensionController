import 'package:flutter/material.dart';

class setbar extends StatelessWidget {
  const setbar({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings Page'),
      ),
      body: const Center(
        child: Text('settings'),
      ),
    );
  }
}