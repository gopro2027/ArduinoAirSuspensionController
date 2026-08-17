import 'package:shared_preferences/shared_preferences.dart';


class AppSettings {
  String uploadedPicture = '';
  String units = 'Psi';
  String passkeyText = '202777';
  String pairedManifoldId = '';

  /// When true the BLE scan is unfiltered (every advertising device is listed)
  /// instead of only devices advertising the OASMan service UUID. Some Android
  /// devices never report the advertised service UUIDs, so the filtered scan
  /// finds nothing for them.
  bool showAllBluetoothDevices = false;

  AppSettings({
    required this.units,
    required this.passkeyText,
    required this.uploadedPicture,
    required this.pairedManifoldId,
    required this.showAllBluetoothDevices,
  });

  factory AppSettings.fromPrefs(SharedPreferences prefs) {
    print("factory app's settings");
    return AppSettings(
      units : prefs.getString('_units') ?? 'Psi',
      passkeyText : prefs.getString('_passkeyText') ?? '202777',
      uploadedPicture: prefs.getString('uploaded_image') ?? '',
      pairedManifoldId: prefs.getString('_pairedManifoldId') ?? '',
      showAllBluetoothDevices:
          prefs.getBool('_showAllBluetoothDevices') ?? false,
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
      showAllBluetoothDevices:
          prefs.getBool('_showAllBluetoothDevices') ?? false,
  );
}