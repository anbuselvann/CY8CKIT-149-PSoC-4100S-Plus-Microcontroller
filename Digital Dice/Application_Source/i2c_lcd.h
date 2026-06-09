#ifndef I2C_LCD_H
#define I2C_LCD_H

#include <stdint.h>

/* PCF8574 I2C address — check A0/A1/A2 jumpers on your module */
#define LCD_I2C_ADDR   0x27

/* Bit positions on PCF8574 -> LCD */
#define LCD_RS   (1 << 0)
#define LCD_RW   (1 << 1)
#define LCD_EN   (1 << 2)
#define LCD_BL   (1 << 3)   /* Backlight */
#define LCD_D4   (1 << 4)
#define LCD_D5   (1 << 5)
#define LCD_D6   (1 << 6)
#define LCD_D7   (1 << 7)

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char *str);
void LCD_PrintChar(char c);

#endif