import 'dart:async';
import 'dart:developer';
import 'dart:io';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:get/get.dart';
import 'package:permission_handler/permission_handler.dart';

// https://pub.dev/packages/flutter_blue_plus#usage

/* 
ERROR CODES MEANING:
-1 - no error
0 - device doesn't have bluetooth
1 - bluetooth scan denied
2 - bluetooth connect denied
3 - bluetooth is not on... currently looping waiting for it to be turned on

 */

class BluetoothController extends GetxController {
  int errorCode = -1;
  void setErrorCode(int i) {
    errorCode = i;
  }

  int getErrorCode() {
    return errorCode;
  }

  void displayErrorInstructions() {
    // TODO: Display toast that promps user what to do based on their error code
    log("THIS IS THE TODO TOAST NO PERMS");
  }

  /*
  This is our main init function which should only be called once
   */
  void initializeBluetooth() async {
    log("Initializing bluetooth");
    //check if the device has bluetooth and turn it on if it is android
    if (await FlutterBluePlus.isSupported == false) {
      setErrorCode(0);
      displayErrorInstructions();
      return;
    }
    if (Platform.isAndroid) {
      await FlutterBluePlus
          .turnOn(); // this only triggers if bluetooth is off so no need to check it
    }

    //in the docs they say to cancel this after but that was causing it to not work for me
    FlutterBluePlus.adapterState.listen((BluetoothAdapterState state) {
      if (state == BluetoothAdapterState.on) {
        // usually start scanning, connecting, etc
        log("Bluetooth on! Ready to go!");
        setErrorCode(-1);
      } else {
        // show an error to the user, etc
        log("bluetooth is not on unfortunately... sleeping and waiting to restart");

        // set error code to 3 so we know we are waiting for the user to turn on the bluetooth. and then run init again after 2 seconds
        setErrorCode(3);
        displayErrorInstructions();
        Future.delayed(const Duration(milliseconds: 2000), () {
          initializeBluetooth();
        });
      }
    });
  }

  /*
  This will prompt the user for perms if necessary
   */
  Future<bool> hasPermissions() async {
    if (await Permission.bluetoothScan.request().isGranted) {
      if (await Permission.bluetoothConnect.request().isGranted) {
        setErrorCode(-1);
        return true;
      } else {
        setErrorCode(2);
        return false;
      }
    } else {
      setErrorCode(1);
      return false;
    }
  }

  Future scanDevices(Function(Stream<List<ScanResult>>) onResults) async {
    if (await hasPermissions()) {
      var subscription = FlutterBluePlus.onScanResults.listen(
        (results) {
          if (results.isNotEmpty) {
            ScanResult r = results.last; // the most recently found device
            log('${r.device.remoteId}: "${r.advertisementData.advName}" found!');
          }
        },
        onError: (e) => log(e),
      );
      FlutterBluePlus.cancelWhenScanComplete(subscription);

      await FlutterBluePlus.startScan(timeout: Duration(seconds: 5));
      log("started scan");

      // wait for scanning to stop
      await FlutterBluePlus.isScanning.where((val) => val == false).first;
      log("scan stopped");
    } else {
      displayErrorInstructions();
    }
  }

  Stream<List<ScanResult>> get scanResults => FlutterBluePlus.scanResults;
}
