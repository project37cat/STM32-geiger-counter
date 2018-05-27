// SSD1306 128*64 OLED Display
//
// ssd1306.h // Mouse project // 25-May-2018
//


#ifndef SSD1306_H_
#define SSD1306_H_


#include "stdint.h"


extern const uint8_t font5x7[];
extern const uint8_t num16x30[];



void oled_io_init(void);
void oled_io_deinit(void);

void oled_write(uint8_t data);  //write byte
void oled_init(void);  //init display
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


#endif  // end of SSD1306_H_

