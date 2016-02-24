#include <pic32mx.h>
#include <stdint.h>

#define DISPLAY_VDD PORTFbits.RF6
#define DISPLAY_VBATT PORTFbits.RF5
#define DISPLAY_COMMAND_DATA PORTFbits.RF4
#define DISPLAY_RESET PORTGbits.RG9


#define DISPLAY_VDD_PORT PORTF
#define DISPLAY_VDD_MASK 0x40
#define DISPLAY_VBATT_PORT PORTF
#define DISPLAY_VBATT_MASK 0x20
#define DISPLAY_COMMAND_DATA_PORT PORTF
#define DISPLAY_COMMAND_DATA_MASK 0x10
#define DISPLAY_RESET_PORT PORTG
#define DISPLAY_RESET_MASK 0x200
#define TMR2PERIOD ((80000000 / 256) / 10)

#if TMR2PERIOD > 0xFFFF
#error "Timer_period_is_too_big."
#endif

char textbuffer[4][16];

static const uint8_t const font[1024];

const uint8_t const wall[] = {
	255, 255, 15, 15, 0, 0, 255, 255
};

const uint8_t const bird[] = {
	16, 40, 76, 74, 57, 71, 81, 45, 46, 40, 16
};

const uint8_t const num[10][36];

num[0] = {
	254, 253, 123, 007, 007, 007, 007, 007, 007, 123, 253, 254,

	254, 126, 188, 192, 192, 192, 192, 192, 192, 188, 126, 254,

	000, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 000
};

num[1] = {};
num[2] = {};
num[3] = {};
num[4] = {};

num[5] = {};
num[6] = {};
num[7] = {};

num[8] = {
	254, 253, 123, 135, 135, 135, 135, 135, 135, 123, 253, 254,

	254, 126, 189, 195, 195, 195, 195, 195, 195, 189, 126, 254,

	000, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 000
};

num[9] = {
	254, 253, 123, 135, 135, 135, 135, 135, 135, 123, 253, 254,

	000, 000, 129, 195, 195, 195, 195, 195, 195, 189, 126, 254,

	000, 001, 001, 001, 001, 001, 001, 001, 001, 001, 001, 000
};


void delay(int cyc) {
	int i;
	for(i = cyc; i > 0; i--);
}

/*
void interrupt_init() {
	// INTERRUPT INITIALIZATION 
		IPC(4) = 0x1F000000;			// prio 7, sub 3: IPC <28:24>
		IFS(0) = 0x0;					// clear flags
		IEC(0) = 0x80000;				// SW4 enable interrupts: <IECO:19>
		enable_interrupt();				// Enable interrupts globally
}
*/

uint8_t spi_send_recv(uint8_t data) {
	while(!(SPI2STAT & 0x08));
	SPI2BUF = data;
	while(!(SPI2STAT & 0x01));
	return SPI2BUF;
}

void display_init() {
	DISPLAY_COMMAND_DATA_PORT &= ~DISPLAY_COMMAND_DATA_MASK;
	delay(10);
	DISPLAY_VDD_PORT &= ~DISPLAY_VDD_MASK;
	delay(1000000);
	
	spi_send_recv(0xAE);
	DISPLAY_RESET_PORT &= ~DISPLAY_RESET_MASK;
	delay(10);
	DISPLAY_RESET_PORT |= DISPLAY_RESET_MASK;
	delay(10);
	
	spi_send_recv(0x8D);
	spi_send_recv(0x14);
	
	spi_send_recv(0xD9);
	spi_send_recv(0xF1);
	
	DISPLAY_VBATT_PORT &= ~DISPLAY_VBATT_MASK;
	delay(10000000);
	
	spi_send_recv(0xA1);
	spi_send_recv(0xC8);
	
	spi_send_recv(0xDA);
	spi_send_recv(0x20);
	
	spi_send_recv(0xAF);
}

void display_string(int line, char *s) {
	int i;
	if(line < 0 || line >= 4)
		return;
	if(!s)
		return;
	
	for(i = 0; i < 16; i++)
		if(*s) {
			textbuffer[line][i] = *s;
			s++;
		} else
			textbuffer[line][i] = ' ';
}

void display_number (int x, const uint8_t *data) {
int i, j;
	
	for(i = 0; i < 3; i++) {
		DISPLAY_COMMAND_DATA_PORT &= ~DISPLAY_COMMAND_DATA_MASK;
		spi_send_recv(0x22); // = set page
		spi_send_recv(i + 1); // = which page

		spi_send_recv(0x21); // set column address
		spi_send_recv(x & 0xF); // column lower 4 bits
		spi_send_recv(0x10 | ((x >> 4) & 0xF)); // upper 4 bits
		
		DISPLAY_COMMAND_DATA_PORT |= DISPLAY_COMMAND_DATA_MASK;
		
		for(j = 0; j < 12; j++)
			spi_send_recv(data[i*12 + j]);
	}
}

void print_number (int x) {

uint8_t *num_1, *num_2, *num_3;

num_1 = num[0];
num_2 = num[0];
num_3 = num[0];

int digits = 2;

if (x < 100)
{	
	if (x < 50)
	{
		if (x < 10)
		{
			digits = 1;
			num_1 = num[x];
		}

		if ((x > 9) & (x < 20))
		{
			num_2 = num[1];
			num_1 = num[x - 10];
		}

		if ((x > 19) & (x < 30))
		{
			num_2 = num[2];
			num_1 = num[x - 20];
		}

		if ((x > 29) & (x < 40))
		{
			num_2 = num[3];
			num_1 = num[x - 30];
		}

		if ((x > 39))
		{
			num_2 = num[4];
			num_1 = num[x - 40];
		}
	}

	else
	{	
		if ((x < 60))
		{
			num_2 = num[5];
			num_1 = num[x - 50];
		}

		if ((x < 70) & (x > 59))
		{
			num_2 = num[6];
			num_1 = num[x - 60];
		}

		if ((x < 80) & (x > 69))
		{
			num_2 = num[7];
			num_1 = num[x - 70];
		}

		if ((x < 90) & (x > 79))
		{
			num_2 = num[8];
			num_1 = num[x - 80];
		}

		if (x > 89)
		{
			num_2 = num[9];
			num_1 = num[x - 90];
		}

	}
}

else
{
	num_3 = num[1];
	digits = 3;
}

if (digits == 1)
{
	display_update();
	display_number(58, num_1);
}

if (digits == 2)
{
	display_update();
	display_number(45, num_2);
	display_number(60, num_1);
}

if (digits == 3)
{
	display_update();
	display_number(43, num_3);
	display_number(58, num_2);
	display_number(73, num_1);
}

}

void display_bird(int x, const uint8_t *data) {

		DISPLAY_COMMAND_DATA_PORT &= ~DISPLAY_COMMAND_DATA_MASK;
		spi_send_recv(0x22); // = set page
		spi_send_recv(x); // = which page
		
		spi_send_recv(0x21); // set column address
		spi_send_recv(0x18 & 0xF); // column lower 4 bits
		spi_send_recv(0x10 | ((0x18 >> 4) & 0xF)); // upper 4 bits
		
		DISPLAY_COMMAND_DATA_PORT |= DISPLAY_COMMAND_DATA_MASK;
		int j;
		for(j = 0; j < 11; j++)
			spi_send_recv(data[j]);
}

void display_wall(int x, const uint8_t *data) {
	int i, j;
	
	for(i = 0; i < 4; i++) {
		DISPLAY_COMMAND_DATA_PORT &= ~DISPLAY_COMMAND_DATA_MASK;
		spi_send_recv(0x22); // = set page
		spi_send_recv(i); // = which page

		spi_send_recv(0x21); // set column address
		spi_send_recv(x & 0xF); // column lower 4 bits
		spi_send_recv(0x10 | ((x >> 4) & 0xF)); // upper 4 bits
		
		DISPLAY_COMMAND_DATA_PORT |= DISPLAY_COMMAND_DATA_MASK;
		
		for(j = 0; j < 2; j++)
			spi_send_recv(data[i*2 + j]);
	}
}

void display_update() {
	int i, j, k;
	int c;
	for(i = 0; i < 4; i++) {
		DISPLAY_COMMAND_DATA_PORT &= ~DISPLAY_COMMAND_DATA_MASK;
		spi_send_recv(0x22);
		spi_send_recv(i);
		
		spi_send_recv(0x0);
		spi_send_recv(0x10);
		
		DISPLAY_COMMAND_DATA_PORT |= DISPLAY_COMMAND_DATA_MASK;
		
		for(j = 0; j < 16; j++) {
			c = textbuffer[i][j];
			if(c & 0x80)
				continue;
			
			for(k = 0; k < 8; k++)
				spi_send_recv(font[c*8 + k]);
		}
	}
}

int timerinit (void) {
		/* TIMER STUFF */

		/* 1:256 PRESCALE */
		T2CON = 0x70;					// TCKPS <6:4>, 111 => 1:256

		/* SET TIMER TO START AT 0 */
		TMR2 = 0x0;

		/* SET PERIOD */
		PR2 = TMR2PERIOD;

		/* START THE TIMER */
		T2CONSET = 0x8000;				// T2CON <15> => Start timer
}

int main(void) {
	timerinit();
	/* Set up peripheral bus clock */
	OSCCON &= ~0x180000;
	OSCCON |= 0x080000;
	
	/* Set up output pins */
	AD1PCFG = 0xFFFF;
	ODCE = 0x0;
	TRISECLR = 0xFF;
	PORTE = 0x0;
	
	/* Output pins for display signals */
	PORTF = 0xFFFF;
	PORTG = (1 << 9);
	ODCF = 0x0;
	ODCG = 0x0;
	TRISFCLR = 0x70;
	TRISGCLR = 0x200;
	
	/* Set up input pins */
	TRISDSET = (3 << 7);
	TRISFSET = (1 << 1);
	
	/* Set up SPI as master */
	SPI2CON = 0;
	SPI2BRG = 4;
	
	/* Clear SPIROV*/
	SPI2STATCLR &= ~0x40;
	/* Set CKP = 1, MSTEN = 1; */
    SPI2CON |= 0x60;
	
	/* Turn on SPI */
	SPI2CONSET = 0x8000;
	
	
	display_init();

	int h, scaler, page, dead, button4, score, sc;
	page = 0;
	scaler = 0;
	dead = 0;

	while (!dead) {
		for (h = 127; h > 0; h = h - 3)
		{
			button4 = ((PORTD >> 7) & 1);

			if (button4) 
			{
				scaler = 0;
				if (page > 0)
					page--;
			}

			scaler++;
			if (scaler == 6)
			{
				sc++;
				page = page + 1;
				scaler = 0;
			}

			if (page == 4)
			{
				dead = 1;
				page = 3;
			}

			if (!dead)
			{
				display_update();
				display_wall(h, wall);
				display_bird(page, bird);
				delay(1000000);
			}

			if (sc = 5) {
				score++;
				sc = 0;
			}


		}
	}

	display_update();

	if (score > 100)
	{
		score = 100;
	}

	print_number(score);
	
	return 0;
}

