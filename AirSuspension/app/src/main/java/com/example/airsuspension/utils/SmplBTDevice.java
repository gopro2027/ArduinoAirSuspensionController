package com.example.airsuspension.utils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public class SmplBTDevice {
    String mac;
    String name;
    public SmplBTDevice(String mac, String name) {
        this.mac = mac;
        this.name = name == null ? "" : name;
    }
    public String getMac() {
        return mac;
    }
    public String getName() {
        return name;
    }

    @NonNull
    @Override
    public String toString() {
        return getName() + " | " + getMac();
    }

    @Override
    public boolean equals(@Nullable Object obj) {
        if (obj instanceof SmplBTDevice) {
            return ((SmplBTDevice)obj).mac.equals(mac);
        }
        return false;
    }
}
