#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "Xmega_training_8bit11k.h"
#include "../dac_driver.h"

// The board.h header file defines which IO ports peripherals like
// Switches and LEDs are connected to. The header file is configured
// for use with XMEGA-A1 Xplained.
#include "../board.h"

/*                            Task 2
 *
 *
 */

#define END_ADDRESS 11200

// 2MHz/11025Hz gives the period of 181 CK
#define TIMER_C0_PERIOD 181



int main( void )
{
	// First we have to enable the audio amplifier by setting PQ3 high.
	PORTQ.PIN3CTRL = (PORTQ.PIN3CTRL & ~PORT_OPC_gm) | PORT_OPC_PULLUP_gc;

	// Configure switches
	SWITCHPORTL.DIRCLR = 0xff; // Set port as input
    // Configure all keys to be active when pressed (inverted).
    PORTCFG.MPCMASK = 0xFF;
	SWITCHPORTL.PIN0CTRL = (SWITCHPORTL.PIN0CTRL & ~PORT_OPC_gm) | PORT_OPC_PULLUP_gc; //Enable pull-up to get a defined level on the switches
    PORTCFG.MPCMASK = 0xFF;
    SWITCHPORTL.PIN0CTRL |= PORT_INVEN_bm;

    // Setup Timer/Counter 0

    // Dividing the Main clock by 1 gives 2MHz
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;
    // Enable overflow interrupt.
    TCC0.INTCTRLA = ( TCC0.INTCTRLA & TC0_OVFINTLVL_gm ) | TC_OVFINTLVL_MED_gc;

    // Enable low level interrupt in PMIC and enable global interrupts.
	PMIC.CTRL |= PMIC_MEDLVLEN_bm;

    // Setting up the DAC Single Conversion mode.

   DAC_SingleChannel_Enable(  &DACB, DAC_REFSEL_AVCC_gc, DAC_LEFTADJ_bm );

    // Enable global interrupts
    sei();

	while (1)
    {

        if(SWITCHPORTL.IN == 0x00)
        {
            // No sound!
            TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_OFF_gc;
        }
        else
        {
            // Enable Timer C0, prescaler div1 means Main Clock (2MHz)
            TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV1_gc;

    	    if(SWITCHPORTL.IN & PIN0_bm)
            {
                // Normal speed
                TCC0.PER = TIMER_C0_PERIOD;
            }
            else if(SWITCHPORTL.IN & PIN1_bm)
            {
                // Decrease speed
                TCC0.PER = TIMER_C0_PERIOD + 50;
            }
            else if(SWITCHPORTL.IN & PIN2_bm)
            {
                // Increase speed
                TCC0.PER = TIMER_C0_PERIOD - 50;
            }
        }
    }
}




// Timer/Counter C0 Overflow Vector ISR
ISR(TCC0_OVF_vect)
{
    static uint16_t Address = 0;
    uint16_t Sample;

    // Getting a sample from Program Memory
    Sample = pgm_read_byte( &(mydata[Address]) );

    // The DAC data register is left adjusted and
    // we only want to write the high-byte.

	Sample = Sample << 5;	// We don't shift up by more than 5, so as to reduce volume
							// For max volume, shift left by 8. For min; 4


    // Requesting a DAC write

    DAC_Channel_Write( &DACB, Sample, CH0 );

    Address++;

    if(Address >= END_ADDRESS)
    {
        // Repeat playing!
        Address = 0;
    }
}

