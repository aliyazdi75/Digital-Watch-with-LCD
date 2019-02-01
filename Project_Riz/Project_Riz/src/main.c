
#include <avr/io.h>            /* Include AVR std. library file */
#include <util/delay.h>          /* Include inbuilt defined Delay header file */
#include <string.h>
#include <avr/interrupt.h>

#define F_CPU 1000000UL          /* Define CPU Frequency e.g. here its 8MHz */
#define LCD_Dir DDRA          /* Define LCD data port direction */
#define LCD_Port PORTA          /* Define LCD data port */
#define RS PA1              /* Define Register Select (data reg./command reg.) signal pin */
#define EN PA3               /* Define Enable signal pin */

volatile unsigned char hours;
volatile unsigned char minutes;
volatile unsigned char seconds;
int isTime = 1;
int setAlarm = 0;
int cs=0;
unsigned char DigitToLCDEncoder(unsigned char digit);
void changeHour(unsigned char);
void changeMinute(unsigned char);
void alarm(void);
void state0();
void state1();
void state2();
void state3();
void state4();
void state5();
void showNumber(unsigned char h,unsigned char m,unsigned char s);
ISR(TIMER1_COMPA_vect);

//Alarm0
unsigned char minutesAlarmStart0 = 10; 
unsigned char hoursAlarmStart0 =11;
unsigned char minutesAlarmFinish0 = 12;
unsigned char hoursAlarmFinish0 = 13;
//Alarm1
unsigned char minutesAlarmStart1 = 14;
unsigned char hoursAlarmStart1 = 15;
unsigned char minutesAlarmFinish1 = 16;
unsigned char hoursAlarmFinish1 = 17;
//Date
unsigned char day = 3;
unsigned char month = 11;
unsigned char year = 97;

int stateTable[6][3]={
	{1,2,4},
	{0,2,4},
	{-1,3,-1},
	{-1,0,-1},
	{-1,-1,5},
	{-1,-1,0}
};

void LCD_Command( unsigned char cmnd )
{
	LCD_Port = (LCD_Port & 0x0F) | (cmnd & 0xF0); /* sending upper nibble */
	LCD_Port &= ~ (1<<RS);        /* RS=0, command reg. */
	LCD_Port |= (1<<EN);        /* Enable pulse */
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4);  /* sending lower nibble */
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}

void LCD_Char( unsigned char data )
{
	LCD_Port = (LCD_Port & 0x0F) | (data & 0xF0); /* sending upper nibble */
	LCD_Port |= (1<<RS);        /* RS=1, data reg. */
	LCD_Port|= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);

	_delay_us(200);

	LCD_Port = (LCD_Port & 0x0F) | (data << 4); /* sending lower nibble */
	LCD_Port |= (1<<EN);
	_delay_us(1);
	LCD_Port &= ~ (1<<EN);
	_delay_ms(2);
}

void LCD_Init (void)          /* LCD Initialize function */
{
	LCD_Dir = 0xFF;            /* Make LCD command port direction as o/p */
	_delay_ms(20);            /* LCD Power ON delay always >15ms */
	
	LCD_Command(0x33);
	LCD_Command(0x32);            /* send for 4 bit initialization of LCD  */
	LCD_Command(0x28);                /* Use 2 line and initialize 5*7 matrix in (4-bit mode)*/
	LCD_Command(0x0c);                /* Display on cursor off*/
	LCD_Command(0x06);                /* Increment cursor (shift cursor to right)*/
	LCD_Command(0x01);                /* Clear display screen*/
	_delay_ms(2);
	LCD_Command (0x80);          /* Cursor 1st row 0th position */
}

void LCD_String (char *str)        /* Send string to LCD function */
{
	int i;
	for(i=0;str[i]!=0;i++)        /* Send each char of string till the NULL */
	{
		LCD_Char (str[i]);
	}
}

void LCD_String_xy (char row, char pos, char *str)  /* Send string to LCD with xy position */
{
	if (row == 0 && pos<16)
	LCD_Command((pos & 0x0F)|0x80);    /* Command of first row and required position<16 */
	else if (row == 1 && pos<16)
	LCD_Command((pos & 0x0F)|0xC0);    /* Command of first row and required position<16 */
	LCD_String(str);          /* Call LCD string function */
}

void LCD_Clear()
{
	LCD_Command (0x01);          /* Clear display */
	_delay_ms(2);
	LCD_Command (0x80);          /* Cursor 1st row 0th position */
}


int main (void)
{
	
	LCD_Init();
	
	TCCR1B = (1<<CS12|1<<WGM12);
	OCR1A = 15625-1;
	TIMSK = 1<<OCIE1A;
	
	DDRD = 0xFF;
	PORTB = 0XFF;
	DDRC = 0XFF;
	//PINB = 0xFF;

	sei();
	
	while(1)
	{
		alarm();
		if((PINB & 0x01)==0 & stateTable[cs][0]!=-1){
			cs=stateTable[cs][0];
			_delay_ms(1000);
		}
		else if((PINB & 0x02)==0 & stateTable[cs][1]!=-1){
			cs=stateTable[cs][1];
			_delay_ms(1000);
		}
		else if((PINB & 0x04)==0 & stateTable[cs][2]!=-1){
			cs=stateTable[cs][2];
			_delay_ms(1000);
		}

		switch(cs){
			case 0: state0();
			break;
			case 1: state1();
			break;
			case 2: state2();
			break;
			case 3: state3();
			break;
			case 4: state4();
			break;
			case 5: state5();
			break;
		}

	}
	return 0;
}

void state0(){
	changeMinute(minutes);
	changeHour(hours);
	showTime(hours,minutes,seconds);
}

void state1(){
	showDate(year,month,day);
}

void state2(){
	changeMinute(minutesAlarmStart0);
	changeHour(hoursAlarmStart0);
	showNumber(hoursAlarmStart0,minutesAlarmStart0,0);
}

void state3(){
	changeMinute(minutesAlarmFinish0);
	changeHour(hoursAlarmFinish0);
	showNumber(hoursAlarmFinish0,minutesAlarmFinish0,0);
}
void state4(){
	changeMinute(minutesAlarmStart1);
	changeHour(hoursAlarmStart1);
	showNumber(hoursAlarmStart1,minutesAlarmStart1,0);
}
void state5(){
	changeMinute(minutesAlarmFinish1);
	changeHour(hoursAlarmFinish1);
	showNumber(hoursAlarmFinish1,minutesAlarmFinish1,0);
}
void changeMinute(unsigned char m){
	if((PINB & 0x08)==0){
		m++;
		if(m == 60){
			m = 0;
		}
		_delay_ms(1000);
	}
}

void changeHour(unsigned char h){
	if((PINB & 0x10)==0){
		h++;
		if(h == 24){
			h = 0;
		}
		_delay_ms(1000);
	}
}

void showTime(unsigned char h,unsigned char m,unsigned char s){
	LCD_Clear();
	LCD_String (" Time= ");
	showNumber(hours,minutes,seconds);
}

void showDate(unsigned char h,unsigned char m,unsigned char s){
	LCD_Clear();
	LCD_String (" Date= ");
	showNumber(year,month,day);
}

void showNumber(unsigned char h,unsigned char m,unsigned char s){
	
	DigitToLCDEncoder (h/10);
	DigitToLCDEncoder (h%10);
	LCD_String (":");
	DigitToLCDEncoder (m/10);
	DigitToLCDEncoder (m%10);
	LCD_String (":");
	DigitToLCDEncoder (s/10);
	DigitToLCDEncoder (s%10);
	_delay_ms(1000);
}

unsigned char DigitToLCDEncoder(unsigned char digit)
{
	
	switch(digit)
	{
		case 0:  LCD_String("0");
		break;
		case 1: LCD_String("1");
		break;
		case 2:  LCD_String("2");
		break;
		case 3:  LCD_String("3");
		break;
		case 4:  LCD_String("4");
		break;
		case 5:  LCD_String("5");
		break;
		case 6:  LCD_String("6");
		break;
		case 7:  LCD_String("7");
		break;
		case 8:  LCD_String("8");
		break;
		case 9:  LCD_String("9");
	}
}
void alarm(){
	if (hours==hoursAlarmStart0 && minutes == minutesAlarmStart0)
	{
		PORTD =0xFF;
		}else{
		PORTD =0x00;
	}

}

ISR(TIMER1_COMPA_vect)
{
	seconds++;
	if(seconds == 60){
		seconds = 0;
		minutes++;
	}
	if(minutes == 60){
		minutes = 0;
		hours++;
	}
	if(hours > 23){
		hours = 0;
		day++;
	}
	if(day == 31 && month > 7){
		day = 1;
		month++;
	}
	if(day == 32 && month < 7){
		day = 1;
		month++;
	}
	if(month == 13){
		month = 1;
		year++;
	}
	if(year > 99){
		year = 97;
	}
	
}

