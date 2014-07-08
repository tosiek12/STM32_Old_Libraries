/*
 * stm32f4_discovery_LIS3DSH.c
 *
 *  Created on: Dec 26, 2013
 *      Author: Pawel
 *
 *      IsNo 2013
 */

#include "stm32f4_discovery_LIS3DSH.h"

__IO uint32_t  LIS3DSHTimeout = LIS3DSH_FLAG_TIMEOUT;

/* Read/Write command */
#define READWRITE_CMD              ((uint8_t)0x80)
/* Multiple byte read/write command */
#define MULTIPLEBYTE_CMD           ((uint8_t)0x40)
/* Dummy Byte Send by the SPI Master device in order to generate the Clock to the Slave device */
#define DUMMY_BYTE                 ((uint8_t)0x00)

static uint8_t _LIS3DSH_SendByte(uint8_t byte);
static void LIS3DSH_CalcAcceleration(LIS3DSH_OutXYZTypeDef* axes);

void LIS3DSH_SetOutputDataRate(uint8_t odr_selection)
{
	uint8_t tmp;
	tmp=LIS3DSH_ReadByte(LIS3DSH_CTRL_REG4_ADDR);
	tmp&=0x0f;
	tmp|=odr_selection;
	LIS3DSH_WriteByte(LIS3DSH_CTRL_REG4_ADDR,tmp);
}
void LIS3DSH_SetFilterBandwidth(uint8_t filter_selection)
{
	uint8_t tmp;
	tmp=LIS3DSH_ReadByte(LIS3DSH_CTRL_REG5_ADDR);
	tmp&=(~_LIS3DSH_BW_MASK);
	tmp|=filter_selection;
	LIS3DSH_WriteByte(LIS3DSH_CTRL_REG5_ADDR,tmp);
}
void LIS3DSH_SetMeasuredRange(uint8_t range_selection)
{
	uint8_t tmp;
	tmp=LIS3DSH_ReadByte(LIS3DSH_CTRL_REG5_ADDR);
	tmp&=(~_LIS3DSH_FSCALE_MASK);
	tmp|=range_selection;
	LIS3DSH_WriteByte(LIS3DSH_CTRL_REG5_ADDR,tmp);
}
void LIS3DSH_SetOffsets(uint8_t xOffset, uint8_t yOffset, uint8_t zOffset) {
	LIS3DSH_WriteByte(LIS3DSH_OFFSET_X_ADDR, xOffset);
	LIS3DSH_WriteByte(LIS3DSH_OFFSET_Y_ADDR, yOffset);
	LIS3DSH_WriteByte(LIS3DSH_OFFSET_Z_ADDR, zOffset);
}
void LIS3DSH_AxesEnable(uint8_t axes)
{
	uint8_t tmp;
	tmp=LIS3DSH_ReadByte(LIS3DSH_CTRL_REG4_ADDR);
	tmp&=0xf8;
	tmp|=axes;
	LIS3DSH_WriteByte(LIS3DSH_CTRL_REG4_ADDR,tmp);
}
void LIS3DSH_Int1Enable()
{
	uint8_t tmp;
	tmp=LIS3DSH_ReadByte(LIS3DSH_CTRL_REG3_ADDR);
	tmp|=(1<<_LIS3DSH_DR_EN)|
		 (1<<_LIS3DSH_IEA)  |
		 (1<<_LIS3DSH_INT1_EN);
	LIS3DSH_WriteByte(LIS3DSH_CTRL_REG3_ADDR,tmp);
}

static void LIS3DSH_CalcAcceleration(LIS3DSH_OutXYZTypeDef* axes)
{
	uint8_t ctrl = 0;
	ctrl = LIS3DSH_ReadByte(LIS3DSH_CTRL_REG5_ADDR);
	ctrl = (ctrl&_LIS3DSH_FSCALE_MASK);
	switch (ctrl) {
		case _LIS3DSH_FSCALE_2G:
			axes->x = (int16_t)(LIS3DSH_2G_FACTOR*(axes->x));	//(in +/- mg)
			axes->y = (int16_t)(LIS3DSH_2G_FACTOR*(axes->y));	//(in +/- mg)
			axes->z = (int16_t)(LIS3DSH_2G_FACTOR*(axes->z));	//(in +/- mg)
			break;
		case _LIS3DSH_FSCALE_4G:
			axes->x = (int16_t)(LIS3DSH_4G_FACTOR*(axes->x));	//(in +/- mg)
			axes->y = (int16_t)(LIS3DSH_4G_FACTOR*(axes->y));	//(in +/- mg)
			axes->z = (int16_t)(LIS3DSH_4G_FACTOR*(axes->z));	//(in +/- mg)
			break;
		case _LIS3DSH_FSCALE_6G:
			axes->x = (int16_t)(LIS3DSH_6G_FACTOR*(axes->x));	//(in +/- mg)
			axes->y = (int16_t)(LIS3DSH_6G_FACTOR*(axes->y));	//(in +/- mg)
			axes->z = (int16_t)(LIS3DSH_6G_FACTOR*(axes->z));	//(in +/- mg)
			break;
		case _LIS3DSH_FSCALE_8G:
			axes->x = (int16_t)(LIS3DSH_8G_FACTOR*(axes->x));	//(in +/- mg)
			axes->y = (int16_t)(LIS3DSH_8G_FACTOR*(axes->y));	//(in +/- mg)
			axes->z = (int16_t)(LIS3DSH_8G_FACTOR*(axes->z));	//(in +/- mg)
			break;
		case _LIS3DSH_FSCALE_16G:
			axes->x = (int16_t)(LIS3DSH_16G_FACTOR*(axes->x));	//(in +/- mg)
			axes->y = (int16_t)(LIS3DSH_16G_FACTOR*(axes->y));	//(in +/- mg)
			axes->z = (int16_t)(LIS3DSH_16G_FACTOR*(axes->z));	//(in +/- mg)
			break;
		default:
			break;
	}
}
void LIS3DSH_ReadAxes(LIS3DSH_OutXYZTypeDef* axes)
{
	uint8_t tmp=0;
	/* Wait for new set of data */
	while(tmp==0) {
		tmp=LIS3DSH_ReadByte(LIS3DSH_STATUS_ADDR);
		tmp&=(1<<_LIS3DSH_ZYXDA);	//ZYX data available
	}
	LIS3DSH_Read((uint8_t*)axes,LIS3DSH_OUT_X_L_ADDR,6);
	axes->x += 3107;	//programowa korekcja offsetu (wartosc byla za duza, by usunac sprzetowo)
	axes->y -= 3;	//te juz sa dosc blisko, wiec nie ma duzej potrzeby by to robic
	axes->z -= 3;
	LIS3DSH_CalcAcceleration(axes);
}

uint8_t LIS3DSH_ReadByte(uint8_t addres)
{
	uint8_t buffer;

	addres|=(uint8_t)READWRITE_CMD;
	LIS3DSH_CS_LOW();
	_LIS3DSH_SendByte(addres);
	buffer=_LIS3DSH_SendByte(DUMMY_BYTE);
	LIS3DSH_CS_HIGH();

	return buffer;
}
void LIS3DSH_Read(uint8_t* buf, uint8_t addres, uint16_t size)
{
	while(size > 0x00)
	  {
	    *buf = LIS3DSH_ReadByte(addres);
	    size--;
	    buf++;
	    addres++;
	  }
}

void LIS3DSH_WriteByte(uint8_t addres,uint8_t value)
{
	LIS3DSH_CS_LOW();
	_LIS3DSH_SendByte(addres);
	_LIS3DSH_SendByte(value);
	LIS3DSH_CS_HIGH();
}
void LIS3DSH_Write(uint8_t* buf,uint8_t addres,uint8_t size)
{
	while(size > 0x00)
	{
		LIS3DSH_WriteByte(addres,*buf);
		size--;
		buf++;
		addres++;
	}
}

void LIS3DSH_Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;

// Enable the SPI
	RCC_APB2PeriphClockCmd(LIS3DSH_SPI_CLK, ENABLE);
	RCC_AHB1PeriphClockCmd(LIS3DSH_SPI_SCK_GPIO_CLK | LIS3DSH_SPI_MISO_GPIO_CLK | LIS3DSH_SPI_MOSI_GPIO_CLK, ENABLE);
	RCC_AHB1PeriphClockCmd(LIS3DSH_SPI_CS_GPIO_CLK, ENABLE);
// Enable INT1 GPIO clock
	RCC_AHB1PeriphClockCmd(LIS3DSH_SPI_INT1_GPIO_CLK, ENABLE);
// Enable INT2 GPIO clock
	RCC_AHB1PeriphClockCmd(LIS3DSH_SPI_INT2_GPIO_CLK, ENABLE);
	GPIO_PinAFConfig(LIS3DSH_SPI_SCK_GPIO_PORT, LIS3DSH_SPI_SCK_SOURCE, LIS3DSH_SPI_SCK_AF);
	GPIO_PinAFConfig(LIS3DSH_SPI_MISO_GPIO_PORT, LIS3DSH_SPI_MISO_SOURCE, LIS3DSH_SPI_MISO_AF);
	GPIO_PinAFConfig(LIS3DSH_SPI_MOSI_GPIO_PORT, LIS3DSH_SPI_MOSI_SOURCE, LIS3DSH_SPI_MOSI_AF);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

// SPI SCK pin configuration
	GPIO_InitStructure.GPIO_Pin = LIS3DSH_SPI_SCK_PIN;
	GPIO_Init(LIS3DSH_SPI_SCK_GPIO_PORT, &GPIO_InitStructure);
// SPI  MOSI pin configuration
	GPIO_InitStructure.GPIO_Pin =  LIS3DSH_SPI_MOSI_PIN;
	GPIO_Init(LIS3DSH_SPI_MOSI_GPIO_PORT, &GPIO_InitStructure);
// SPI MISO pin configuration
	GPIO_InitStructure.GPIO_Pin = LIS3DSH_SPI_MISO_PIN;
	GPIO_Init(LIS3DSH_SPI_MISO_GPIO_PORT, &GPIO_InitStructure);

// SPI configuration ----------------------------------------------------------------------------
	SPI_I2S_DeInit(LIS3DSH_SPI);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_Init(LIS3DSH_SPI, &SPI_InitStructure);
	// Enable SPI1
	SPI_Cmd(LIS3DSH_SPI, ENABLE);

	// Configure GPIO PIN for Lis Chip select */
	GPIO_InitStructure.GPIO_Pin = LIS3DSH_SPI_CS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LIS3DSH_SPI_CS_GPIO_PORT, &GPIO_InitStructure);

//Deselect : Chip Select high */
	LIS3DSH_CS_HIGH();

// Configure GPIO PINs to detect Interrupts */
	GPIO_InitStructure.GPIO_Pin = LIS3DSH_SPI_INT1_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(LIS3DSH_SPI_INT1_GPIO_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = LIS3DSH_SPI_INT2_PIN;
	GPIO_Init(LIS3DSH_SPI_INT2_GPIO_PORT, &GPIO_InitStructure);
}

static uint8_t _LIS3DSH_SendByte(uint8_t byte)
{
  /* Loop while DR register in not emplty */
	LIS3DSHTimeout = LIS3DSH_FLAG_TIMEOUT;
  while (SPI_I2S_GetFlagStatus(LIS3DSH_SPI, SPI_I2S_FLAG_TXE) == RESET)
  {
    if((LIS3DSHTimeout--) == 0) return LIS3DSH_Timeout_UserCallback();
  }

  /* Send a Byte through the SPI peripheral */
  SPI_I2S_SendData(LIS3DSH_SPI, byte);

  /* Wait to receive a Byte */
  LIS3DSHTimeout = LIS3DSH_FLAG_TIMEOUT;
  while (SPI_I2S_GetFlagStatus(LIS3DSH_SPI, SPI_I2S_FLAG_RXNE) == RESET)
  {
    if((LIS3DSHTimeout--) == 0) return LIS3DSH_Timeout_UserCallback();
  }

  /* Return the Byte read from the SPI bus */
  return (uint8_t)SPI_I2S_ReceiveData(LIS3DSH_SPI);
}
uint8_t LIS3DSH_Timeout_UserCallback(void)
{
/* Block communication and all processes */
  while (1)
  {
  }
}
