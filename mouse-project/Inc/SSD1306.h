// SSD1306 128*64 OLED Display
//
// 4 mouse  04-03-2018
//



#include "stm32f1xx_hal.h"
#include "stdlib.h"


#include "font.h"


// Software SPI

#define SCK_H  (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET))
#define SCK_L  (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET))

#define DAT_H  (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET))
#define DAT_L  (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET))

#define DC_H   (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET))
#define DC_L   (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET))

#define CS_H   (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET))
#define CS_L   (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET))

inline void oled_io_init(void)
    {
    GPIO_InitTypeDef GPIO_InitStruct;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

inline void oled_io_deinit(void)
    {
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

//screen buffer

#define BUFF_SIZE  1024  //columns*strings(pages)=128*8=1024

uint8_t scrBuff[BUFF_SIZE];


char strbuff[32]={0};


//data/command

#define DATA_MODE     DC_H
#define COMMAND_MODE  DC_L


//chip select

#define CS_ACTIVE    CS_L
#define CS_INACTIVE  CS_H


//commands for the initialization of SSD1306

#define INIT_SIZE  25

const uint8_t init[INIT_SIZE] =
{
0xae, //display off sleep mode

0xd5, //display clock divide
0x80, //

0xa8, //set multiplex ratio
0x3f, //

0xd3, //display offset
0x00, //

0x40, //set display start line

0x8d, //charge pump setting
0x14, //0x10 //0x14 - internal

0x20, //memory addressing mode
0x00, //horizontal addressing mode

0xa1, //segment re-map
0xc8, //COM output scan direction

0xda, //COM pins hardware configuration
0x12, //

0x81, //set contrast (brightness)
0xc8, //8f //cf //0..255

0xd9, //pre-charge period
0xf1, //

0xdb, //VCOMH deselect level
0x20, //

0xa4,  // Entire Display ON disable

0xa6, //normal display, 0xa7 inverse display

0xaf  //display turned on
};


void oled_write(uint8_t data);  //write byte
void oled_init(void); //init display
void oled_off(void);
void oled_bright(uint8_t val);

//operations with the screen buffer

void oled_clear(void); //clear screen
void oled_fill(void);
void oled_update(void); //write buffer to screen

void oled_pixel(uint8_t h, uint8_t y);  //draw pixel  //x: 0..127  //y: 0..63
void oled_pixel_off(uint8_t x, uint8_t y);  //x: 0..127  //y: 0..63

void oled_v(uint8_t x, uint8_t y, uint8_t db);
void oled_h(uint8_t x, uint8_t y, uint8_t db);

void oled_char(uint8_t x, uint8_t y, uint8_t sign);  //x: 0..127 //y: 0..63
void oled_print(uint8_t x, uint8_t y, char *str);

void oled_char_inv(uint8_t x, uint8_t y, uint8_t l);  //x: 0..127 //y: 0..63 //l: 1..128

void oled_num16x30(uint8_t x, uint8_t y, uint8_t sign);  //print 16x30pixels digit
void oled_print16x30(uint8_t x, uint8_t y, char *str);

uint8_t oled_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2); //draw line //x: 0..127  //y: 0..63



//-------------------------------------------------------------------------------------------------
void oled_write(uint8_t data)  //write byte
    {
    for(uint8_t k=0x80; k; k>>=1)
        {
        SCK_L;
        (data & k) ? DAT_H : DAT_L; //data line
        SCK_H;
        }
    }


//-------------------------------------------------------------------------------------------------
void oled_init(void) //init display
    {
    oled_io_init();

    CS_INACTIVE;
    CS_ACTIVE;
    COMMAND_MODE;

    for(uint8_t k=0; k<INIT_SIZE; k++) oled_write(init[k]);

    CS_INACTIVE;

    oled_clear();
    oled_update();
    }


//-------------------------------------------------------------------------------------------------
void oled_off(void) //display sleep
    {
    CS_ACTIVE;
    COMMAND_MODE;

    oled_write(0xae); //display off sleep mode

    CS_INACTIVE;
    }


//-------------------------------------------------------------------------------------------------
void oled_bright(uint8_t val)
    {
    CS_ACTIVE;
    COMMAND_MODE;

    oled_write(0x81); //set contrast (brightness)
    oled_write(val);

    CS_INACTIVE;
    }


//-------------------------------------------------------------------------------------------------
void oled_clear(void)  //clear buffer
    {
    for(uint16_t k=0; k<BUFF_SIZE; k++) scrBuff[k]=0;
    }


//-------------------------------------------------------------------------------------------------
void oled_fill(void)
    {
    for(uint16_t k=0; k<BUFF_SIZE; k++) scrBuff[k]=0xff;
    }


//-------------------------------------------------------------------------------------------------
void oled_update(void)  //write buffer to screen
    {
    CS_ACTIVE;
    COMMAND_MODE;

    oled_write(0x21); //set column address
    oled_write(0);    //start address
    oled_write(127);  //end address

    oled_write(0x22); //set page address
    oled_write(0);
    oled_write(7);

    DATA_MODE;    // send data to screen

    for(uint16_t k=0; k<BUFF_SIZE; k++) oled_write(scrBuff[k]);  //write

    CS_INACTIVE;
    }


//-------------------------------------------------------------------------------------------------
void oled_pixel(uint8_t x, uint8_t y)  //x: 0..127  //y: 0..63
    {
    if(x<=127 && y<=63) scrBuff[x+128*(y/8)] |= 1<<(y%8);
    }


//-------------------------------------------------------------------------------------------------
void oled_pixel_off(uint8_t x, uint8_t y)  //x: 0..127  //y: 0..63
    {
    if(x<=127 && y<=63) scrBuff[x+128*(y/8)] &= ~(1<<(y%8));
    }


//-------------------------------------------------------------------------------------------------
void oled_pixel_inv(uint8_t x, uint8_t y)  //x: 0..127  //y: 0..63
    {
    if(x<=127 && y<=63) scrBuff[x+128*(y/8)] ^= 1<<(y%8);
    }


//-------------------------------------------------------------------------------------------------
void oled_v(uint8_t x, uint8_t y, uint8_t db)  //db - data byte
    {
    for(uint8_t i=0; i<8; i++)
        {
        if( (db >> (7-i)) & 1 ) oled_pixel(x, y-i);
        //else oled_pixel_off(x, y-i);
        }
    }


//-------------------------------------------------------------------------------------------------
void oled_h(uint8_t x, uint8_t y, uint8_t db)  //db - data byte
    {
    for(uint8_t i=0; i<8; i++)
        {
        if( (db >> (7-i)) & 1 ) oled_pixel(x+i, y);
        //else oled_pixel_off(x+i, y);
        }
    }


//-------------------------------------------------------------------------------------------------
void oled_char(uint8_t x, uint8_t y, uint8_t sign)  //x: 0..127 //y: 0..63
    {
    if(sign<32 || sign>127) sign=128;

    for(uint8_t i=0; i<5; i++) oled_v(x+i,y,font[5*(sign-32)+i]);
    }


//-------------------------------------------------------------------------------------------------
void oled_char_inv(uint8_t x, uint8_t y, uint8_t l)  //x: 0..127 //y: 0..63 //l: 1..128
    {
    for(uint8_t k=0; k<l; k++)
        {
        for(uint8_t i=0; i<9; i++)
            {
            oled_pixel_inv(x+k, y-i);
            }
        }
    }


//-------------------------------------------------------------------------------------------------
void oled_print(uint8_t x, uint8_t y, char *str)
    {
    for(; (*str && x<=127 && y<=63); x+=6) oled_char(x, y, *str++);
    }


//-------------------------------------------------------------------------------------------------
void oled_num16x30(uint8_t x, uint8_t y, uint8_t sign)  //print 16x30pixels digit   //
    {
    for(uint8_t i=0; (i<60) && (sign>=48) && (sign<=57); i+=2)
        {
        oled_h(x, y-30+i/2, num16x30[i+(sign-48)*60]);
        oled_h(x+8, y-30+i/2, num16x30[i+1+(sign-48)*60]);
        }
    }


//-------------------------------------------------------------------------------------------------
void oled_print16x30(uint8_t x, uint8_t y, char *str)
    {
    for(; (*str && x<=120); x+=16) oled_num16x30(x, y, *str++);
    }


//-------------------------------------------------------------------------------------------------
uint8_t oled_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)  //x: 0..127  //y: 0..63
    {
    if(x1>127 || y1>63 || x2>127 || y2>63) return 1;

    int16_t p;

    int8_t x = x1;
    int8_t y = y1;

    int8_t dx = abs((int8_t)(x2-x1));
    int8_t dy = abs((int8_t)(y2-y1));

    int8_t addx, addy;

    if(x1>x2) addx=-1;
    else addx=1;

    if(y1>y2) addy=-1;
    else addy=1;

    if(dx>=dy)
        {
        p=2*dy-dx;

        for(uint8_t k=0; k<=dx; k++)
            {
            oled_pixel(x, y);

            if(p<0)
                {
                p+=2*dy;
                x+=addx;
                }
            else
                {
                p+=2*dy-2*dx;
                x+=addx;
                y+=addy;
                }
            }
        }
    else
        {
        p=2*dx-dy;

        for(uint8_t k=0; k<=dy; k++)
            {
            oled_pixel(x, y);

            if(p<0)
                {
                p+=2*dx;
                y+=addy;
                }
            else
                {
                p+=2*dx-2*dy;
                x+=addx;
                y+=addy;
                }
            }
        }
    return 0;
    }

