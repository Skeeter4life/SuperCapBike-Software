/*
 * ATMEGA328_SuperCapBike_Firmware.c
 *
 * Created: 2024-09-20 11:38:06 AM
 * Author : Andrew Fischer
 */ 

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#define F_CPU 16000000

// VOLATILE DEFN'S: Compiler assumes variables do not changed unless they are explicitly modified within the program; this is not the case here; Must always read from memory
volatile uint16_t ADC_DATA_0; // Throttle Data
volatile uint16_t ADC_DATA_1; // C1 Voltage
volatile uint16_t ADC_DATA_2; // VCC Voltage

volatile uint8_t ADC_Selection = 0; // 0 = ADC 0 1 = ADC 1 2 = ADC 2

struct Diagnostics{
	bool is_Seires; // True if capacitors are in series False if in parallel
	
	uint8_t VC1; // Voltage of Capacitor 1
	uint8_t mVC1; // mV of Capacitor 1

	uint8_t VC2;  // Voltage of Capacitor 2
	uint8_t mVC2; // mV of Capacitor 2

	uint8_t VCC; // VCC Volts
	uint8_t mVCC; // VCC miliVolts

	uint8_t Throttle_Position; // Range of {0 1, 2 ... 100} (% Applied)

	bool is_ShutDown;
	char Fault_Code[16];
}Bike_Status;


void Compute_Voltages(){
	uint8_t mV;
	uint8_t V;

	//Compute C1 First:

	mV = ((ADC_DATA_1 * 5)/1024) * 1000 * (80/25); // ADC = (VIN*1024)/VREF (80/25 Voltage divider)
	V = (mV/1000);

	mV -= V * 1000;

	Bike_Status.mVC1 = mV;
	Bike_Status.VC1 = V;

	// Compute VCC:
	if(Bike_Status.is_Seires){
		mV = ((ADC_DATA_2 * 5)/1024) * 1000 * (23704/3704);
		V = (mV/1000);

		mV -= V * 1000;

		if(Bike_Status.mVC1 > mV){
			Bike_Status.mVC2 = 1000 - (Bike_Status.mVC1 - mV);
			}else{
			Bike_Status.mVC2 = mV - Bike_Status.mVC1;
		}
		Bike_Status.mVC2 = V - Bike_Status.VC1;

		}else{
		mV = (ADC_DATA_2 * 1024/5) * 1000 * (9091/29091);
		V = (mV/1000);

		mV -= V * 1000;
	}

	Bike_Status.mVCC = mV;
	Bike_Status.VCC = V;

}

void Compute_Throttle(){

}

ISR(ADC_vect){

	uint16_t Reg0_7 = ADCL; // Read the ADCL registers first
	uint16_t Reg8_9 = ADCH; // Read ADCH last, setting ADSC LOW, allowing writing to the ADC

	if(ADC_Selection == 0){ // ADC 0 - Handles THROTTLE information
		ADC_DATA_0 = Reg0_7 | (Reg8_9 << 8); // Combine the results from the ADC
		ADMUX |= (1 << MUX0); // Switch to ADC 1
		ADC_Selection = 1;
		Compute_Throttle();
		
		}else if (ADC_Selection == 1) { // ADC 1 Capacitor 1 Voltage
		ADC_DATA_1 = Reg0_7 | (Reg8_9 << 8);
		ADMUX = (ADMUX & ~(1 << MUX0)) | (1 << MUX1); // Set MUX0 to 0 and Set MUX1 to 1, Switch to ADC 2
		ADC_Selection = 2;

		}else if (ADC_Selection == 2){ // ADC 2 +VCC Voltage
		ADMUX &= ~(1 << MUX1); // Set MUX 1 to 0, Switch to ADC 0
		ADC_DATA_2 = Reg0_7 | (Reg8_9 << 8);
		ADC_Selection = 0;
		Compute_Voltages();

		}else{
		return;
	}

	if((int)ADC_DATA_0 > 400){
		PORTB |= (1 << PORTB0); // Set B0 HIGH
		}else{
		PORTB  &= ~(1 << PORTB0); // Inverts to 0b11111110 and uses logical AND for each bit to maintain data (Set B0 LOW)
	}
	
	ADCSRA |= (1 << ADSC); // Start the next conversion

}

void init_ADC(){
	PRR &= ~(1 << PRADC); // Set the Power Reduction ADC bit to logic LOW
	ADMUX = (1 << REFS0); // Set DAC AREF to AVCC, Right shifted, ADC0
	ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | ( 1 << ADPS1 ) | (1 << ADPS0);// Enable ADC Auto Trigger, Enable ADC Complete Interupt, Division Factor of 128 = 125kHz ADC
}

int main(void){
	// ** Read state of series parallel switch!!
	
	sei(); // Enable interrupts
	init_ADC();
	ADCSRA |= (1 << ADSC); // Start the ADC

	DDRB |= (1 << DDB0); // Set B0 as an output pin
	DDRD = (1 << DDD4) | (1 << DDD7); // Set D4 and D7 as an output pin

	while(1){
		
	}

}

