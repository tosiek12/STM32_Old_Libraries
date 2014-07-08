#ifndef NOKIALCD_H_
#define NOKIALCD_H_
//Folder: Nokia_5110_Display_Lib_for_STM32_GCC\LCD5110_Lib_Ifalt32-2.0

void LCD5110_Initialize(void);

void LCD5110_WriteData(uint8_t c);
void LCD5110_WriteDataInversed(uint8_t c);

void LCD5110_Clear(void);
void LCD5110_ClearLine(uint8_t line);

void LCD5110_GoTo(uint8_t X,uint8_t Y);

void LCD5110_WriteText(char *s);
void LCD5110_WriteTextInversed(char *s);
void LCD5110_WriteTextXY(char *s, uint8_t X, uint8_t Y);

void LCD5110_WriteNumberInDec(uint16_t b);

void LCD5110_WriteBMP();

#endif



