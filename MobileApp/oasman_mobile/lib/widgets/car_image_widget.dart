import 'dart:io';
import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../theme/app_theme.dart';

class CarImageWidget extends StatefulWidget {
  final double width;
  final double height;

  const CarImageWidget({
    super.key,
    required this.width,
    required this.height,
  });

  @override
  State<CarImageWidget> createState() => _CarImageWidgetState();
  
}

class _CarImageWidgetState extends State<CarImageWidget> {
  File? _uploadedImage;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    _loadUploadedImage(); // reload whenever widget rebuilds
  }
  @override
  void initState() {
    super.initState();
    _loadUploadedImage();
  }

  Future<void> _loadUploadedImage() async {
    final prefs = await SharedPreferences.getInstance();
    final savedPath = prefs.getString('uploaded_image');
    if (savedPath != null && File(savedPath).existsSync()) {
      setState(() {
        _uploadedImage = File(savedPath);
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    if (_uploadedImage != null) {
      return Image.file(
        _uploadedImage!,
        width: widget.width,
        height: widget.height,
        fit: BoxFit.scaleDown,
      );
    }

    // The glow behind the car used to be baked into car_black-transformed1.png
    // as a fixed purple, which no theme could reach. It now ships as a separate
    // alpha-only layer (car_glow.png) that is tinted with the theme accent and
    // drawn behind the car body. Both layers share the same 300x379 canvas, so
    // an identical size + BoxFit keeps them aligned.
    return SizedBox(
      width: widget.width,
      height: widget.height,
      child: Stack(
        alignment: Alignment.center,
        children: [
          ColorFiltered(
            colorFilter:
                ColorFilter.mode(AppTheme.accent(context), BlendMode.srcIn),
            child: Image.asset(
              'assets/car_glow.png',
              width: widget.width,
              height: widget.height,
              fit: BoxFit.contain,
            ),
          ),
          Image.asset(
            'assets/car_body.png',
            width: widget.width,
            height: widget.height,
            fit: BoxFit.contain,
          ),
        ],
      ),
    );
  }
}