package com.vividaesthetic.airsuspension.utils;

import androidx.annotation.NonNull;

public class FourPressure {
    public int fp, rp, fd, rd;
    public FourPressure(int fp, int rp, int fd, int rd) {
        this.fp = fp;
        this.rp = rp;
        this.fd = fd;
        this.rd = rd;
    }
    public FourPressure(String fp, String rp, String fd, String rd) throws NumberFormatException {
        this(Integer.parseInt(fp), Integer.parseInt(rp), Integer.parseInt(fd), Integer.parseInt(rd));
    }
    public int getFP() {
        return fp;
    }
    public int getRP() {
        return rp;
    }
    public int getFD() {
        return fd;
    }
    public int getRD() {
        return rd;
    }

    @NonNull
    @Override
    public String toString() {
        return "FP: "+fp+", RP: "+rp+", FD: "+fd+", RD: "+rd;
    }
}
