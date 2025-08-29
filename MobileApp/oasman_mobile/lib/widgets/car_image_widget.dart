import 'dart:io';
import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

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
    return _uploadedImage != null
        ? Image.file(
            _uploadedImage!,
            width: widget.width,
            height: widget.height,
            fit: BoxFit.scaleDown,
          )
        : Image.asset(
            'assets/car_black-transformed1.png',
            width: widget.width,
            height: widget.height,
            fit: BoxFit.contain,
          );
  }
}