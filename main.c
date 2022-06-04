/*
 * Digital Clock.c
 *
 * Created: 4/23/2022 10:55:51 AM
 * Author : Orhan Ozbasaran
 */ 

#include <avr/io.h>

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <stdio.h>

#define XTAL_FRQ 8000000lu

#define SET_BIT(p,i) ((p) |=  (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) &   (1 << (i)))

#define WDR() asm volatile("wdr"::)
#define NOP() asm volatile("nop"::)
#define RST() for(;;);

#define DDR    DDRB
#define PORT   PORTB
#define RS_PIN 0
#define RW_PIN 1
#define EN_PIN 2

void avr_wait(unsigned short msec)
{
	TCCR0 = 3;
	while (msec--) {
		TCNT0 = (unsigned char)(256 - (XTAL_FRQ / 64) * 0.001);
		SET_BIT(TIFR, TOV0);
		WDR();
		while (!GET_BIT(TIFR, TOV0));
	}
	TCCR0 = 0;
}

static inline void
set_data(unsigned char x)
{
	PORTD = x;
	DDRD = 0xff;
}

static inline unsigned char
get_data(void)
{
	DDRD = 0x00;
	return PIND;
}

static inline void
sleep_700ns(void)
{
	NOP();
	NOP();
	NOP();
}

static unsigned char
input(unsigned char rs)
{
	unsigned char d;
	if (rs) SET_BIT(PORT, RS_PIN); else CLR_BIT(PORT, RS_PIN);
	SET_BIT(PORT, RW_PIN);
	get_data();
	SET_BIT(PORT, EN_PIN);
	sleep_700ns();
	d = get_data();
	CLR_BIT(PORT, EN_PIN);
	return d;
}

static void
output(unsigned char d, unsigned char rs)
{
	if (rs) SET_BIT(PORT, RS_PIN); else CLR_BIT(PORT, RS_PIN);
	CLR_BIT(PORT, RW_PIN);
	set_data(d);
	SET_BIT(PORT, EN_PIN);
	sleep_700ns();
	CLR_BIT(PORT, EN_PIN);
}

static void
write(unsigned char c, unsigned char rs)
{
	while (input(0) & 0x80);
	output(c, rs);
}

void
lcd_init(void)
{
	SET_BIT(DDR, RS_PIN);
	SET_BIT(DDR, RW_PIN);
	SET_BIT(DDR, EN_PIN);
	avr_wait(16);
	output(0x30, 0);
	avr_wait(5);
	output(0x30, 0);
	avr_wait(1);
	write(0x3c, 0);
	write(0x0c, 0);
	write(0x06, 0);
	write(0x01, 0);
}

void
lcd_clr(void)
{
	write(0x01, 0);
}

void
lcd_pos(unsigned char r, unsigned char c)
{
	unsigned char n = r * 40 + c;
	write(0x02, 0);
	while (n--) {
		write(0x14, 0);
	}
}

void
lcd_put(char c)
{
	write(c, 1);
}

void
lcd_puts1(const char *s)
{
	char c;
	while ((c = pgm_read_byte(s++)) != 0) {
		write(c, 1);
	}
}

void
lcd_puts2(const char *s)
{
	char c;
	while ((c = *(s++)) != 0) {
		write(c, 1);
	}
}

int is_pressed(int r, int c) {
	// Set all 8 GPIOs to N/C
	DDRA = 0;
	PORTA = 0;
	SET_BIT(DDRA, 4 + r);
	SET_BIT(PORTA, c);
	avr_wait(1);
	if (!GET_BIT(PINA, c)) {
		return 1;
	}
	return 0;
}

int get_key() {
	int i, j;
	for (i=0; i < 4; i++) {
		for (j=0; j < 4; j++) {
			if (is_pressed(i, j)) {
				return 4 * i + j + 1;
			}
		}
	}
	return 0;
}

typedef struct {
	char *location;
	int hour;
	int minute;
	int second;
} Time;


void init_time(Time *currentTime, int hour, int minute, int second, char *location) {
	currentTime->location = location;
	currentTime->hour = hour;
	currentTime->minute = minute;
	currentTime->second = second;
}

void check_time(Time *currentTime) {
	if (currentTime->second >= 60) {
		currentTime->second = 0;
		++currentTime->minute;
	}
	if (currentTime->second <= -1) {
		currentTime->second = 59;
		--currentTime->minute;
	}
	if (currentTime->minute >= 60) {
		currentTime->minute = 0;
		++currentTime->hour;
	}
	if (currentTime->minute <= -1) {
		currentTime->minute = 59;
		--currentTime->hour;
	}
	if (currentTime->hour >= 24) {
		currentTime->hour = 0;
	}
	if (currentTime->hour < 0) {
		currentTime->hour = 23;
	}
}

void advance_time(Time *currentTime) {
	++currentTime->second;
	check_time(currentTime);
}

// Print clock
void print_clock(const Time *currentTime) {
	char buf[30];
	// Print location on top row.
	lcd_pos(0, 0);
	sprintf(buf, "%s", currentTime->location);
	lcd_puts2(buf);
	// Print time on bottom row.
	lcd_pos(1, 0);
	sprintf(buf, "%02d:%02d:%02d", currentTime->hour, currentTime->minute, currentTime->second);
	lcd_puts2(buf);
}

// Print clock with currently being configured section empty.
void print_empty(int cursor, const Time *currentTime) {
	char buf[30];
	
	switch (cursor) {
		case 1:
			lcd_pos(0, 0);
			sprintf(buf, "%s", currentTime->location);
			lcd_puts2(buf);
			lcd_pos(1, 0);
			sprintf(buf, "%s:%02d:%02d", "__", currentTime->minute, currentTime->second);
			lcd_puts2(buf);
			break;
		case 2:
			lcd_pos(0, 0);
			sprintf(buf, "%s", currentTime->location);
			lcd_puts2(buf);
			lcd_pos(1, 0);
			sprintf(buf, "%02d:%s:%02d", currentTime->hour, "__", currentTime->second);
			lcd_puts2(buf);
			break;
		case 3:
			lcd_pos(0, 0);
			sprintf(buf, "%s", currentTime->location);
			lcd_puts2(buf);
			lcd_pos(1, 0);
			sprintf(buf, "%02d:%02d:%s", currentTime->hour, currentTime->minute, "__");
			lcd_puts2(buf);
			break;
		}
}

void increase(int cursor, Time *currentTime) {
	switch (cursor) {
		case 1:
			++currentTime->hour;
			break;
		case 2:
			++currentTime->minute;
			break;
		case 3:
			++currentTime->second;
			break;
	}
	check_time(currentTime);
}

void decrease(int cursor, Time *currentTime) {
	switch (cursor) {
		case 1:
		--currentTime->hour;
		break;
		case 2:
		--currentTime->minute;
		break;
		case 3:
		--currentTime->second;
		break;
	}
	check_time(currentTime);
}

int main() {
	Time times[10];
	int currentTimeCursor = 0;
	int currentAlarmCursor = 5;
	// Initialize 5 world clocks
	init_time(&times[0], 12, 53, 32, "Los Angeles, CA     ");
	init_time(&times[1], 22, 53, 32, "Ankara, Turkey      ");
	init_time(&times[2], 14, 53, 32, "Dallas, TX          ");
	init_time(&times[3], 22, 53, 32, "Izmir, Turkey       ");
	init_time(&times[4], 15, 53, 32, "Chicago, IL         ");
	// After this point alarms are initialized
	init_time(&times[5], 12, 53, 40, "Alarm 1:            ");
	init_time(&times[6], 00, 00, 00, "Alarm 2:            ");
	init_time(&times[7], 00, 00, 00, "Alarm 3:            ");
	init_time(&times[8], 00, 00, 00, "Alarm 4:            ");
	init_time(&times[9], 00, 00, 00, "Alarm 5:            ");
	lcd_init();
	while (1) 
	{
		// Mode 0: Alarm Clock, Mode 1: Timer
		int mode = 0;
		// Variable for key press
		int k, b, c;
		// Variable for cursor when configuring time
		int cursor = 1;
		avr_wait(950);
		k = get_key();
		// If button 3 is pressed on the keypad, system enters clock mode
		if (k == 14)
		{
			mode = 0;
		}
		// If button 2 is pressed on the keypad, system enters timer mode
		else if (k == 15){
			mode = 1;
		}
		avr_wait(50);
		// Clock Mode
		if (mode == 0){
			// Enter settings mode
			if (k == 1)
			{
				while (1) {
					b = get_key();
					// Exit setting mode
					if (b == 2)
					{
						break;
					}
					// Enter alarm setting mode
					else if (b == 1){
						cursor = 1;
						while (1){
							// Variable for cursor when configuring time
							c = get_key();
							avr_wait(10);
							if (c == 2)
							{
								break;
							}
							// Move cursor right
							else if (c == 4)
							{
								++cursor;
								if (cursor == 4) {
									cursor = 1;
								}
							}
							// Increase current cursor
							else if (c == 3){
								increase(cursor, &times[currentAlarmCursor]);
							}
							// Decrease current cursor
							else if (c == 7){
								decrease(cursor, &times[currentAlarmCursor]);
							}
							else if (c == 13){
								++currentAlarmCursor;
								if (currentAlarmCursor == 10){
									currentAlarmCursor = 5;
								}
							}
							print_empty(cursor, &times[currentAlarmCursor]);
							avr_wait(200);
							print_clock(&times[currentAlarmCursor]);
							avr_wait(200);
						}
					}
					// Move cursor right
					else if (b == 4)
					{
						++cursor;
						if (cursor == 4) {
							cursor = 1;
						}
					}
					// Increase current cursor
					else if (b == 3){
						int j;
						for (j = 0; j < 5; j++){
							increase(cursor, &times[j]);
						}
					}
					// Decrease current cursor
					else if (b == 7){
						int j;
						for (j = 0; j < 5; j++){
							decrease(cursor, &times[j]);
						}
					}
					
					print_empty(cursor, &times[currentTimeCursor]);
					avr_wait(200);
					print_clock(&times[currentTimeCursor]);
					avr_wait(200);
				}
			}
			// Change the location
			else if (k == 13){
				++currentTimeCursor;
				if (currentTimeCursor == 5){
					currentTimeCursor = 0;
				}
				k = 0;
			}

			int j;
			// Advance all clocks 1 second
			for (j = 0; j < 5; j++){
				advance_time(&times[j]);
			}
			
			// Print the clock on LCD
			print_clock(&times[currentTimeCursor]);
			
			// Compare alarms and time if alarm is not set to 00:00:00
			for (j = 5; j < 10; j++){
				// Alarm is off!
				if (times[currentTimeCursor].hour == times[j].hour &&
				times[currentTimeCursor].minute == times[j].minute &&
				times[currentTimeCursor].second == times[j].second){
					// Play sound
					while(1){
						
					}
				}
			}
		// Timer mode
		} else if (mode == 1){
			
		}
	}
	return 0;
} 