package dev.oasman.oasman_mobile

import android.content.ActivityNotFoundException
import android.content.Intent
import android.provider.Settings
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel

class MainActivity : FlutterActivity() {

    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, SETTINGS_CHANNEL)
            .setMethodCallHandler { call, result ->
                when (call.method) {
                    "openBluetoothSettings" ->
                        result.success(openSettings(Settings.ACTION_BLUETOOTH_SETTINGS))
                    "openLocationSettings" ->
                        result.success(openSettings(Settings.ACTION_LOCATION_SOURCE_SETTINGS))
                    else -> result.notImplemented()
                }
            }
    }

    /**
     * Open a system settings screen, falling back to the top level settings app
     * when the specific screen isn't resolvable. Aftermarket head units often
     * hide or rename these activities, and doing nothing at all leaves the user
     * with no way in - which is the whole reason this channel exists.
     *
     * Returns true if some settings screen was opened.
     */
    private fun openSettings(action: String): Boolean {
        for (candidate in listOf(action, Settings.ACTION_SETTINGS)) {
            try {
                startActivity(Intent(candidate).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
                return true
            } catch (e: ActivityNotFoundException) {
                // Not available on this ROM - try the next candidate.
            }
        }
        return false
    }

    companion object {
        private const val SETTINGS_CHANNEL = "dev.oasman.oasman_mobile/settings"
    }
}
