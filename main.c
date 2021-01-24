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
uint8_t display_value=0, actual_display_index = 0, USARTReadBuffer = 0;
int T0 = 298;
int B = 3797;
int R0 = 10000;
float CURRENT = 0.00025;

int ocra_val_40 = 102;
int ocra_val_50 = 128;
int ocra_val_60 = 153;
int ocra_val_70 = 179;
int ocra_val_80 = 204;
int ocra_val_90 = 230;
int ocra_val_100 = 255;

void PortInit();
void DisplayNumbers();
void USARTInit();
void Init_TC1_MM_SS();
void Init_TC0_MULTIPLEX();
void Init_TC2_MOTOR();
void UpdadeSeconds();
void ModeCheck();
void getNumbers(int T);
void Init_ADC();
void updateMotor();
uint16_t ADC_Read();
int SteinhartHartCalculation(uint16_t voltage);

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
	//enable OUTPUT COMPARE MATCH A in TIMSK1 reg
	//TIMSK2 = (1 << OCIE2A);
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
}

ISR (TIMER0_COMPA_vect) {
	actual_display_index++;
	if (actual_display_index > 3) {
		actual_display_index=0;
	}
	UpdadeSeconds(actual_display_index);
}


ISR (TIMER1_COMPA_vect){
	uint16_t voltage = ADC_Read();
	int T = SteinhartHartCalculation(voltage);
	//int T = 99;
	//int T = 31;
	getNumbers(T);
	updateMotor(T);
}

ISR (TIMER2_COMPA_vect){
	PORTB ^= (1<<  PORTB3);
}

/*^^-----------ISR----------------*/
ModeCheck() {
	switch (USARTReadBuffer) {
		case 'y':
			PORTB ^= (1<<  PORTB5);
			break;
		case '0':
		ADMUX = (1 << REFS0);
		break;
		case '1':
		ADMUX = (1 << REFS0) |(1 << MUX0);
		break;
		case '2':
		ADMUX = (1 << REFS0) | (1 << MUX1);
		break;		
		case 'a':
		OCR2A = ocra_val_40;
		break;
		case 'b':
		OCR2A = ocra_val_50;
		break;
		case 'c':
		OCR2A = ocra_val_60;
		break;
		case 'd':
		OCR2A = ocra_val_70;
		break;
		case 'f':
		OCR2A = ocra_val_80;
		break;
		case 'g':
		OCR2A = ocra_val_90;
		break;
		case 'h':
		OCR2A = ocra_val_100;
		break;
		case 'z':
		TIMSK2 = (0 << OCIE2A);
		break;
		case 'x':
		TIMSK2 = (1 << OCIE2A);
		break;
	}	
}

uint16_t ADC_Read() {
	//low = ADCL;
	//high = ADCH;
	//return ADCL | (ADCH << 8);
	//ADMUX = (1<<REFS0);
	ADCSRA |= (1<< ADSC);
	while (ADCSRA & (1<<ADSC)) {};
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

int SteinhartHartCalculation(uint16_t voltage){
	double voltageVolts = (voltage * 5)/1023;
	double resistance = voltageVolts/CURRENT;
	return ((T0 * B)/((T0 * log(resistance/R0))+B));
	//return voltageVolts * 4;
}

void getNumbers(int T) {
 	int i;
	for (i = 0; i< 3; i++) {
		int digit = T % 10;
		display_values[i] = digit;
		T /= 10;
	}
}

void updateMotor(int T){
	int lim1 = 10;
	int lim2 = 11;
	int lim3 = 12;
	int lim4 = 13;
	int lim5 = 14;
	int lim6 = 17;
	int lim7 = 20;
	int lim8 = 23;
// 	int ocra_val_40 = 102;
// 	int ocra_val_50 = 128;
// 	int ocra_val_60 = 153;
// 	int ocra_val_70 = 179;
// 	int ocra_val_80 = 204;
// 	int ocra_val_90 = 230;
// 	int ocra_val_100 = 255;
	/*int lim1 = 30;
	int lim2 = 40;
	int lim3 = 50;
	int lim4 = 60;
	int lim5 = 70;
	int lim6 = 80;
	int lim7 = 90;
	int lim8 = 100;*/
	if (T <= lim1) {
		TIMSK2 = (0 << OCIE2A);
	}
	else  {
		//if (T > 30) {
			if (T > lim1 && T < lim2) {
			OCR2A = ocra_val_40;
		} 
		else if (T > lim2 && T < lim3) {
		OCR2A = ocra_val_50;
		}
		else if (T > lim3 && T < lim4) {
		OCR2A = ocra_val_60;
		}
		else if (T > lim4 && T < lim5) {
			OCR2A = ocra_val_70;
		}
		else if (T > lim5 && T < lim6) {
			OCR2A = ocra_val_80;
		}
		else if (T > lim6 && T < lim7) {
			OCR2A = ocra_val_90;
		}
		else if (T > lim7 && T < lim8) {
			OCR2A = ocra_val_100;
		}
		TIMSK2 = (1 << OCIE2A);
	}
}
