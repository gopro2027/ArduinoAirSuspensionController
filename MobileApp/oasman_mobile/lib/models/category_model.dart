import 'package:flutter/material.dart';

class CategoryModel {
  String name;
  String iconPath;
  Color boxColor;

  CategoryModel({
    required this.name,
    required this.iconPath,
    required this.boxColor,
  });

  static List<CategoryModel> getCategories() {
    List<CategoryModel> categories = [];

    categories.add(
      CategoryModel(
        name: 'Salad',
        iconPath: 'assets/icons/plate.svg',
        boxColor: Color(0xFF22aabb)
      )
    );

    categories.add(
      CategoryModel(
        name: 'Chocolate',
        iconPath: 'assets/icons/chocolate.svg',
        boxColor: Color.fromARGB(255, 183, 185, 40)
      )
    );

    return categories;
  }
}