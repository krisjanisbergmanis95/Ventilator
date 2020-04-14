/*
 * 2020.02.26_7_segment_display_DEMO.c
 *
 * Created: 3/2/2020 3:26:10 AM
 * Author : 
 */ 


#define F_CPU 16000000UL

#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <avr/interrupt.h>
uint8_t display_values [4] = {0, 0, 0, 0}; // going around - filling up
uint8_t digits [10] = {0b01111110, 0b00001100, 0b10110110, 0b10011110, 0b11001100,
0b11011010, 0b11111010, 0b00001110, 0b11111110, 0b11011110}; // 7 segment display digits
uint8_t actual_display_digits [4] = {0b0000001, 0b00000010, 0b00000100, 0b00010000};
uint8_t display_value=0, actual_display_index = 0, USARTReadBuffer = 0, low = 0,
high = 0, voltage = 0, T0 = 298, B = 3797, R0 = 10000, T = 0;
float AREF = 4.83;
float CURRENT = 0.04;

void PortInit();
void DisplayNumbers();
void USARTInit();
void Init_TC1_MM_SS();
void Init_TC0_MULTIPLEX();
void Init_TC2_MOTOR();
void UpdadeSeconds();
void ModeCheck();
void getNumbers(uint8_t T);
void Init_ADC();
void updateMotor();
int ADC_Read();
int SteinhartHartCalculation(uint8_t voltage);

int main(void)
{
	PortInit();
	USARTInit();
	Init_TC1_MM_SS();
	Init_TC0_MULTIPLEX();
	Init_TC2_MOTOR();
	Init_ADC();
	
	sei ();
	
	while (1)
	{
		ModeCheck();	
	}
}

/*-------------INIT----------------*/
void PortInit()
{
	DDRD = 0xFF;
	DDRB = 0x2F;
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

void Init_TC2_MOTOR() {
	TCNT2 = 0; //set timer counter 1 to 0
	OCR2A = 30;//15625;//8000;//15625
	//enable OUTPUT COMPARE MATCH A in TIMSK1 reg
	TIMSK2 = (1 << OCIE2A);
	//TC1 for fast PWM
	//SET waveform generators in  timer/counter control registers
	TCCR2A = ( 1 <<WGM21) | (1<< WGM20);
	TCCR2B = (1<< WGM22) | (1<< CS22) | (1<< CS20);
	//SET CLK /1024
	//TCCR0B = (1<< CS02) | (1<< CS00);
}

void Init_ADC() {
	 //reģistrā ADMUX kā sprieguma referenci izvēlēties AVCC with external capacitor at AREF pin
	 ADMUX = (1 << REFS0);
	// 	reģistrā ADCSRA biti ADPS0 - ADPS2 dalīšanas faktoram 128
	// 	reģistrā ADCSRA jāiestata bits ADEN un ADCSR

	ADCSRA = (1 << ADEN) | (1<< ADSC)| (1<< ADPS2)| (1<< ADPS1)| (1<< ADPS1);
	// 	ADC Auto Trigger Source - free running mode ADCSRB.ADTS[2:0] = 0
}
/*^^------------INIT----------------*/

/*-------------ISR----------------*/
ISR(USART_RX_vect) {
	USARTReadBuffer = UDR0; // UDR - USART Data Register
	PORTB ^= (1<<  PORTB5);
}

ISR (TIMER0_COMPA_vect) {
	actual_display_index++;
	if (actual_display_index > 3) {
		actual_display_index=0;
	}
	UpdadeSeconds(actual_display_index);
}


ISR (TIMER1_COMPA_vect){
	voltage = ADC_Read();
	T = SteinhartHartCalculation(voltage);
	getNumbers(T);
	updateMotor();
}

ISR (TIMER2_COMPA_vect){
	//PORTD = PORTD | 0b00001000;
	PORTB ^= (1<<  PORTB3);
}

/*^^-----------ISR----------------*/
ModeCheck() {
	if (USARTReadBuffer == '1') {
		PORTB ^= (1<<  PORTB5);
	}
}

int ADC_Read() {
	//low = ADCL;
	//high = ADCH;
	//return ADCL | (ADCH << 8);
	//ADMUX = (1<<REFS0);
	ADCSRA |= (1<< ADSC);
	//while (ADCSRA & (1<<ADSC)) {};
	return ADCW; 
}

void DisplaySeconds(display_value){
	PORTD = digits [display_value];
}

void UpdadeSeconds() {
	PORTB = actual_display_digits[3-actual_display_index];
	display_value = display_values[actual_display_index];
	DisplaySeconds(display_value);
}

int SteinhartHartCalculation(uint8_t voltage){
	double voltageVolts = (voltage * AREF)/1024;
	double resistance = voltageVolts/CURRENT;
	return (T0 * B/(T0 * log(resistance/R0)+B));
}

void getNumbers(uint8_t T) {
 	int i;
	for (i = 0; i< 3; i++) {
		int digit = T % 10;
		display_values[i] = digit;
		T /= 10;
	}
}

void updateMotor(){
	OCR2A = 40000000;
}
