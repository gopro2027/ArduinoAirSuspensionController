#include "screen.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void drawPSIReadings()
{
    display.clearDisplay();

    display.drawBitmap(0, 0, logo_bmp_corvette, 128, 20, 1);

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    int textHeightPx = 10;
    int secondRowXPos = SCREEN_WIDTH / 2;

    display.setCursor(0, 5 * textHeightPx + 5);
    display.print(F("Tank: "));
    display.print(int(getTankPressure()));

    // Front

    display.setCursor(0, 2 * textHeightPx + 5);
    display.print(F("FD: "));
    display.print(int(getWheel(WHEEL_FRONT_DRIVER)->getPressure())); // front driver

    display.setCursor(secondRowXPos, 2 * textHeightPx + 5);
    display.print(F("FP: "));
    display.print(int(getWheel(WHEEL_FRONT_PASSENGER)->getPressure())); // front passenger

    // Rear
    display.setCursor(0, 3.5 * textHeightPx + 5);
    display.print(F("RD: "));
    display.print(int(getWheel(WHEEL_REAR_DRIVER)->getPressure())); // rear driver

    display.setCursor(secondRowXPos, 3.5 * textHeightPx + 5);
    display.print(F("RP: "));
    display.print(int(getWheel(WHEEL_REAR_PASSENGER)->getPressure())); // rear passenger

    display.display();
}

void drawsplashscreen()
{
    for (int i = -display.height(); i <= 0; i += 2)
    {
        display.clearDisplay();

        display.drawBitmap(0, i, logo_bmp_airtekk, 128, 64, 1);
        display.display();
        delay(1); // 1 ms
    }
    display.clearDisplay();

    display.drawBitmap(0, 0, logo_bmp_airtekk, 128, 64, 1);
    display.display();
    delay(2000); // 2 seconds
}
