package com.vividaesthetic.airsuspension.utils;

import com.vividaesthetic.airsuspension.MainActivity;

import java.text.DecimalFormat;

public class PressureUnit {
    public enum Unit {
        PSI, // Base Unit
        BAR,
        PASCAL;
        public static Unit fromId(int i) {
            return Unit.values()[i];
        }
        public int getId() {
            return ordinal();
        }
    }

    private final Unit unit;
    private double val;
    public PressureUnit(Unit unit, double val) {
        this.unit = unit;
        this.val = val;
    }
    public PressureUnit(PressureUnit pu) {
        this.unit = pu.unit;
        this.val = pu.val;
    }

    // converts current unit to PSI
    public PressureUnit toBaseUnit() {
        PressureUnit baseUnit = new PressureUnit(Unit.PSI, 0);
        switch (this.unit) {
            case PSI:
                baseUnit.val = val;
                break;
            case BAR:
                baseUnit.val = val * 14.5038; // bar to psi
                break;
            case PASCAL:
                baseUnit.val = val / 6894.76; // pascall to psi
                break;
            default:
                break;
        }
        return baseUnit;
    }

    // converts psi to specified unit
    public PressureUnit toUnit(Unit unit) {
        // quick short if we are not converting anything, return a deep copy
        if (unit == this.unit) {
            return new PressureUnit(this);
        }
        PressureUnit newUnit = new PressureUnit(unit, 0);
        PressureUnit baseUnit = toBaseUnit();
        switch (unit) {
            case PSI:
                newUnit.val = baseUnit.val;
                break;
            case BAR:
                newUnit.val = baseUnit.val / 14.5038; // psi to bar
                break;
            case PASCAL:
                newUnit.val = baseUnit.val * 6894.76; // psi to pascall
                break;
            default:
                break;
        }
        return newUnit;
    }

    public double getVal() {
        return val;
    }

    public String forArduino() {
        return Math.round(toBaseUnit().val)+"";
    }

    public static String convertValueFromBaseUnitToDisplay(String received, Unit unitToDisplay) {
        DecimalFormat df = new DecimalFormat("#.#");
        //df.setRoundingMode(RoundingMode.);
        return df.format(new PressureUnit(PressureUnit.Unit.PSI, Integer.parseInt(received)).toUnit(unitToDisplay).getVal());
    }

    public static PressureUnit ofBaseUnit(int psi) {
        return new PressureUnit(Unit.PSI, psi);
    }
    public static PressureUnit ofPreferredUnit(MainActivity activity, double val) {
        return new PressureUnit(activity.preferredUnit, val);
    }
}
