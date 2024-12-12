import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';

class HomePage extends StatelessWidget {
  const HomePage({super.key});

  @override
  Widget build(BuildContext context) {
    final size = MediaQuery.of(context).size;

    return Scaffold(
      backgroundColor: Color(0xFF121212),
      body: SingleChildScrollView(
        child: Container(
          padding: EdgeInsets.only(top: 0), // Adjust this value to move the entire window up
          child: Column(
            children: [
              // Top Bluetooth Icon
              Padding(
                padding: const EdgeInsets.all(1.0),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.start,
                  children: [
                    Icon(
                      Icons.bluetooth,
                      color: Colors.pink,
                      size: 24,
                    ),
                  ],
                ),
              ),

              // Car Section with Pressure and Percentage
              Container(
                height: size.height * 0.4,
                child: Stack(
                  children: [
                    Center(
                      child: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Image.asset(
                            'assets/car_black-transformed1.png',
                            width: size.width * 0.6,
                            height: size.height * 0.4,
                          ),
                        ],
                      ),
                    ),

                    // Top Left
                    Positioned(
                      top: size.height * 0.04,
                      left: size.width * 0.05,
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          
                           Text(
                            '-Bar',
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: 14,
                            ),
                          ),
                           SizedBox(height: 20),
                          SvgPicture.asset(
                            'assets/Group2.svg',
                            width: 20,
                            height: 20,
                          ),
                          SizedBox(height: 4),
                          Text(
                            '- %',
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: 14,
                            ),
                          ),
                        ],
                      ),
                    ),

      // Top Right
      Positioned(
        top: size.height * 0.04,
        right: size.width * 0.05,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.end,
          children: [         Text(
                            '-Bar',
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: 14,
                            ),
                          ),
               SizedBox(height: 20),
            Transform(
              transform: Matrix4.identity()..scale(-1.0, 1.0), // Flip horizontally
              alignment: Alignment.center,
              child: SvgPicture.asset(
                'assets/Group2.svg',
                width: 20,
                height: 20,
              ),
            ),
            SizedBox(height: 4),
            Text(
              '- %',
              style: TextStyle(
                color: Colors.white,
                fontSize: 14,
              ),
            ),
          ],
        ),
      ),

                    // Bottom Left
                    Positioned(
                      bottom: size.height * 0.09,
                      left: size.width * 0.05,
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                                   Text(
                            '-Bar',
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: 14,
                            ),
                          ),
            Transform(
              transform: Matrix4.identity()..scale(-1.0, 1.0), // Flip horizontally
              alignment: Alignment.center,
              child: SvgPicture.asset(
                'assets/Group1.svg',
                width: 20,
                height: 20,
              ),
            ),
                          SizedBox(height: 4),
                          Text(
                            '- %',
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: 14,
                            ),
                          ),
                        ],
                      ),
                    ),

                    // Bottom Right
                    Positioned(
                      bottom: size.height * 0.09,
                      right: size.width * 0.05,
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.end,
                        children: [
                          Text(
                            '-Bar',
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: 14,
                            ),
                          ),
                          SvgPicture.asset(
                            'assets/Group1.svg',
                            width: 20,
                            height: 20,
                          ),
                          SizedBox(height: 4),
                          Text(
                            '- %',
                            style: TextStyle(
                              color: Colors.white,
                              fontSize: 14,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),

              // Control Buttons Section
              Container(
                height: size.height * 0.40,
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    SizedBox(height: 16),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                      children: [
                        // button leftfront
                        OvalControlButton(
                          iconUp: Icons.keyboard_arrow_up,
                          iconDown: Icons.keyboard_arrow_down,
                          onUpPressed: () => print('Left Front Up Pressed'),
                          onDownPressed: () => print('Left Front Down Pressed'),
                        ),
                        // button front
                        OvalControlButton(
                          iconUp: Icons.keyboard_double_arrow_up,
                          iconDown: Icons.keyboard_double_arrow_down,
                          isLarge: true,
                          onUpPressed: () => print('Front Up Pressed'),
                          onDownPressed: () => print('Front Down Pressed'),
                        ),
                        // button rightfront
                        OvalControlButton(
                          iconUp: Icons.keyboard_arrow_up,
                          iconDown: Icons.keyboard_arrow_down,
                          onUpPressed: () => print('Right Front Up Pressed'),
                          onDownPressed: () => print('Right Front Down Pressed'),
                        ),
                      ],
                    ),
                    SizedBox(height: 15),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                      children: [
                        // button leftback
                        OvalControlButton(
                          iconUp: Icons.keyboard_arrow_up,
                          iconDown: Icons.keyboard_arrow_down,
                          onUpPressed: () => print('Left Back Up Pressed'),
                          onDownPressed: () => print('Left Back Down Pressed'),
                        ),
                        // button back
                        OvalControlButton(
                          iconUp: Icons.keyboard_double_arrow_up,
                          iconDown: Icons.keyboard_double_arrow_down,
                          isLarge: true,
                          onUpPressed: () => print('Back Up Pressed'),
                          onDownPressed: () => print('Back Down Pressed'),
                        ),
                        // button rightback
                        OvalControlButton(
                          iconUp: Icons.keyboard_arrow_up,
                          iconDown: Icons.keyboard_arrow_down,
                          onUpPressed: () => print('Right Back Up Pressed'),
                          onDownPressed: () => print('Right Back Down Pressed'),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
              // Presets
              Padding(
                padding: const EdgeInsets.symmetric(vertical: 1.0),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    for (int i = 1; i <= 5; i++)
                      GestureDetector(
                        onTap: () {},
                        child: CircleAvatar(
                          backgroundColor: i == 3 ? Colors.purple : Colors.grey[800],
                          child: Text(
                            '$i',
                            style: TextStyle(
                              color: i == 3 ? Colors.white : Colors.grey[400],
                            ),
                          ),
                        ),
                      ),
                  ],
                ),
              ),
            ],
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

  const OvalControlButton({
    super.key, 
    required this.iconUp,
    required this.iconDown,
    this.isLarge = false,
    this.onUpPressed,
    this.onDownPressed,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      width: isLarge ? 50 : 50,
      height: isLarge ? 110 : 80,
      decoration: BoxDecoration(
        color: Colors.black,
        borderRadius: BorderRadius.circular(isLarge ? 40 : 30),
        boxShadow: [
          BoxShadow(
            color: Colors.black87.withOpacity(0.7),
            blurRadius: 15,
            spreadRadius: 2,
          ),
        ],
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.spaceEvenly,
        children: [
          ControlButton(
            icon: iconUp,
            alignment: Alignment.topCenter,
            onPressed: onUpPressed,
          ),
          ControlButton(
            icon: iconDown,
            alignment: Alignment.bottomCenter,
            onPressed: onDownPressed,
          ),
        ],
      ),
    );
  }
}

class ControlButton extends StatelessWidget {
  final IconData icon;
  final bool isLarge;
  final double? iconSize;
  final AlignmentGeometry alignment;
  final VoidCallback? onPressed;

  const ControlButton({
    super.key, 
    required this.icon,
    this.isLarge = false,
    this.iconSize,
    this.alignment = Alignment.center,
    this.onPressed,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onPressed,
      child: Container(
        width: isLarge ? 40 : 40,
        height: isLarge ? 40 : 40,
        decoration: BoxDecoration(
          color: Colors.black,
          borderRadius: BorderRadius.circular(isLarge ? 30 : 20),
          boxShadow: [
            BoxShadow(
              color: Colors.black87.withOpacity(0.7),
              blurRadius: 15,
              spreadRadius: 2,
            ),
          ],
        ),
        child: Align(
          alignment: alignment,
          child: Icon(
            icon,
            color: Colors.purple,
            size: iconSize ?? (isLarge ? 22 : 20),
          ),
        ),
      ),
    );
  }
}