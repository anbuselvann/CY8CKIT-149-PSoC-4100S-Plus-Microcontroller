#include "i2c_lcd.h"
#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  I2C bit-bang on P3.0 = SCL, P3.1 = SDA                            */
/* ------------------------------------------------------------------ */
#define GPIO_PRT3_BASE  0x40040300UL
#define DR3   (*((volatile uint32_t *)(GPIO_PRT3_BASE + 0x00)))
#define PS3   (*((volatile uint32_t *)(GPIO_PRT3_BASE + 0x04)))
#define PC3   (*((volatile uint32_t *)(GPIO_PRT3_BASE + 0x08)))

#define SCL_PIN  0   /* P3.0 */
#define SDA_PIN  1   /* P3.1 */

#define SCL_HIGH()  (DR3 |=  (1u << SCL_PIN))
#define SCL_LOW()   (DR3 &= ~(1u << SCL_PIN))
#define SDA_HIGH()  (DR3 |=  (1u << SDA_PIN))
#define SDA_LOW()   (DR3 &= ~(1u << SDA_PIN))

static void i2c_delay(void)
{
    for (volatile uint32_t i = 0; i < 50; i++);
}

static void i2c_init_pins(void)
{
    /* P3.0 SCL, P3.1 SDA: Open-drain (drive mode 4 = open-drain drives low) */
    /* PC bits: 3 bits per pin. Pin0 = bits[2:0], Pin1 = bits[5:3] */
    PC3 = (PC3 & ~(0x3Fu)) | (4u << 0) | (4u << 3);
    DR3 |= (1u << SCL_PIN) | (1u << SDA_PIN); /* both high */
}

static void i2c_start(void)
{
    SDA_HIGH(); SCL_HIGH(); i2c_delay();
    SDA_LOW();  i2c_delay();
    SCL_LOW();  i2c_delay();
}

static void i2c_stop(void)
{
    SDA_LOW();  i2c_delay();
    SCL_HIGH(); i2c_delay();
    SDA_HIGH(); i2c_delay();
}

static void i2c_write_bit(uint8_t bit)
{
    if (bit) SDA_HIGH(); else SDA_LOW();
    i2c_delay();
    SCL_HIGH(); i2c_delay();
    SCL_LOW();  i2c_delay();
}

static uint8_t i2c_read_ack(void)
{
    SDA_HIGH(); i2c_delay();
    SCL_HIGH(); i2c_delay();
    uint8_t ack = !((PS3 >> SDA_PIN) & 1u); /* ACK = SDA pulled low by slave */
    SCL_LOW();  i2c_delay();
    return ack;
}

static void i2c_write_byte(uint8_t byte)
{
    for (int8_t i = 7; i >= 0; i--)
        i2c_write_bit((byte >> i) & 1u);
    i2c_read_ack(); /* read and discard ACK */
}

static void pcf8574_write(uint8_t data)
{
    i2c_start();
    i2c_write_byte((LCD_I2C_ADDR << 1) | 0u); /* write mode */
    i2c_write_byte(data);
    i2c_stop();
}

/* ------------------------------------------------------------------ */
/*  HD44780 via PCF8574 (4-bit mode)                                   */
/* ------------------------------------------------------------------ */
static uint8_t backlight = LCD_BL;

static void lcd_pulse_enable(uint8_t data)
{
    pcf8574_write(data | LCD_EN);
    i2c_delay();
    pcf8574_write(data & ~LCD_EN);
    i2c_delay();
}

static void lcd_send_nibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data = backlight | mode;
    data |= ((nibble & 0x08) ? LCD_D7 : 0);
    data |= ((nibble & 0x04) ? LCD_D6 : 0);
    data |= ((nibble & 0x02) ? LCD_D5 : 0);
    data |= ((nibble & 0x01) ? LCD_D4 : 0);
    lcd_pulse_enable(data);
}

static void lcd_send_byte(uint8_t byte, uint8_t mode)
{
    lcd_send_nibble(byte >> 4, mode);   /* high nibble first */
    lcd_send_nibble(byte & 0x0F, mode); /* low nibble */
    /* command needs more time */
    for (volatile uint32_t i = 0; i < 2000; i++);
}

static void lcd_cmd(uint8_t cmd)  { lcd_send_byte(cmd, 0u); }
static void lcd_data(uint8_t dat) { lcd_send_byte(dat, LCD_RS); }

void LCD_Init(void)
{
    i2c_init_pins();
    /* Wait for LCD power-up */
    for (volatile uint32_t i = 0; i < 500000; i++);

    /* Initialization sequence for 4-bit mode */
    lcd_send_nibble(0x03, 0); for (volatile uint32_t i=0;i<40000;i++);
    lcd_send_nibble(0x03, 0); for (volatile uint32_t i=0;i<4000;i++);
    lcd_send_nibble(0x03, 0); for (volatile uint32_t i=0;i<4000;i++);
    lcd_send_nibble(0x02, 0); /* switch to 4-bit */

    lcd_cmd(0x28); /* 4-bit, 2 lines, 5x8 font */
    lcd_cmd(0x0C); /* Display on, cursor off */
    lcd_cmd(0x06); /* Auto-increment, no shift */
    lcd_cmd(0x01); /* Clear display */
    for (volatile uint32_t i = 0; i < 20000; i++); /* clear needs 1.6ms */
}

void LCD_Clear(void)
{
    lcd_cmd(0x01);
    for (volatile uint32_t i = 0; i < 20000; i++);
}

void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    lcd_cmd(0x80 | (addr + col));
}

void LCD_PrintChar(char c)
{
    lcd_data((uint8_t)c);
}

void LCD_Print(const char *str)
{
    while (*str)
        lcd_data((uint8_t)(*str++));
}