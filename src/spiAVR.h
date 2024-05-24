#ifndef SPIAVR_H
#define SPIAVR_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>


//B5 should always be SCK(spi clock) and B3 should always be MOSI. If you are using an
//SPI peripheral that sends data back to the arduino, you will need to use B4 as the MISO pin.
//The SS pin can be any digital pin on the arduino. Right before sending an 8 bit value with
//the SPI_SEND() funtion, you will need to set your SS pin to low. If you have multiple SPI
//devices, they will share the SCK, MOSI and MISO pins but should have different SS pins.
//To send a value to a specific device, set it's SS pin to low and all other SS pins to high.

// Outputs, pin definitions
#define PIN_SCK                   PORTB5//SHOULD ALWAYS BE B5 ON THE ARDUINO
#define PIN_MOSI                  PORTB3//SHOULD ALWAYS BE B3 ON THE ARDUINO
#define PIN_SS                    PORTB2


//If SS is on a different port, make sure to change the init to take that into account.
void SPI_INIT(){
    DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS);//initialize your pins. 
    SPCR |= (1 << SPE) | (1 << MSTR); //initialize SPI coomunication
}


void SPI_SEND(char data)
{
    SPDR = data;//set data that you want to transmit
    while (!(SPSR & (1 << SPIF)));// wait until done transmitting
}

void SPI_SEND_COL(unsigned char col, unsigned char data)
{
    PORTB &= ~(1 << PIN_SS); // Set SS low
    SPI_SEND(col);
    SPI_SEND(data);
    PORTB |= (1 << PIN_SS); // Set SS high
}

void MAX7219_init()
{
    SPI_SEND_COL(0x09, 0x00); // Decode mode: no decode for any digit
    SPI_SEND_COL(0x0A, 0x0F); // Intensity: set to maximum (0x00 to 0x0F)
    SPI_SEND_COL(0x0B, 0x07); // Scan limit: display all rows/digits
    SPI_SEND_COL(0x0C, 0x01); // Shutdown register: normal operation
    SPI_SEND_COL(0x0F, 0x00); // Display test: off
}

#endif /* SPIAVR_H */