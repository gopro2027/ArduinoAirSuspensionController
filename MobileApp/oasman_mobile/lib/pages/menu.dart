import 'package:flutter/material.dart';

import '../theme/app_theme.dart';

class CustomBottomNavigationBar extends StatelessWidget {
  final int selectedIndex;
  final Function(int) onItemTapped;
  final int itemCount;

  const CustomBottomNavigationBar({
    super.key,
    required this.selectedIndex,
    required this.onItemTapped,
    required this.itemCount,
  });

  /// Total bar height. A Material [BottomNavigationBar] reserves room for a
  /// label and its own padding even when the label is empty, which wasted
  /// vertical space the valve controls need. This is a plain row instead.
  static const double _barHeight = 44;
  static const Color _idle = Color.fromARGB(255, 239, 239, 239);

  @override
  Widget build(BuildContext context) {
    final accent = AppTheme.accent(context);
    return Container(
      color: const Color(0xFF121212),
      height: _barHeight,
      child: Row(
        children: List.generate(itemCount, (index) {
          final selected = selectedIndex == index;
          final color = selected ? accent : _idle;
          return Expanded(
            child: InkWell(
              onTap: () => onItemTapped(index),
              child: Column(
                children: [
                  // Selected-item underline, full item width.
                  Container(
                    height: 3,
                    color: selected ? accent : Colors.transparent,
                  ),
                  Expanded(
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(
                          index == 0 ? Icons.gamepad : Icons.settings,
                          color: color,
                          size: 20,
                        ),
                        const SizedBox(width: 6),
                        Text(
                          index == 0 ? 'Controller' : 'Setup',
                          style: TextStyle(color: color, fontSize: 13),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          );
        }),
      ),
    );
  }
}
