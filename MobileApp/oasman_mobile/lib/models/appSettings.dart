import 'package:shared_preferences/shared_preferences.dart';


class AppSettings {
  String uploadedPicture = '';
  String units = 'Psi';
  String passkeyText = '202777';
  String pairedManifoldId = '';
  bool autoConnect = true;
  bool wifiHotspot = false;

  AppSettings({
    required this.units,
    required this.passkeyText,
    required this.uploadedPicture,
    required this.pairedManifoldId,
    required this.autoConnect,
    required this.wifiHotspot,
  });

  factory AppSettings.fromPrefs(SharedPreferences prefs) {
    print("factory app's settings");
    return AppSettings(
      units : prefs.getString('_units') ?? 'Psi',
      passkeyText : prefs.getString('_passkeyText') ?? '202777',
      uploadedPicture: prefs.getString('uploaded_image') ?? '',
      pairedManifoldId: prefs.getString('_pairedManifoldId') ?? '',
      autoConnect: prefs.getBool('_autoConnect') ?? true,
      wifiHotspot: prefs.getBool('_wifiHotspot') ?? false,
    );
  }
}

AppSettings? globalSettings;

// Helper to load settings
Future<void> loadGlobalSettings() async {
  final prefs = await SharedPreferences.getInstance();
  print("load app's settings");
  globalSettings = AppSettings(
      units : prefs.getString('_units') ?? 'Psi',
      passkeyText : prefs.getString('_passkeyText') ?? '202777',
      uploadedPicture: prefs.getString('uploaded_image') ?? '',
      pairedManifoldId: prefs.getString('_pairedManifoldId') ?? '',
      autoConnect: prefs.getBool('_autoConnect') ?? true,
      wifiHotspot: prefs.getBool('_wifiHotspot') ?? false,
  );
}