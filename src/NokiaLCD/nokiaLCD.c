#include "stm32f4_discovery.h"
#include "nokiaLCD.h"
#include "english_6x8_pixel.h"
#include "bmp_pixel.h"
#include "../Delay/delay.h"

#define LCD5110_CS_PORT GPIOC
#define LCD5110_CS_PIN GPIO_Pin_9

#define LCD5110_RST_PORT GPIOC
#define LCD5110_RST_PIN GPIO_Pin_8

#define LCD5110_DC_PORT GPIOC
#define LCD5110_DC_PIN GPIO_Pin_7

#define LCD5110_MO_PORT GPIOC
#define LCD5110_MO_PIN GPIO_Pin_6

#define LCD5110_SCK_PORT GPIOA
#define LCD5110_SCK_PIN GPIO_Pin_8

#define LCD5110_GPIO_CLK (RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOC)

//Define the LCD Operation function
static void LCD5110_LCD_write_byte(uint8_t dat, uint8_t command);

//Define the hardware operation function
static void LCD5110_GPIO_Config(void);
static void LCD5110_SCK(uint8_t temp);
static void LCD5110_MO(uint8_t temp);
static void LCD5110_CS(uint8_t temp);
static void LCD5110_RST(uint8_t temp);
static void LCD5110_DC(uint8_t temp);

/*
 * Before, sysTick must be initialized.
 * It use delay_ms function!
 */
void LCD5110_Initialize() {
	LCD5110_GPIO_Config();
	delay_ms(10);

	LCD5110_RST(0); //LCD_RST = 0;
	delay_ms(1);
	LCD5110_RST(1); //LCD_RST = 1;

	LCD5110_CS(0); //SPI_CS = 0;
	delay_ms(1);
	LCD5110_CS(1); //SPI_CS = 1;

	delay_ms(1);

#define LCD5110_BIAS_1x24 0x15
#define LCD5110_BIAS_1x40_1x34 0x14
#define LCD5110_BIAS_1x100 0x10	//n=7 1:100
#define LCD5110_BIAS_1x80 0x11	//n=6 1:80
#define LCD5110_BIAS_1x65  0x12	//n=5 1:65/1:65
#define LCD5110_BIAS_1x48 0x13	//n=4 1:48
#define LCD5110_BIAS_1x40_1x34 0x14	//n=3 1:40/1:34
#define LCD5110_BIAS_1x24 0x15	//n=2 1:24
#define LCD5110_BIAS_1x18_1x16  0x16	//n=1 1:18/1:16
#define LCD5110_BIAS_1x10_1x9_1x8  0x17	//n=0 1:10/1:9/1:8

	//Must be adjusted to the temperature of ambient.
#define LCD5110_TEMPERATURE_COEFFICIENT0 0x04
#define LCD5110_TEMPERATURE_COEFFICIENT1 0x05
#define LCD5110_TEMPERATURE_COEFFICIENT2 0x06
#define LCD5110_TEMPERATURE_COEFFICIENT3 0x07

#define LCD5110_CommandSet_Extended 0x21	//H = 1
#define LCD5110_CommandSet_Basic 0x20	//H = 0

#define LCD5110_NormalMode 0x0C	//H = 0


	LCD5110_LCD_write_byte(LCD5110_CommandSet_Extended, 0);
	LCD5110_LCD_write_byte(0xC0, 0);//Value of Vop(controls contrast) = (0x80 | 7-bit Vop value )
	LCD5110_LCD_write_byte(LCD5110_TEMPERATURE_COEFFICIENT2, 0);
	LCD5110_LCD_write_byte(LCD5110_BIAS_1x40_1x34, 0);
	LCD5110_LCD_write_byte(LCD5110_CommandSet_Basic, 0);
	LCD5110_Clear();				//Clear LCD
	LCD5110_LCD_write_byte(LCD5110_NormalMode, 0);	//enable normal display (dark on light), horizontal addressing
	LCD5110_CS(0);	//SPI_CS = 0;
}

void LCD5110_WriteData(uint8_t c) {
	uint8_t line;
	c -= 32;
	for (line = 0; line < 6; line++) {
		LCD5110_LCD_write_byte(font6x8[c][line], 1);
	}
}

void LCD5110_WriteDataInversed(uint8_t c) {
	uint8_t line;
	c -= 32;
	for (line = 0; line < 6; line++) {
		LCD5110_LCD_write_byte(~font6x8[c][line], 1);
	}
}

/*-----------------------------------------------------------------------
 LCD_draw_map      : Bitmap drawing function
 input parameter
 starting point X,Y
 *map    bitmap data
 Pix_x   height
 Pix_y   width
 -----------------------------------------------------------------------*/
void LCD5110_draw_bmp_pixel(uint8_t X, uint8_t Y, uint8_t *map, uint8_t Pix_x,
		uint8_t Pix_y) {
	uint8_t i, n;
	uint8_t row;

	if (Pix_y % 8 == 0)
		row = Pix_y / 8;      //calculate how many line is needed
	else
		row = Pix_y / 8 + 1;

	for (n = 0; n < row; n++) {
		LCD5110_GoTo(X, Y);
		for (i = 0; i < Pix_x; i++) {
			LCD5110_LCD_write_byte(map[i + n * Pix_x], 1);
		}
		++Y;	//change line
	}
}

void LCD5110_WriteText(char *s) {
	while (*s != '\0') {
		LCD5110_WriteData(*s);
		++s;
	}
}

void LCD5110_WriteTextXY(char *s, uint8_t X, uint8_t Y) {
	LCD5110_GoTo(X,Y);
	while (*s != '\0') {
		LCD5110_WriteData(*s);
		++s;
	}
}

void LCD5110_WriteTextInversed(char *s) {
	while (*s != '\0') {
		LCD5110_WriteDataInversed(*s);
		++s;
	}
}

void LCD5110_Clear() {
//	LCD5110_LCD_write_byte(0x0c, 0);
//	LCD5110_LCD_write_byte(0x80, 0); //Inna wersja - tego nie bylo wczesniej
	for (uint16_t i = 0; i < 504; i++) {	//6x84 = 504 pixeli
		LCD5110_LCD_write_byte(0, 1);
	}
}
void LCD5110_ClearLine(uint8_t line) {
//	LCD5110_LCD_write_byte(0x0c, 0);
//	LCD5110_LCD_write_byte(0x80, 0); //Inna wersja - tego nie bylo wczesniej

	LCD5110_GoTo(0,line);
	for (uint16_t i = 0; i < 84; i++) {	//6x84 = 504 pixeli
		LCD5110_LCD_write_byte(0, 1);
	}
}

void LCD5110_GoTo(uint8_t X, uint8_t Y) {
	uint8_t x = 6 * X;	//Tego nie ma w innym
	LCD5110_LCD_write_byte(0x40 | Y, 0);	//Column
	LCD5110_LCD_write_byte(0x80 | x, 0);	//Row
}

void LCD5110_WriteNumberInDec(uint16_t b) {
	uint8_t datas[3];

	datas[0] = b / 1000;
	b = b - datas[0] * 1000;
	datas[1] = b / 100;
	b = b - datas[1] * 100;
	datas[2] = b / 10;
	b = b - datas[2] * 10;
	datas[3] = b;

	//Change number to its ASCII code.
	datas[0] += 48;
	datas[1] += 48;
	datas[2] += 48;
	datas[3] += 48;

	LCD5110_WriteData(datas[0]);
	LCD5110_WriteData(datas[1]);
	LCD5110_WriteData(datas[2]);
	LCD5110_WriteData(datas[3]);

	//a++;
}

void LCD5110_WriteBMP() {
	LCD5110_draw_bmp_pixel(0,0,AVR_bmp,24,40);
}

static void LCD5110_LCD_write_byte(uint8_t dat, uint8_t command) {
	uint8_t i;
	LCD5110_CS(0);	//SPI_CS = 0;
	delay_us(20);
	if (command == 0)
		LCD5110_DC(0);	//LCD_DC = 0;
	else
		LCD5110_DC(1);	//LCD_DC = 1; - write data to RAM
	delay_us(20);
	for (i = 0; i < 8; i++) {
		LCD5110_MO(dat & 0x80);	//SPI_MO = dat & 0x80;
		dat = dat << 1;
		LCD5110_SCK(0);	//SPI_SCK = 0;
		delay_us(1);
		LCD5110_SCK(1);	//SPI_SCK = 1;
		delay_us(1);
	}
	LCD5110_CS(1);	//SPI_CS = 1;
}

static void LCD5110_GPIO_Config() {
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(LCD5110_GPIO_CLK, ENABLE); // The LEDs are on GPIOD

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;

	GPIO_InitStructure.GPIO_Pin = LCD5110_CS_PIN;
	GPIO_Init(LCD5110_CS_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD5110_RST_PIN;
	GPIO_Init(LCD5110_RST_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD5110_DC_PIN;
	GPIO_Init(LCD5110_DC_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD5110_MO_PIN;
	GPIO_Init(LCD5110_MO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD5110_SCK_PIN;
	GPIO_Init(LCD5110_SCK_PORT, &GPIO_InitStructure);
}

static void LCD5110_CS(uint8_t temp) {
	if (temp) {
		GPIO_SetBits(LCD5110_CS_PORT, LCD5110_CS_PIN );
	} else {
		GPIO_ResetBits(LCD5110_CS_PORT, LCD5110_CS_PIN );
	}
}

static void LCD5110_RST(uint8_t temp) {
	if (temp) {
		GPIO_SetBits(LCD5110_RST_PORT, LCD5110_RST_PIN );
	} else {
		GPIO_ResetBits(LCD5110_RST_PORT, LCD5110_RST_PIN );
	}
}

static void LCD5110_DC(uint8_t temp) {
	if (temp) {
		GPIO_SetBits(LCD5110_DC_PORT, LCD5110_DC_PIN );
	} else {
		GPIO_ResetBits(LCD5110_DC_PORT, LCD5110_DC_PIN );
	}
}

static void LCD5110_MO(uint8_t temp) {
	if (temp) {
		GPIO_SetBits(LCD5110_MO_PORT, LCD5110_MO_PIN );
	} else {
		GPIO_ResetBits(LCD5110_MO_PORT, LCD5110_MO_PIN );
	}
}

static void LCD5110_SCK(uint8_t temp) {
	if (temp) {
		GPIO_SetBits(LCD5110_SCK_PORT, LCD5110_SCK_PIN );
	} else {
		GPIO_ResetBits(LCD5110_SCK_PORT, LCD5110_SCK_PIN );
	}
}

