import 'package:flutter/material.dart';

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

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;
    final itemWidth = screenWidth / itemCount; // Dynamically calculate width based on item count

    return BottomNavigationBar(
      items: List.generate(
        itemCount,
        (index) => BottomNavigationBarItem(
          icon: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Container(
                height: 3, // Height of the line
                width: itemWidth,
                color: selectedIndex == index
                    ? Color(0xFFBB86FC) // Visible color for selected item
                    : Colors.transparent, // Transparent for unselected items
                margin: EdgeInsets.only(bottom: 5), // Space below the line
              ),
              Row(
                mainAxisSize: MainAxisSize.min,
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Icon(
                    index == 0 ? Icons.gamepad : Icons.settings,
                    color: selectedIndex == index
                        ? Color(0xFFBB86FC) // Selected icon color
                        : Color.fromARGB(255, 239, 239, 239), // Unselected icon color
                  ),
                  SizedBox(width: 8), // Space between icon and text
                  Text(
                    index == 0 ? 'Controller' : 'Setup',
                    style: TextStyle(
                      color: selectedIndex == index
                          ? Color(0xFFBB86FC) // Selected text color
                          : Color.fromARGB(255, 239, 239, 239), // Unselected text color
                    ),
                  ),
                ],
              ),
            ],
          ),
          label: '', // Remove default label positioning
        ),
      ),
      currentIndex: selectedIndex,
      selectedItemColor: Color(0xFFBB86FC),
      unselectedItemColor: Color.fromARGB(255, 239, 239, 239),
      backgroundColor: Color(0xFF121212),
      onTap: onItemTapped,
    );
  }
}
