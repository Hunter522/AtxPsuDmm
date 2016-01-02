/**
 *   \file main.cpp
 *   \brief Main program file
 *
 *  Main file for DIY ATX Bench PSU. This project uses the Anarduino Mini board
 *  which is basically an Arduino Pro Mini clone. The main difference between the boards
 *  is the FTDI header arrangement which is flipped. Here's a useful ASCII pinout diagram:
 *
 *          RST   D1   D0
 *          /DTR  TX   RX   VCC  GND  GND
 *        +--------------------------------+
 *        |  [ ]  [ ]  [ ]  [ ]  [ ]  [ ]  |
 *        |              FTDI              |
 *    D1  | [ ]1/TX                 RAW[ ] |    
 *    D0  | [ ]0/RX                 GND[ ] |    
 *        | [ ]RST        SCL/A5[ ] RST[ ] |   C6
 *        | [ ]GND        SDA/A4[ ] VCC[ ] |    
 *    D2  | [ ]2/INT0    ___         A3[ ] |   C3
 *    D3  |~[ ]3/INT1   /   \        A2[ ] |   C2
 *    D4  | [ ]4      Anarduino      A1[ ] |   C1
 *    D5  |~[ ]5       \MINI /       A0[ ] |   C0
 *    D6  |~[ ]6        \___/    SCK/13[ ] |   B5
 *    D7  | [ ]7          A7[ ] MISO/12[ ] |   B4
 *    B0  | [ ]8          A6[ ] MOSI/11[ ]~|   B3
 *    B1  |~[ ]9                  SS/10[ ]~|   B2
 *        |           [RST-BTN]            |    
 *        +--------------------------------+  
 *
 *       http://busyducks.com/ascii-art-arduinos
 *
 * \author Hunter N. Morgan (hunter.nelson.morgan@gmail.com)
 */

#include <Arduino.h>
#include "LedControl.h"

#define DISPLAY_ADDRESS 0  // SPI address of MAX72XX chip
#define SAMPLE_CNT 5       // number of samples to read from analog input
#define VOLT_AIN_PIN A0    // analog in pin for measuring voltage
#define AMP_AIN_PIN A1     // analog in pin for measuring current
#define MOSI_PIN 11	   // SPI MOSI pin
#define SS_PIN 10          // SPI SS pin
#define SCK_PIN 13         // SPI SCK pin

#define DEBUG

#ifdef DEBUG
 #define LOG(x) Serial.print(x)
#endif // DEBUG

LedControl display = LedControl(MOSI_PIN, SCK_PIN, SS_PIN, 1);

/**
 *  \brief displays floating point number on 4 x 7-segment display,  right-justified
 *
 *  e.g. 12.450 ---> _ _ 1 2.4 5
 * 
 *  \param f the floating point num to display
 */
void displayFloat(const float val) {
    char str[32];
    const int valLen = sprintf(str, "%.2f", val);

    if(valLen > 0 && valLen <= 4) {
	// clear display first
	display.clearDisplay(DISPLAY_ADDRESS);
    
	// start off at right-most digit,  walking through string backwards and setting
	// the corresponding digit on the display
	int digit = 3;
	int valInt = 0;
	for(int i = valLen-1; i >= 0; i--) {
	    // set digit on display
	    // if '.' then set display decimal visible
	    if(str[i] == '.') {
		display.setDigit(DISPLAY_ADDRESS, digit, (byte) valInt, true);
	    } else {
		valInt = str[i] - '0'; // convert char to int
		display.setDigit(DISPLAY_ADDRESS, digit, (byte) valInt, false);
		digit--;	    
	    }
	}
    }
}

void setup() {
    // initialize the MAX72XX chip
    display.shutdown(DISPLAY_ADDRESS, false);  // turns display on since chip is in power savings mode on boot
    display.setIntensity(DISPLAY_ADDRESS, 8);  // set reasonable brightness
    display.clearDisplay(DISPLAY_ADDRESS);
}

void loop() {
    // VOLTMETER
    // read analog pin value n times and average it
    int voltSample = 0;
    for (int i = 0; i < SAMPLE_CNT; i++) {
	voltSample += analogRead(VOLT_AIN_PIN);
    }
    const float voltAvg = (float)voltSample / (float)SAMPLE_CNT;
    LOG("Voltage: ");
    LOG(voltAvg);
    displayFloat(voltAvg);

    
    // AMMETER
    // read analog pin value n times and average it
    int currentSample = 0;
    for (int i = 0; i < SAMPLE_CNT; i++) {
	currentSample += analogRead(VOLT_AIN_PIN);
    }
    const float currentAvg = (float)currentSample / (float)SAMPLE_CNT;
    LOG("Current: ");
    LOG(currentAvg);
    displayFloat(currentAvg);
}




// Main function derived from Arduino's main() 
int main(void)
{
    init();

#if defined(USBCON)
    USBDevice.attach();
#endif

    setup();

    for (;;) {
        loop();
        if (serialEventRun) serialEventRun();
    }

    return 0;
}
