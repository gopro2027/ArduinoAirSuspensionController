


function generate_manifest(name, version, bootloader, partitions, firmware) {

    var manifest = {
        "name": name,
        "version": version,
        "funding_url": "https://www.patreon.com/oasman",
        "new_install_prompt_erase": true,
        "builds": [
            {
                "chipFamily": "ESP32",
                "improv": false,
                "parts": [
                    { "path": bootloader, "offset": 4096 }, // on the s3 this might be 0 but otherwise all the offsets are the same
                    { "path": partitions, "offset": 32768 }, // 0x8000
                    { "path": "https://raw.githubusercontent.com/gopro2027/ArduinoAirSuspensionController/refs/heads/main/docs/flash/boot_app0.bin", "offset": 57344 }, // 0xe000
                    { "path": firmware, "offset": 65536 } // 0x10000
                ]
            }
        ]
    }

    //"C:\Users\user\.platformio\penv\Scripts\python.exe" "C:\Users\user\.platformio\packages\tool-esptoolpy\esptool.py" --chip esp32s3 --port "COM5" --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 16MB 
    // 0x0000 C:\Users\user\Documents\GitHub\ArduinoAirSuspensionController\Wireless_Controller\.pio\build\esp32-s3touchlcd2p8\bootloader.bin 
    // 0x8000 C:\Users\user\Documents\GitHub\ArduinoAirSuspensionController\Wireless_Controller\.pio\build\esp32-s3touchlcd2p8\partitions.bin 
    // 0xe000 C:\Users\user\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin 
    // 0x10000 .pio\build\esp32-s3touchlcd2p8\firmware.bin

    // duplicate data for the s3 family
    manifest["builds"].push(structuredClone(manifest["builds"][0]));
    manifest["builds"][1]["chipFamily"] = "ESP32-S3";
    manifest["builds"][1]["parts"][0]["offset"] = 0;

    console.log(manifest);

    var json = JSON.stringify(manifest);
    var blob = new Blob([json], {type: "application/json"});
    return URL.createObjectURL(blob)
}

