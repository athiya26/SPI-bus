#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "spi0.h"
#include "gpio.h"
#include "nvic.h"
#include "wait.h"
#include "clock.h"

// Pins
#define SSI0TX PORTA, 5  // PA5
#define SSI0RX PORTA, 4  // PA4
#define SSI0FSS PORTA, 3 // PA3
#define SSI0CLK PORTA, 2 // PA2
#define INTPIN PORTA, 6  // PA6
#define PUSH_BUTTON 0x20 // Pushbutton Mask

void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Initialize SPI0 interface
    initSpi0(USE_SSI_RX);
    setSpi0BaudRate(2e6, 40e6);
    setSpi0Mode(1, 1);

    enablePort(PORTA);
    selectPinPushPullOutput(SSI0FSS);

    // Initialize GPIO for button (input with pull-up)
    selectPinDigitalInput(INTPIN);

    // Configure GPIO interrupt on button press (rising edge)
    selectPinInterruptRisingEdge(INTPIN);
    enablePinInterrupt(INTPIN);

    // CS is idiling high - only when we communicate we pull it low
    // setPinValue(SSI0FSS, 1);
}

// Function that writes data using SPI
void writeToSpi(uint8_t devAddress, uint8_t regAddress, uint8_t data)
{

    // waitMicrosecond(1000);

    // Activate the device by lowering the chip select line
    setPinValue(SSI0FSS, 0);

    // SPI write command opcode
    writeSpi0Data(devAddress);

    // Dummy read
    readSpi0Data();

    // Register address to write
    writeSpi0Data(regAddress);

    // Dummy read
    readSpi0Data();

    // Send data to set the register
    writeSpi0Data(data);

    // Dummy read
    readSpi0Data();

    // End SPI communication by raising the chip select line
    setPinValue(SSI0FSS, 1);

    // waitMicrosecond(1000);
}

uint8_t readFromSpi(uint8_t deviceAddress, uint8_t registorAddress)
{
    uint8_t data;

    setPinValue(SSI0FSS, 0);
    writeSpi0Data(deviceAddress);
    readSpi0Data();
    writeSpi0Data(registorAddress);
    readSpi0Data();
    writeSpi0Data(0x00);
    data = readSpi0Data();
    setPinValue(SSI0FSS, 1);

    return data;
}

void waitPbPress()
{
    // Wait for the push button to be pressed
    while (readFromSpi(0x41, 0x09) & PUSH_BUTTON)
    {
        // Continue looping until the push button is pressed
    }

    // When push button is pressed, turn on green LED
    writeToSpi(0x40, 0x09, 0x02);
}

void readBusExpanderISR()
{
    // When push button is pressed, turn on green LED
    writeToSpi(0x40, 0x09, 0x02);

    // Clear the interrupt flag - INTCAP
    writeToSpi(0x41, 0x08, 0x20);

    // Clear the pin-level interrupt on INTPIN
    clearPinInterrupt(INTPIN);
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    // Initialize hardware
    initHw();

    // Setting Pushbutton (GPO5) as an input - IODIR
    // 00100000
    writeToSpi(0x40, 0x00, 0x20);

    // Enable pull-up for the pushbutton using - GPPU
    writeToSpi(0x40, 0x06, 0x20);

    // Turn on red LED - GPIO
    writeToSpi(0x40, 0x09, 0x04);

    waitPbPress();

    // Turn on green LED and turn off red LED
    writeToSpi(0x40, 0x09, 0x02);

    // Wait for 1 second
    waitMicrosecond(1000000);

    // Turn on red LED - GPIO
    writeToSpi(0x40, 0x09, 0x04);

    // waitMicrosecond(500000);

    // disableNvicInterrupt(INT_GPIOA);

    while (readFromSpi(0x41, 0x09) & PUSH_BUTTON)
    {
        // Continue looping until the push button is pressed
    }

    // Enable interrupt on Pushbutton (GPO5) - GPINTEN
    writeToSpi(0x40, 0x02, 0x20);

    // Enable interrupt on Pushbutton (GPO5) - DEFVAL
    writeToSpi(0x40, 0x03, 0x20);

    // Enable interrupt on Pushbutton (GPO5) - INTCON
    writeToSpi(0x40, 0x04, 0x20);

    // Setting polarity of INT output pin as high - IOCON INTPOL
    writeToSpi(0x40, 0x05, 0x02);

    // Clear the interrupt PB - INTCAP
    writeToSpi(0x41, 0x08, 0x20);

    // Clear the pin-level interrupt on INTPIN
    clearPinInterrupt(INTPIN);

    enableNvicInterrupt(INT_GPIOA);

    // Endless loop
    while (true)
        ;
}
