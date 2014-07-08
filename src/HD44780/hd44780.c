//hd44780.c
//***************** (c) 2010 BTC Korporacja ***********************************
//                     http://www kamami.com
//
//    THE SOFTWARE INCLUDED IN THIS FILE IS FOR GUIDANCE ONLY.
//    BTC KORPORACJA SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT
//    OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
//    FROM USE OF THIS SOFTWARE.
//
//******************************************************************************
#include "hd44780.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <stdarg.h>
#include "../Delay/delay.h"

#define LCD_GPIO GPIOE
#define LCD_CLK RCC_AHB1Periph_GPIOE

#define LCD_D4 GPIO_Pin_10
#define LCD_D5 GPIO_Pin_11
#define LCD_D6 GPIO_Pin_12
#define LCD_D7 GPIO_Pin_13

#define LCD_RS GPIO_Pin_7
#define LCD_RW GPIO_Pin_8
#define LCD_EN GPIO_Pin_9
GPIO_InitTypeDef GPIO_InitStructure;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_WriteNibble(u8 nibbleToWrite)
{
    GPIO_WriteBit(LCD_GPIO, LCD_EN, Bit_SET);
    GPIO_WriteBit(LCD_GPIO, LCD_D4, (nibbleToWrite & 0x01));
    GPIO_WriteBit(LCD_GPIO, LCD_D5, (nibbleToWrite & 0x02));
    GPIO_WriteBit(LCD_GPIO, LCD_D6, (nibbleToWrite & 0x04));
    GPIO_WriteBit(LCD_GPIO, LCD_D7, (nibbleToWrite & 0x08));
    GPIO_WriteBit(LCD_GPIO, LCD_EN, Bit_RESET);

}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
u8 LCD_ReadNibble(void)
{
    u8 tmp = 0;
    GPIO_WriteBit(LCD_GPIO, LCD_EN, Bit_SET);
    tmp |= (GPIO_ReadInputDataBit(LCD_GPIO, LCD_D4) << 0);
    tmp |= (GPIO_ReadInputDataBit(LCD_GPIO, LCD_D5) << 1);
    tmp |= (GPIO_ReadInputDataBit(LCD_GPIO, LCD_D6) << 2);
    tmp |= (GPIO_ReadInputDataBit(LCD_GPIO, LCD_D7) << 3);
    GPIO_WriteBit(LCD_GPIO, LCD_EN, Bit_RESET);
    return tmp;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
u8 LCD_ReadStatus(void)
{
    u8 status = 0;

    GPIO_InitStructure.GPIO_Pin   =  LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7;
    GPIO_InitStructure.GPIO_Mode  =  GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(LCD_GPIO, &GPIO_InitStructure);

    GPIO_WriteBit(LCD_GPIO, LCD_RW, Bit_SET);
    GPIO_WriteBit(LCD_GPIO, LCD_RS, Bit_RESET);

    status |= (LCD_ReadNibble() << 4);
    status |= LCD_ReadNibble();

    GPIO_InitStructure.GPIO_Pin   =  LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(LCD_GPIO, &GPIO_InitStructure);

    return status;
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_WriteData(u8 dataToWrite)
{
    GPIO_WriteBit(LCD_GPIO, LCD_RW, Bit_RESET);
    GPIO_WriteBit(LCD_GPIO, LCD_RS, Bit_SET);

    LCD_WriteNibble(dataToWrite >> 4);
    LCD_WriteNibble(dataToWrite & 0x0F);

    while(LCD_ReadStatus() & 0x80);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_WriteCommand(u8 commandToWrite)
{
    GPIO_WriteBit(LCD_GPIO, LCD_RW | LCD_RS, Bit_RESET);
    LCD_WriteNibble(commandToWrite >> 4);
    LCD_WriteNibble(commandToWrite & 0x0F);

    while(LCD_ReadStatus() & 0x80);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_DefineCharacter(char index, const char * pattern)
{
    char i ;
    LCD_WriteCommand(HD44780_CGRAM_SET + (8 * index));
    for(i = 0; i < 8; i++)
        LCD_WriteData(*(pattern + i));
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void LCD_WriteText(char * text)
{
    while((*text !=0) && (*text != 13))
        LCD_WriteData(*text++);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_GoTo(unsigned char x, unsigned char y)
{
    LCD_WriteCommand(HD44780_DDRAM_SET | (x + (0x40 * y)));
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_WriteTextXY(char * text, u8 x, u8 y)
{
    LCD_GoTo(x,y);
    LCD_WriteText(text);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

void LCD_Clear(void)
{
    LCD_WriteCommand(HD44780_CLEAR);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_WriteBinary(u32 var, u8 bitCount)
{
    s8 i;

    for(i = (bitCount - 1); i >= 0; i--)
    {
        LCD_WriteData((var & (1 << i))?'1':'0');
    }
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_ShiftLeft(void)
{
    LCD_WriteCommand(HD44780_DISPLAY_CURSOR_SHIFT | HD44780_SHIFT_LEFT | HD44780_SHIFT_DISPLAY);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_ShiftRight(void)
{
    LCD_WriteCommand(HD44780_DISPLAY_CURSOR_SHIFT | HD44780_SHIFT_RIGHT | HD44780_SHIFT_DISPLAY);
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void LCD_Initialize(void)
{
    vu8 i = 0;
    RCC_AHB1PeriphClockCmd (LCD_CLK, ENABLE);
    GPIO_InitStructure.GPIO_Pin=LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7 | LCD_RS | LCD_RW | LCD_EN;
    GPIO_InitStructure.GPIO_OType= GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd= GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Init (LCD_GPIO, & GPIO_InitStructure);
    GPIO_Write (LCD_GPIO,0);

    GPIO_ResetBits(LCD_GPIO, LCD_RS | LCD_EN | LCD_RW);

    delay_ms(20);

    for(i = 0; i < 3; i++)
    {
        LCD_WriteNibble(0x03);
        delay_ms(4);
    }

    LCD_WriteNibble(0x02);

    delay_ms(1);

    LCD_WriteCommand(HD44780_FUNCTION_SET |
                     HD44780_FONT5x7 |
                     HD44780_TWO_LINE |
                     HD44780_4_BIT);

    LCD_WriteCommand(HD44780_DISPLAY_ONOFF |
                     HD44780_DISPLAY_OFF);

    LCD_WriteCommand(HD44780_CLEAR);

    LCD_WriteCommand(HD44780_ENTRY_MODE |
                     HD44780_EM_SHIFT_CURSOR |
                     HD44780_EM_INCREMENT);

    LCD_WriteCommand(HD44780_DISPLAY_ONOFF |
                     HD44780_DISPLAY_ON |
                     HD44780_CURSOR_OFF |
                     HD44780_CURSOR_NOBLINK);
}

void LCD_CreateCustomCharsForLevels(void) {
	const char L0[8] = {0, 0, 0, 0, 0, 0, 0, 31};
	const char L1[8] = {0, 0, 0, 0, 0, 0, 31, 31};
	const char L2[8] = {0, 0, 0, 0, 0, 31, 31, 31};
	const char L3[8] = {0, 0, 0, 0, 31, 31, 31, 31};
	const char L4[8] = {0, 0, 0, 31, 31, 31, 31, 31};
	const char L5[8] = {0, 0, 31, 31, 31, 31, 31, 31};
	const char L6[8] = {0, 31, 31, 31, 31, 31, 31, 31};
	const char L7[8] = {31, 31, 31, 31, 31, 31, 31, 31};

	LCD_DefineCharacter(0,L0);
	LCD_DefineCharacter(1,L1);
	LCD_DefineCharacter(2,L2);
	LCD_DefineCharacter(3,L3);
	LCD_DefineCharacter(4,L4);
	LCD_DefineCharacter(5,L5);
	LCD_DefineCharacter(6,L6);
	LCD_DefineCharacter(7,L7);

}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
//
