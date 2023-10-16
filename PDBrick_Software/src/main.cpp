#include <Arduino.h>
#include <SPI.h>
#include <SSD1303.h>
#include <Fonts/Picopixel.h>        // better text
#include <Fonts/TomThumb.h>         // better numbers
#include <Fonts/Org_01.h>           // futuristic

#include "HP_DPS.h"

#define DISP_SCK    26
#define DISP_MOSI   18
#define DISP_RES    19
#define DISP_DC     23
#define DISP_CS     5

#define PSU_SCL     17
#define PSU_SDA     16
#define PSU_ADDR    0x5F

SSD1303 disp(128, 64, &SPI, DISP_DC, DISP_RES, DISP_CS);
DPS psu(PSU_ADDR);

DPS::dpsValues_t curStats;

void dispPrintCentered(int16_t xOffset, int16_t y, const char* text) {
    int16_t textX, textY;
    uint16_t textWidth, textHeight;
    disp.getTextBounds(text, 0, 0, &textX, &textY, &textWidth, &textHeight);
    int16_t x = disp.width() / 2 - textWidth / 2;
    disp.setCursor(x + xOffset, y);
    disp.print(text);
}

uint8_t dispScreen = 0;

void updateDisplay() {
    disp.clearDisplay();

    // Input watts were the most accurate < 200W (still complete guesswork tho)
    float watts = curStats.inV * curStats.inA * 2;  

    switch (dispScreen) {
        case 0: {   // Overview
            int16_t yPos = 6;
            // Header
            disp.setFont(NULL);
            disp.setTextSize(1);
            // disp.setTextColor(BLACK);
            // disp.fillRect(0, yPos - 2, disp.width(), 11, WHITE);
            dispPrintCentered(0, yPos, "PDBrick 2400");
            disp.drawFastHLine(0, yPos += 9, disp.width(), WHITE);
            yPos += 7;
            // yPos += 9;
            // disp.setTextColor(WHITE);

            // Watts
            disp.setTextSize(2);
            char buf[32];
            sprintf(buf, "%4.2f kW", watts / 1000);
            dispPrintCentered(0, yPos, buf);
            yPos += 16 + 2;

            // Bar graph
            int16_t barOffset = 6;
            disp.drawRoundRect(barOffset, yPos, disp.width() - 2 * barOffset, 8, 1, WHITE);
            int barMaxWidth = disp.width() - 2 * (barOffset + 2);
            int16_t barWidth = barMaxWidth * watts / 2400;
            disp.fillRect(barOffset + 2, yPos + 2, barWidth, 4, WHITE);

            // Footer
            disp.setFont(&Picopixel);
            disp.setTextSize(1);
            int16_t xPos = 7;
            yPos = disp.height() - 2;
            disp.setCursor(xPos, yPos);
            disp.printf("In: %4.1f'C", curStats.intakeTemp);
            disp.setCursor(xPos += 37, yPos);
            disp.printf("PCB: %4.1f'C", curStats.internTemp);
            disp.setCursor(xPos += 42, yPos);
            disp.printf("RPM: %5.0f", curStats.fanRpm);
            disp.drawFastHLine(0, yPos -= 7, disp.width(), WHITE);
        }
        break;

        case 1: {   // Detailed / debug view (with all values, like before)
            // TODO
        }
        break;

    }

    disp.display();
}


void setup() {
    Serial.begin(921600);
    SPI.begin(DISP_SCK, -1, DISP_MOSI, DISP_CS);
    Wire.begin(PSU_SDA, PSU_SCL, 400e3);

    disp.begin();

    // set brightness and clear display
    disp.setContrast(128);
    disp.clearDisplay();
    disp.display();

    // show welcome text (21 characters x 8 rows at default font and size 1)
    disp.setTextSize(1);
    disp.setTextColor(WHITE);
    disp.setCursor(0, 0);
    disp.print("\n  PDBrick Display\n\n  Hello World!");
    disp.display();

    Serial.println("\nPDBrick Display");
    psu.setFanSpeed(4500);  // Set idle fan speed a bit higher, so hopefully the case doesn't get as hot to the touch
}

// uint16_t regs[256];
uint32_t lastUpdate = 0;
uint16_t rpm = 3000;

void loop() {
    if (millis() - lastUpdate >= 100) {
        lastUpdate = millis();
        curStats = psu.getValues();
        // updateDisp();
        updateDisplay();
    }


    while (Serial.available()) {
        switch(Serial.read()) {
            case 'w': {
                rpm += 250;
                psu.setFanSpeed(rpm);
            }
            break;
            case 's': {
                rpm -= 250;
                if (rpm < 1000) {
                    rpm = 1000;
                }
                psu.setFanSpeed(rpm);
            }
            break;
        }
    }

    
    // i2c_scan();


    // printf("%d\n", psu._read(4));


    // for (int i = 0; i < 0x60; i++) {
    //     regs[i] = psu._read(i);
    // }
    // printHex(regs, 0x60);
}


void i2c_scan() {
    byte error, address;
    int nDevices;

    Serial.println("Scanning...");

    nDevices = 0;
    for (address = 1; address < 127; address++) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println("  !");

            nDevices++;
        } else if (error == 4) {
            Serial.print("Unknown error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("done\n");

    delay(5000); // wait 5 seconds for next scan
}


void printHex(uint16_t* buf, uint16_t size) {
	printf("         ");
	for(uint8_t i = 0; i < 16; i++) {
		printf("%1X    ", i);
	}
	printf("\n%04X  ", 0);
	for(uint16_t i = 0; i < size; i++) {
		printf("%04X ", buf[i]);
		if(i % 16 == 15) {
			printf("\n%04X  ", i+1);
		}
	}
	printf("\n");
}

void updateDisp() {

    printf("Input: %.2f V, %5.2f A / Output: %.3f V, %6.2f A / Intake: %.2f °C, Internal: %.2f °C, RPM: %5.0f\n", 
        curStats.inV,
        curStats.inA,
        curStats.outV,
        curStats.outA,
        curStats.intakeTemp,
        curStats.internTemp,
        curStats.fanRpm
    );



    char buf[256];
    /*sprintf(buf, 
        //"                     "
        "\n  Input   |  Output"
        "\n %6.2fV  | %6.3fV"
        "\n %6.3fA  | %6.2fA"
        "\n %6.1fW  | %6.1fW"
        "\n %6.1fVA | %6.1fVA"
        "\n  %5.2fC  |  %5.2fC"
        // "\n---------------------"
        "\n    RPM: %5.0f",
        // "\n < %6.2fV %6.3fA\n > %6.3fV %6.2fA\n 1: %5.2fC 2: %5.2fC\nRPM: %5.0f", 
        curStats.inV,
        curStats.outV,
        curStats.inA,
        curStats.outA,
        curStats.inW,
        curStats.outW,
        curStats.inV * curStats.inA,
        curStats.outV * curStats.outA,
        curStats.intakeTemp,
        curStats.internTemp,
        curStats.fanRpm);*/

    sprintf(buf, 
        //"                     "
        "\nInput |Output|  x2   "
        "\n%6.2f|%6.3f|%6.3fV"
        "\n%6.3f|%6.2f|%6.2fA"
        "\n%6.1f|%6.1f|%6.1fW"
        "\n%6.1f|%6.1f|%6.1fP"
        "\n %5.2f|    %5.2fC"
        // "\n---------------------"
        "\nRPM: %5.0f   |%6.1fW",
        // "\n < %6.2fV %6.3fA\n > %6.3fV %6.2fA\n 1: %5.2fC 2: %5.2fC\nRPM: %5.0f", 
        curStats.inV,
        curStats.outV,
        curStats.outV * 2,
        curStats.inA,
        curStats.outA,
        curStats.outA,
        curStats.inW,
        curStats.outW,
        curStats.outW * 2,
        curStats.inV * curStats.inA,
        curStats.outV * curStats.outA,
        curStats.outV * curStats.outA * 2,
        curStats.intakeTemp,
        curStats.internTemp,
        curStats.fanRpm,
        curStats.inV * curStats.inA * 2);

    disp.clearDisplay();
    disp.setCursor(0, 0);
    disp.print(buf);
    disp.display();
}
