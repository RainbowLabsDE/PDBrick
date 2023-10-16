#include <Arduino.h>
#include <SPI.h>
#include <SSD1303.h>

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
    disp.print("\nSSD1303 OLED Test\n\nHello World!");
    disp.display();

    Serial.println("\nPDBrick Display");
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

uint16_t regs[256];

void updateDisp() {
    DPS::dpsValues_t stats = psu.getValues();

    printf("Input: %.2f V, %5.2f A / Output: %.3f V, %6.2f A / Intake: %.2f °C, Internal: %.2f °C, RPM: %5.0f\n", 
        stats.inV,
        stats.inA,
        stats.outV,
        stats.outA,
        stats.intakeTemp,
        stats.internTemp,
        stats.fanRpm
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
        stats.inV,
        stats.outV,
        stats.inA,
        stats.outA,
        stats.inW,
        stats.outW,
        stats.inV * stats.inA,
        stats.outV * stats.outA,
        stats.intakeTemp,
        stats.internTemp,
        stats.fanRpm);*/

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
        stats.inV,
        stats.outV,
        stats.outV * 2,
        stats.inA,
        stats.outA,
        stats.outA,
        stats.inW,
        stats.outW,
        stats.outW * 2,
        stats.inV * stats.inA,
        stats.outV * stats.outA,
        stats.outV * stats.outA * 2,
        stats.intakeTemp,
        stats.internTemp,
        stats.fanRpm,
        stats.inV * stats.inA * 2);

    disp.clearDisplay();
    disp.setCursor(0, 0);
    disp.print(buf);
    disp.display();
}

uint32_t lastUpdate = 0;
uint16_t rpm = 3000;

void loop() {
    if (millis() - lastUpdate >= 100) {
        lastUpdate = millis();
        updateDisp();
    }


    while (Serial.available()) {
        switch(Serial.read()) {
            case 'w': {
                rpm += 100;
                psu.setFanSpeed(rpm);
            }
            break;
            case 's': {
                rpm -= 100;
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

    // printf("         ");
	// for(uint8_t i = 0; i < 16; i++) {
	// 	printf("%1X    ", i);
	// }
	// printf("\n%04X  ", 0);
	// for(uint16_t i = 0; i < 0x60; i++) {
	// 	printf("%04X ", psu._read(i));
	// 	if(i % 16 == 15) {
	// 		printf("\n%04X  ", i+1);
	// 	}
    //     // delay(5);
	// }
	// printf("\n");

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
