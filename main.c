/*
 * ATMEGA328_SuperCapBike_Firmware.c
 *
 * Created: 2024-09-20 11:38:06 AM
 * Author : Andrew
 */ 

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#define F_CPU 16000000
#define SHIFT_AMOUNT 8
#define SHIFT_MASK ((1 << SHIFT_AMOUNT) - 1)

// VOLATILE DEFN'S: Compiler assumes variables do not changed unless they are explicitly modified within the program; this is not the case here; Must always read from memory
volatile uint16_t ADC_DATA_0; // Throttle Data
volatile uint16_t ADC_DATA_1; // C1 Voltage
volatile uint16_t ADC_DATA_2; // VCC Voltage
volatile uint8_t ADC_Selection = 0; // 0 = ADC 0 1 = ADC 1 2 = ADC 2

// Note: I prefer returning '0' when a process has failed, and '1' when it succeeds.

struct Diagnostics{
	
	uint8_t VC1; // Voltage of Capacitor 1
	uint8_t mVC1; // mV of Capacitor 1

	uint8_t VC2;  // Voltage of Capacitor 2
	uint8_t mVC2; // mV of Capacitor 2

	uint8_t Throttle_Position; // Range of {0 1, 2 ... 100} (% Applied)

	uint8_t Fault_Code;
	
	bool is_ShutDown;
	bool is_Series; // True if capacitors are in series False if in parallel
	///char Fault_Code[16];
}Bike_Status;


void Compute_Voltages(){ // Uses fixed point arithmetic

	uint32_t VC1_Equation = (((ADC_DATA_1 << SHIFT_AMOUNT)<<4)/1023);
	uint32_t VCC_Equation;

	uint32_t VC1;
	uint32_t mVC1; // I use these for a couple calculations. It intuitively seems faster to make a local copy for this subroutine, rather then going to Bike_Status each time.
	

	VC1 = VC1_Equation >> SHIFT_AMOUNT;
	mVC1 = (((VC1_Equation & SHIFT_MASK) * 100)/(1 << SHIFT_AMOUNT));


	if(Bike_Status.is_Series){
		VCC_Equation = (((ADC_DATA_2 << SHIFT_AMOUNT) * 391)/12500);

		Bike_Status.VC2 = (VCC_Equation >> SHIFT_AMOUNT) - VC1;
		Bike_Status.mVC2 = (((VCC_Equation & SHIFT_MASK) * 100)/(1 << SHIFT_AMOUNT)) - mVC1;
		
		}else{
		VCC_Equation = (((ADC_DATA_2 << SHIFT_AMOUNT)<<4)/1023);

		Bike_Status.VC2 = (VCC_Equation >> SHIFT_AMOUNT);
		Bike_Status.VC2 = (((VCC_Equation & SHIFT_MASK) * 100)/(1 << SHIFT_AMOUNT));
		
	}

	Bike_Status.VC1 = VC1;
	Bike_Status.mVC1 = mVC1;
	
	
}

void Compute_Throttle(){
}

int8_t Transmit_Data(uint8_t DATA){
	TWCR = (1 << TWINT) &~(1<<TWSTA) &~(1<<TWSTO) | (1<<TWEN); // Reset the interrupt flag, clear start bit, ensure stop bit is cleared, ensure TWI is enabled in the control register
	TWDR = DATA;
	while(TWSR &= ~(1<<TWINT));  // Wait for the interrupt flag to be reset
	if(TWSR == 0x30){
		// Error, NOT ACK
		return 0;
	}

	return 1;

}

void TWI_Stop(){
	TWCR = (1 << TWINT) &~ (1 << TWSTA) | (1 << TWSTO) | (1<<TWEN); // Clear interrupt flag, set start bit, ensure stop register is cleared, and enable TWI to the control register
}


int8_t Slave_Write(uint8_t SLA_W){
	TWCR = (1 << TWINT) &~(1<<TWSTA) &~(1<<TWSTO) | (1<<TWEN); // Reset the interrupt flag, clear start bit, ensure stop bit is cleared, ensure TWI is enabled in the control register
	TWDR = (SLA_W << 1); // Write to the data register the adress of the MCP23017
	while(TWSR &= ~(1<<TWINT));  // Wait for the interrupt flag to be reset
	if(TWSR == 0x20){
		//Error, slave did not acklowadge adress was received! Attempt 3 more times.
		return 0;
	}

	return 1;
}

int8_t TWI_Start(){
	TWCR = (1 << TWINT) | (1 << TWSTA) &~(1 << TWSTO) | (1<<TWEN); // Clear interrupt flag, set start bit, ensure stop register is cleared, and enable TWI to the control register
	while(TWCR &= ~(1<<TWINT)); // Wait for the interrupt flag to be reset
	if(TWSR != 0xF8){
		// Error, start bit not successfully sent!
		return 0;
	}

	return 1;
}

void MCP_GPIO_Handler(uint8_t Register_Adress, uint8_t Data){
	uint8_t MCP23017_Adress = 0x20;

	uint8_t Start = TWI_Start(); // I sacrified a few bytes here for readability.
	uint8_t Write;
	uint8_t Received;

	if(Start){
		Write = Slave_Write(MCP23017_Adress);
	}
	if(Write){
		Received = Transmit_Data(Register_Adress);
		Received = Transmit_Data(Data);
	}
	TWI_Stop();
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
	// SREG |= (1 << SREG_I);  // Sets the global interrupt enable bit in SREG

	MCP_GPIO_Handler(0x12, 0b01111111); // Adress GPIOA, configure all pins except IO 7 as input
	
	init_ADC();

	sei(); // Enable global interrupts
	// cli(); for error handling

	ADCSRA |= (1 << ADSC); // Start the ADC

	DDRB |= (1 << DDB0); // Set B0 as an output pin
	DDRD = (1 << DDD4) | (1 << DDD7); // Set D4 and D7 as an output pin

	while(1){
		
	}

}


