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

const uint8_t const num[10][36] = {
		{
		254, 253, 123, 7, 7, 7, 7, 7, 7, 123, 253, 254,
		254, 126, 188, 192, 192, 192, 192, 192, 192, 188, 126, 254,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
		},

		{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 120, 252, 254,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 188, 126, 254,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		},

		{
		1, 3, 7, 135, 135, 135, 135, 135, 135, 123, 253, 254,
		254, 126, 189, 195, 195, 195, 195, 195, 195, 193, 128, 0,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
		},

		{
		1, 3, 7, 135, 135, 135, 135, 135, 135, 123, 253, 254,
		0, 128, 193, 195, 195, 195, 195, 195, 195, 189, 126, 254,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
		},

		{
		254, 252, 120, 128, 128, 128, 128, 128, 128, 120, 252, 254,
		0, 0, 1, 3, 3, 3, 3, 3, 3, 188, 126, 254,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		},

		{
		254, 253, 123, 135, 135, 135, 135, 135, 135, 7, 3, 1,
		0, 128, 193, 195, 195, 195, 195, 195, 195, 189, 126, 254,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
		},

		{
		254, 253, 123, 135, 135, 135, 135, 135, 135, 7, 3, 1,
		254, 126, 189, 195, 195, 195, 195, 195, 195, 189, 126, 254,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
		},

		{
		254, 253, 123, 7, 7, 7, 7, 7, 7, 123, 253, 254,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 188, 126, 254,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		},

		{
		254, 253, 123, 135, 135, 135, 135, 135, 135, 123, 253, 254,
		254, 126, 189, 195, 195, 195, 195, 195, 195, 189, 126, 254,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
		},

		{
		254, 253, 123, 135, 135, 135, 135, 135, 135, 123, 253, 254,
		0, 0, 129, 195, 195, 195, 195, 195, 195, 189, 126, 254,
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0
		}
	};

void delay(int cyc) {
	int i;
	for(i = cyc; i > 0; i--);
}

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

void display_number (int x, const uint8_t* data) {
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

void print_max100 (uint8_t x) {
	
	uint8_t num_1, num_2;
	num_1 = num_2 = 0;

	uint8_t a, c, digits;
	a = 10;
	c = 1;

	if (x < 100)
	{
		digits = 2;

		if (x < 10)
			{
				digits = 1;
				num_1 = x;
				c = 0;
			}

		while (c)
		{
			if ((x >= a) & (x < (a + 10)))
			{
				num_2 = (a / 10);
				num_1 = (x - a);
				c = 0;
			}

			a = a + 10;
		}
	}
	else digits = 3;

	display_update();
	switch (digits)
	{
		case '1' :
		display_number(58, num[num_1]);
		break;

		case '2' :
		display_number(45, num[num_2]);
		display_number(60, num[num_1]);
		break;

		case '3' :
		display_number(43, num[0]);
		display_number(58, num[0]);
		display_number(73, num[1]);
		break;

		default :
		PORTE = 170;
		break;
	}

}

int timerinit (void) {
		/* TIMER STUFF */

		/* 1:256 PRESCALE */
		T2CON = 0x70;					// TCKPS <6:4>, 111 => 1:256

		/* SET TIMER TO START AT 0 */
		TMR2 = 0x0;
		IFS(0) = 0x0;

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

	uint8_t h, scaler, page, alive, button4, score, sc;
	page = scaler = score = 0;
	alive = 1;

	while (alive) {
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
				alive = 0;
				page = 3;
			}

			if (alive)
			{
				display_update();
				display_wall(h, wall);
				display_bird(page, bird);
				delay(1000000);
			}

			if (sc = 5) {
				score++;
				sc = 0;

				if (score == 100)
					sc = 6; 		// stop counting score
			}

		}
	}

	print_max100(score);
	
	return 0;
}
