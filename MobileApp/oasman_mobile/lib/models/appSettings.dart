import 'package:shared_preferences/shared_preferences.dart';


class AppSettings {
  String uploadedPicture = '';
  String units = 'Psi';
  String passkeyText = '202777';

  AppSettings({
    required this.units,
    required this.passkeyText,
    required this.uploadedPicture,
  });

  factory AppSettings.fromPrefs(SharedPreferences prefs) {
    return AppSettings(
      units : prefs.getString('_units') ?? 'Psi',
      passkeyText : prefs.getString('_passkeyText') ?? '202777',
      uploadedPicture: prefs.getString('uploaded_image') ?? '',
    );
  }
}

AppSettings? globalSettings;

// Helper to load settings
Future<void> loadGlobalSettings() async {
  final prefs = await SharedPreferences.getInstance();
  globalSettings = AppSettings(
      units : prefs.getString('_units') ?? 'Psi',
      passkeyText : prefs.getString('_passkeyText') ?? '202777',
      uploadedPicture: prefs.getString('uploaded_image') ?? '',
  );
}