import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../models/appSettings.dart';

/// Colour themes for the app.
///
/// These mirror the Wireless_Controller's theme presets one for one — the
/// values are copied from `THEME_COLOR_*` in
/// `Wireless_Controller/src/utils/util.h` — so a phone and a controller set to
/// the same theme look the same. [id] matches the controller's `ThemePreset`
/// enum value and is what gets persisted, so the ids must not be renumbered.
enum ThemePreset {
  oceanBlue(
    0,
    'Ocean Blue',
    Color(0xFF60A5FA),
    Color(0xFF3B82F6),
    Color(0xFF2563EB),
  ),
  plumpPurple(
    1,
    'Plump Purple',
    Color(0xFFA78BFA),
    Color(0xFF8B5CF6),
    Color(0xFF6D28D9),
  ),
  forestGreen(
    2,
    'Forest Green',
    Color(0xFF34D399),
    Color(0xFF10B981),
    Color(0xFF059669),
  ),
  desertSand(
    3,
    'Desert Sand',
    Color(0xFFE7D399),
    Color(0xFFC4B382),
    Color(0xFFA2946B),
  );

  const ThemePreset(this.id, this.label, this.light, this.medium, this.dark);

  final int id;
  final String label;

  /// The accent used for most UI (text, icons, borders, filled buttons). This
  /// is the shade the app used to hardcode.
  final Color light;

  /// Mid shade, kept for parity with the controller's three-shade themes.
  final Color medium;

  /// Darkest shade, kept for parity with the controller's three-shade themes.
  final Color dark;

  static ThemePreset fromId(int id) => ThemePreset.values.firstWhere(
        (p) => p.id == id,
        orElse: () => defaultPreset,
      );

  /// Purple is the default because it is what the app shipped with.
  static const ThemePreset defaultPreset = ThemePreset.plumpPurple;
}

/// Selected theme, persisted to SharedPreferences.
class ThemeProvider extends ChangeNotifier {
  ThemePreset _preset =
      ThemePreset.fromId(globalSettings?.themePreset ?? ThemePreset.defaultPreset.id);

  ThemePreset get preset => _preset;

  Future<void> setPreset(ThemePreset preset) async {
    if (_preset == preset) return;
    _preset = preset;
    globalSettings?.themePreset = preset.id;
    notifyListeners();
    final prefs = await SharedPreferences.getInstance();
    await prefs.setInt(AppSettings.themePresetKey, preset.id);
  }
}

/// Theme helpers.
///
/// The three preset shades are carried on the [ColorScheme] (light → primary,
/// medium → secondary, dark → tertiary) rather than in a widget of their own,
/// so every widget that reads them through [Theme.of] rebuilds automatically
/// when the theme changes — even inside `const` subtrees.
class AppTheme {
  const AppTheme._();

  static ThemeData themeData(ThemePreset preset) {
    return ThemeData(
      colorScheme: ColorScheme.fromSeed(seedColor: const Color(0xFF000000))
          .copyWith(
        primary: preset.light,
        secondary: preset.medium,
        tertiary: preset.dark,
      ),
      useMaterial3: true,
    );
  }

  /// The accent colour — use this anywhere the app used to write
  /// `Color(0xFFBB86FC)`.
  static Color accent(BuildContext context) =>
      Theme.of(context).colorScheme.primary;

  static Color accentMedium(BuildContext context) =>
      Theme.of(context).colorScheme.secondary;

  static Color accentDark(BuildContext context) =>
      Theme.of(context).colorScheme.tertiary;
}
