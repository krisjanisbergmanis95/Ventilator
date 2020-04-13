/*
 * 2020.02.26_7_segment_display_DEMO.c
 *
 * Created: 3/2/2020 3:26:10 AM
 * Author : 
 */ 


#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
uint8_t display_values [6] = {0, 0, 0, 0, 0, 0}; // going around - filling up
uint8_t digits [10] = {0b01111110, 0b00001100, 0b10110110, 0b10011110, 0b11001100,
	0b11011010, 0b11111010, 0b00001110, 0b11111110, 0b11011110}; // 7 segment display digits
uint8_t actual_display_digits [4] = {0b0000001, 0b00000010, 0b00000100, 0b00010000};
uint8_t display_value=0, array_offset = 0, actual_display_index = 0, USARTReadBuffer = 0; 
char buff[8] = {};

void PortInit();
void DisplaySeconds();
void USARTInit();
void Init_TC1_MM_SS();
void Init_TC0_MULTIPLEX();
void Init_TC2_MOTOR();
void UpdadeSeconds();
void ModeCheck();
void countTime();
void Init_ADC();
void ADC_Read();

int main(void)
{
	PortInit();
	USARTInit();
	Init_TC1_MM_SS();
	Init_TC0_MULTIPLEX();
	Init_ADC();
	Init_TC2_MOTOR();
	
	sei ();
	
	while (1)
	{
		ModeCheck();
		//PORTD = 0b00001110;	
		//PORTB = 0b00001001;
	}
}

/*-------------INIT----------------*/
void PortInit()
{
	DDRD = 0xFF;
	DDRB = 0x2F;
}

void Init_TC1_MM_SS(){
	TCNT1 = 0; //set timer counter 1 to 0
	//set output compare to 128;
	//frevenc = CLK/(OCR1A*(1+prescaler)) = 16000000/1024
	OCR1A = 15625;//15625
	//enable OUTPUT COMPARE MATCH A in TIMSK1 reg
	TIMSK1 = (1 << OCIE1A);
	//TC1 for fast PWM
	//SET waveform generators in  timer/counter control registers
	TCCR1A = ( 1 <<WGM11) | (1<< WGM10);
	TCCR1B = ( 1 <<WGM13) | (1<< WGM12)| (1<< CS12) | (0 << CS11) | (1<< CS10);
	//SET CLK /1024
	//TCCR1B = (1<< CS12) | (0 << CS11) | (1<< CS10);
}

void Init_TC0_MULTIPLEX() {
	TCNT0 = 0; //set timer counter 1 to 0
	OCR0A = 33;//15625;//8000;//15625
	//enable OUTPUT COMPARE MATCH A in TIMSK1 reg
	TIMSK0 = (1 << OCIE0A);
	//TC1 for fast PWM
	//SET waveform generators in  timer/counter control registers
	TCCR0A = ( 1 <<WGM01) | (1<< WGM00);
	TCCR0B = (1<< WGM02) | (1<< CS02) | (1<< CS00);
	//SET CLK /1024
	//TCCR0B = (1<< CS02) | (1<< CS00);
}

 void Init_TC2_MOTOR() {
	TCNT2 = 0; //set timer counter 1 to 0
 	OCR2A = 3003;//15625;//8000;//15625
 	//enable OUTPUT COMPARE MATCH A in TIMSK1 reg
 	TIMSK2 = (1 << OCIE2A);
 	//TC1 for fast PWM
 	//SET waveform generators in  timer/counter control registers
 	TCCR2A = ( 1 <<WGM21) | (1<< WGM20);
 	TCCR2B = (1<< WGM22) | (1<< CS22) | (1<< CS20);
 	//SET CLK /1024
 	//TCCR0B = (1<< CS02) | (1<< CS00);
 }

void USARTInit() {
	// 	datu biti: 8
	// STOP biti: 1
	// paritāte: nav
	// baud rate: 9600
	UBRR0 = 103; //9600
	UCSR0B = (1 << RXEN0) | (1 << RXCIE0); //enable reciever and receive interupt
	UCSR0C = (1 << UCSZ00) | (1 <<  UCSZ01); //8 data bits
}

void Init_ADC() {
	/*
	reģistrā ADMUX kā sprieguma referenci izvēlēties AVCC with external capacitor at AREF pin
	reģistrā ADCSRA biti ADPS0 - ADPS2 dalīšanas faktoram 128
	reģistrā ADCSRA jāiestata bits ADEN un ADCSR
	ADC Auto Trigger Source - free running mode ADCSRB.ADTS[2:0] = 0
	*/
}
/*^^------------INIT----------------*/

/*-------------ISR----------------*/
ISR (TIMER1_COMPA_vect){

 }

ISR (TIMER0_COMPA_vect) {
	actual_display_index++;
	if (actual_display_index > 3) {
		actual_display_index=0;
	}
	UpdadeSeconds(actual_display_index);
}

ISR (TIMER2_COMPA_vect){
	//PORTD = PORTD | 0b00001000;
	PORTB ^= (1<<  PORTB3);
}

ISR(USART_RX_vect) {
	USARTReadBuffer = UDR0; // UDR - USART Data Register
	/*PORTB ^= (1<<  PORTB5);*/
}
/*^^-----------ISR----------------*/

void DisplaySeconds(display_value)
{
	PORTD = digits [display_value];	
	//PORTD = 0b00001110;
}

void UpdadeSeconds() {
	PORTB = actual_display_digits[3-actual_display_index];
	//PORTB = 0b00001001;
	display_value = display_values[actual_display_index+array_offset];
	DisplaySeconds(display_value);
}



void ModeCheck() {
	if (USARTReadBuffer == 'y') {
		PORTB ^= (1<<  PORTB5);
	}
	if (USARTReadBuffer == 's') {
		array_offset = 0;
	} else if (USARTReadBuffer == 'm') {
		array_offset = 2;
	}
}

void ADC_Read() {
	
}

void countTime() {
	display_values[0]++;
	
	if (display_values[0] > 9) {
		display_values[0]=0;
		display_values[1]++;
		
	}
	if(display_values[1] > 5) {
		display_values[1]=0;
		display_values[2]++;
	}
	if(display_values[2] > 9) {
		display_values[2] =0;
		display_values[3]++;
	}
	if(display_values[3] > 5) {
		display_values[3]=0;
		display_values[4]++;
	}
	if(display_values[4] > 4) {
		display_values[4] =0;
		display_values[5]++;
	}
	if(display_values[5] > 2) {
		display_values[0]=0;
		display_values[1]=0;
		display_values[2]=0;
		display_values[3]=0;
		display_values[4]=0;
		display_values[5]=0;
	}
}
