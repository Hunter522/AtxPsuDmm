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
 *   \author Hunter N. Morgan (hunter.nelson.morgan@gmail.com)
 */

#include <Arduino.h>
#include "LedControl.h"

#define DISPLAY_ADDRESS 0  // SPI address of MAX72XX chip
#define SAMPLE_CNT 250     // number of samples to read from analog input
#define VOLT_AIN_PIN A0    // analog in pin for measuring voltage
#define AMP_AIN_PIN A1     // analog in pin for measuring current
#define MOSI_PIN 11	   // SPI MOSI pin
#define SS_PIN 10          // SPI SS pin
#define SCK_PIN 13         // SPI SCK pin
#define VOLT_SCALE 60.0f   // voltage scaler for calculating true voltage
#define CURRENT_SCALE 0.20f// current scaler for calculating true current

//#define VERBOSE

#ifdef VERBOSE
 #define LOG(x) Serial.print(x)
 #define LOGLN(x) Serial.println(x)
#else
 #define LOG(x)
 #define LOGLN(x) 
#endif // VERBOSE

// Global variables
LedControl display = LedControl(MOSI_PIN, SCK_PIN, SS_PIN, 1);

/**
 *  \brief displays floating point number on 4 x 7-segment display,  right-justified
 *
 *  e.g. 12.450 ---> _ _ 1 2.4 5
 * 
 *  \param val the floating point num to display
 *  \param offset the offset in number of digits to start displaying at (use 4 to display on 2nd line)
 */
void displayFloat(const float val, const int offset) {
    char str[32];
    const int valLen = strlen(dtostrf(val, 4, 2, str)); // use avr libc's dtostrf since sprtintf doesn't support %f...

    if(valLen > 0 && valLen <= 5) {    
	// start off at right-most digit,  walking through string backwards and setting
	// the corresponding digit on the display
	int digit = offset;
	int valInt = 0;
	bool setDecimal = false;
	for(int i = valLen-1; i >= 0; i--) {
	    // set digit on display
	    // if '.' then set display decimal visible
	    if(str[i] == '.') {
		setDecimal = true;
	    } else {
		valInt = str[i] - '0'; // convert char to int
		display.setDigit(DISPLAY_ADDRESS, digit, (byte) valInt, setDecimal);
		digit++;
		setDecimal = false;
	    }
	}
    }
}

/**
 *  \brief Performs an analog read using a median-filter method
 *
 *  \param pin the analog pin to read from
 *  \param numSamples the number of samples to use when reading (100 is good)
 *  \return the filtered anlog value [0, 1023] 
 */
float analogReadMedianFiltered(const int pin, const int numSamples) {
    // read multiple values and sort them to take the mode
    int sortedValues[numSamples];
    for(int i = 0; i < numSamples; i++) {
	int value = analogRead(pin);
	int j;
	if(value < sortedValues[0] || i == 0) {
	    j = 0; //insert at first position
	}
	else{
	    for(j = 1; j < i; j++) {
		if(sortedValues[j-1]<=value && sortedValues[j]>=value){
		    // j is insert position
		    break;
		}
	    }
	}
	for(int k = i; k > j; k--) {
	    // move all values higher than current reading up one position
	    sortedValues[k] = sortedValues[k-1];
	}
	sortedValues[j] = value; //insert current reading
    }
    //return scaled mode of 10 values
    float returnval = 0;
    for(int i = numSamples/2-5; i < (numSamples/2+5); i++) {
	returnval += sortedValues[i];
    }
    returnval = returnval/10;
    return returnval;
}

void setup() {
    // initialize the MAX72XX chip
    display.shutdown(DISPLAY_ADDRESS, false);  // turns display on since chip is in power savings mode on boot
    display.setIntensity(DISPLAY_ADDRESS, 5);  // set reasonable brightness
    display.clearDisplay(DISPLAY_ADDRESS);
    
#ifdef VERBOSE    
    Serial.begin(9600);    
#endif // VERBOSE
}

void loop() {

    // read analog values from voltage and current pins using a median filter method
    const float voltAvg = analogReadMedianFiltered(VOLT_AIN_PIN, SAMPLE_CNT);
    const float scaledVoltAvg = VOLT_SCALE * (voltAvg / 1023.0f);
    LOG("voltAvg: ");
    LOGLN(voltAvg);

    // calculate current using Ohm's law, I = V/R
    // R_shunt = 0.1
    // current is measured from analog input
    // analog signal is sourced from a AD623 op-amp which has a gain of aprox. 5
    // so that we can get aprox 1mA resolution (otherwise we'd get only 5mA resolution)
    // assuming we are using a 10-bit ADC and only care about max current of 10.0A
    const float shuntVoltAvg = analogReadMedianFiltered(AMP_AIN_PIN, SAMPLE_CNT);
    const float scaledCurrentAvg = (CURRENT_SCALE * (5.0f * (shuntVoltAvg / 1023.0f))) / 0.1f;
    LOG("shuntVoltAvg: ");
    LOGLN(shuntVoltAvg);
    
    // display the values
    display.clearDisplay(DISPLAY_ADDRESS);
    displayFloat(scaledVoltAvg, 0);
    displayFloat(scaledCurrentAvg, 4);
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
