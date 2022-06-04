/*
 * Digital Clock.c
 *
 * Created: 06/04/2022 10:55:51 AM
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

void avr_wait2(unsigned short msec)
{
	TCCR0 = 2;
	while (msec--) {
		TCNT0 = (unsigned char)(256 - (XTAL_FRQ / 128) * 0.001);
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

// SPEAKER
typedef enum {
	A, As, B, C, Cs, D, Ds, Ee, F, Fs, G, Gs, A1, As1, B1, C1, Cs1, D1, Ds1, Ee1, F1, Fs1, G1, Gs1, Z
} Note;

typedef enum {
	W, HQ, H, Q, Ei
} Duration;

typedef struct {
	Note note;
	Duration duration;
} PlayingNote;

void play_note(const PlayingNote* note) {
	int i, k, wait;
	double f;
	k = 0;
	f = 0;
	wait = 0;
	switch (note->note) {
		case (A1):
		wait = 46;
		f = 110;
		break;
		case As1:
		wait = 42;
		f = 116.54;
		break;
		case B1:
		wait = 40;
		f = 123.47;
		break;
		case C1:
		wait = 38;
		f = 130.815;
		break;
		case Cs1:
		wait = 36;
		f = 138.59;
		break;
		case D1:
		wait = 34;
		f = 146.83;
		break;
		case Ds1:
		wait = 32;
		f = 155.565;
		break;
		case Ee1:
		wait = 30;
		f = 164.815;
		break;
		case F1:
		wait = 29;
		f = 174.615;
		break;
		case Fs1:
		wait = 28;
		f = 184.995;
		break;
		case G1:
		wait = 26;
		f = 196;
		break;
		case Gs1:
		wait = 24;
		f = 207.65;
		break;
		case A:
		wait = 23;
		f = 220.00;
		break;
		case As:
		wait = 21;
		f = 233.08;
		break;
		case B:
		wait = 20;
		f = 246.94;
		break;
		case C:
		wait = 19;
		f = 261.63;
		break;
		case Cs:
		wait = 18;
		f = 277.18;
		break;
		case D:
		wait = 17;
		f = 293.66;
		break;
		case Ds:
		wait = 16;
		f = 311.13;
		break;
		case Ee:
		wait = 15;
		f = 329.63;
		break;
		case F:
		wait = 14;
		f = 349.23;
		break;
		case Fs:
		wait = 14;
		f = 369.99;
		break;
		case G:
		wait = 13;
		f = 392.00;
		break;
		case Gs:
		wait = 12;
		f = 415.30;
		break;
		case Z:
		wait = 30;
		f = 166.66;
		break;
	}
	
	switch (note->duration) {
		case W:
		k = f;
		break;
		case HQ:
		k = 0.75 * f;
		break;
		case H:
		k = 0.5 * f;
		break;
		case Q:
		k = 0.25 * f;
		break;
		case Ei:
		k = 0.125 * f;
		break;
	}
	
	if (wait == 30) {
		for (i = 0; i < k; i++) {
			avr_wait2(wait*2);
		}
	}
	else {
		for (i = 0; i < k; i++) {
			SET_BIT(PORTB, 3);
			avr_wait2(wait);
			CLR_BIT(PORTB, 3);
			avr_wait2(wait);
		}
	}

}

void play_song(const PlayingNote song[], int length) {
	int i, k;
	for (i = 0; i < length; i++) {
		k = get_key();
		if (k == 2) {
			break;
		}
		play_note(&song[i]);
	}
}

// CLOCK
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

void check_timer(Time *currentTime) {
	if (currentTime->second >= 60) {
		currentTime->second = 0;
		++currentTime->minute;
	}
	if (currentTime->minute >= 60) {
		currentTime->minute = 0;
		++currentTime->hour;
	}
	if (currentTime->second < 0) {
		currentTime->second = 59;
		--currentTime->minute;
	}
	if (currentTime->minute < 0) {
		currentTime->minute = 59;
		--currentTime->hour;
	}
	if (currentTime->hour < 0) {
		currentTime->hour = 0;
	}
}

void advance_timer(Time *currentTime) {
	--currentTime->second;
	check_timer(currentTime);
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
	SET_BIT(DDRB, 3);
	PlayingNote star_wars[] = {
			{G1, HQ},
			{Z, Q},
			{G1, HQ},
			{Z, Q},
			{G1, HQ},
			{Z, Q},
			{Ds1, H},
			{Z, Q},
			{As, Q},
			{G1, HQ},
			{Z, Q},
			{Ds1, H},
			{Z, Q},
			{As, Q},
			{G1, W},
			{G1, H},
			{Z, H},
			{D, HQ},
			{Z, Q},
			{D, HQ},
			{Z, Q},
			{D, HQ},
			{Z, Q},
			{Ds, H},
			{Z, Q},
			{As, Q},
			{Fs1, HQ},
			{Z, Q},
			{Ds1, H},
			{Z, Q},
			{As, Q},
			{G1, HQ},
			{G1, HQ},
			{Z, Q},
			{G1, HQ},
			{Z, Q},
			{G1, HQ},
			{Z, Q},
			{Ds1, H},
			{Z, Q},
			{As, Q},
			{G1, HQ},
			{Z, Q},
			{Ds1, H},
			{Z, Q},
			{As, Q},
			{G1, W},
			{G1, H},
			{Z, H},
			{D, HQ},
			{Z, Q},
			{D, HQ},
			{Z, Q},
			{D, HQ},
			{Z, Q},
			{Ds, H},
			{Z, Q},
			{As, Q},
			{Fs1, HQ},
			{Z, Q},
			{Ds1, H},
			{Z, Q},
			{As, Q},
			{G1, HQ},
			{G1, HQ},
			{Z, Q},
			{G1, HQ},
			{Z, Q},
			{G1, HQ},
			{Z, Q},
			{Ds1, H},
			{Z, Q},
			{As, Q},
			{G1, HQ},
			{Z, Q},
			{Ds1, H},
			{Z, Q},
			{As, Q},
			{G1, W},
			{G1, H},
			{Z, H},
			{D, HQ},
			{Z, Q},
			{D, HQ},
			{Z, Q},
			{D, HQ},
			{Z, Q},
			{Ds, H},
			{Z, Q},
			{As, Q},
			{Fs1, HQ},
			{Z, Q},
			{Ds1, H},
			{Z, Q},
			{As, Q},
			{G1, HQ},
			/* Keep going... */
		};
	Time times[11];
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
	// After this point timer is initialized
	init_time(&times[10], 00, 00, 10, "Timer:            ");
	// Mode 0: Alarm Clock, Mode 1: Timer
	int mode = 0;
	lcd_init();
	while (1) 
	{
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
						play_song(star_wars, 99);
					}
				}
			}
		// Timer mode
		} else if (mode == 1){
			// Enter Setting mode
			if (k == 1) {
				while (1) {
					b = get_key();
					// Exit setting mode
					if (b == 2)
					{
						break;
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
						increase(cursor, &times[10]);
					}
					// Decrease current cursor
					else if (b == 7){
						decrease(cursor, &times[10]);
					}
					print_empty(cursor, &times[10]);
					avr_wait(200);
					print_clock(&times[10]);
					avr_wait(200);
				}
			}
			// Alarm is off!
			advance_timer(&times[10]);
			if (times[10].hour == 0 &&
				times[10].minute == 0 &&
				times[10].second == 0){
					// Play sound
					while(1){
						play_song(star_wars, 99);
					}
				}
			print_clock(&times[10]);
		}
	}
	return 0;
} 
