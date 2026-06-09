#include <stdint.h>

uint8_t app_heap[512]   __attribute__((section(".heap")));
uint8_t app_stack[4096] __attribute__((section(".stack")));

/*------------------------------------------------------------------
 *  I2C Bit-bang: SCL = P3.0, SDA = P3.1
 *------------------------------------------------------------------*/
#define GPIO_PRT3_DR  (*((volatile uint32_t *)0x40040300))
#define GPIO_PRT3_PS  (*((volatile uint32_t *)0x40040304))
#define GPIO_PRT3_PC  (*((volatile uint32_t *)0x40040308))

#define SCL_PIN  0u
#define SDA_PIN  1u

#define SCL_HIGH()  (GPIO_PRT3_DR |=  (1u << SCL_PIN))
#define SCL_LOW()   (GPIO_PRT3_DR &= ~(1u << SCL_PIN))
#define SDA_HIGH()  (GPIO_PRT3_DR |=  (1u << SDA_PIN))
#define SDA_LOW()   (GPIO_PRT3_DR &= ~(1u << SDA_PIN))
#define SDA_READ()  ((GPIO_PRT3_PS >> SDA_PIN) & 1u)

/*------------------------------------------------------------------
 *  PCF8574 I2C address — 0x27 is most common.
 *  If LCD shows nothing, try 0x3F
 *------------------------------------------------------------------*/
#define LCD_ADDR  0x27

/* PCF8574 bit mapping to LCD */
#define LCD_RS  (1u << 0)
#define LCD_EN  (1u << 2)
#define LCD_BL  (1u << 3)   /* backlight — keep always ON */
#define LCD_D4  (1u << 4)
#define LCD_D5  (1u << 5)
#define LCD_D6  (1u << 6)
#define LCD_D7  (1u << 7)

/*------------------------------------------------------------------
 *  Delays
 *------------------------------------------------------------------*/
static void Delay(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++);
}

static void I2C_Delay(void)
{
    for (volatile uint32_t i = 0; i < 50u; i++);
}

/*------------------------------------------------------------------
 *  I2C primitives
 *------------------------------------------------------------------*/
static void I2C_Init(void)
{
    /* P3.0 and P3.1: open-drain (drive mode 4), default HIGH */
    GPIO_PRT3_PC = (GPIO_PRT3_PC & ~(0x3Fu)) | (4u << 0) | (4u << 3);
    GPIO_PRT3_DR |= (1u << SCL_PIN) | (1u << SDA_PIN);
}

static void I2C_Start(void)
{
    SDA_HIGH(); SCL_HIGH(); I2C_Delay();
    SDA_LOW();              I2C_Delay();
    SCL_LOW();              I2C_Delay();
}

static void I2C_Stop(void)
{
    SDA_LOW();  I2C_Delay();
    SCL_HIGH(); I2C_Delay();
    SDA_HIGH(); I2C_Delay();
}

static void I2C_WriteBit(uint8_t bit)
{
    if (bit) SDA_HIGH(); else SDA_LOW();
    I2C_Delay();
    SCL_HIGH(); I2C_Delay();
    SCL_LOW();  I2C_Delay();
}

static void I2C_WriteByte(uint8_t byte)
{
    for (int8_t i = 7; i >= 0; i--)
        I2C_WriteBit((byte >> i) & 1u);
    /* read ACK (discard) */
    SDA_HIGH(); I2C_Delay();
    SCL_HIGH(); I2C_Delay();
    SCL_LOW();  I2C_Delay();
}

static void PCF8574_Write(uint8_t data)
{
    I2C_Start();
    I2C_WriteByte((LCD_ADDR << 1u) | 0u);  /* address + write bit */
    I2C_WriteByte(data);
    I2C_Stop();
}

/*------------------------------------------------------------------
 *  HD44780 LCD via PCF8574 (4-bit mode)
 *------------------------------------------------------------------*/
static void LCD_PulseEnable(uint8_t data)
{
    PCF8574_Write(data | LCD_EN);
    I2C_Delay();
    PCF8574_Write(data & ~LCD_EN);
    Delay(2000);
}

static void LCD_SendNibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data = LCD_BL | mode;
    if (nibble & 0x08u) data |= LCD_D7;
    if (nibble & 0x04u) data |= LCD_D6;
    if (nibble & 0x02u) data |= LCD_D5;
    if (nibble & 0x01u) data |= LCD_D4;
    LCD_PulseEnable(data);
}

static void LCD_SendByte(uint8_t byte, uint8_t mode)
{
    LCD_SendNibble(byte >> 4u,   mode);   /* high nibble */
    LCD_SendNibble(byte & 0x0Fu, mode);   /* low nibble  */
    Delay(2000);
}

static void LCD_Cmd(uint8_t cmd)       { LCD_SendByte(cmd,  0u);     }
static void LCD_Char(uint8_t ch)       { LCD_SendByte(ch,   LCD_RS); }

static void LCD_Init(void)
{
    Delay(500000);                          /* power-up wait            */
    LCD_SendNibble(0x03u, 0u); Delay(50000);
    LCD_SendNibble(0x03u, 0u); Delay(5000);
    LCD_SendNibble(0x03u, 0u); Delay(2000);
    LCD_SendNibble(0x02u, 0u);              /* switch to 4-bit mode     */
    LCD_Cmd(0x28u);                         /* 2 lines, 5x8 font        */
    LCD_Cmd(0x0Cu);                         /* display on, cursor off   */
    LCD_Cmd(0x06u);                         /* auto-increment           */
    LCD_Cmd(0x01u);                         /* clear display            */
    Delay(20000);
}

static void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0u) ? 0x00u : 0x40u;
    LCD_Cmd(0x80u | (addr + col));
}

static void LCD_Print(const char *str)
{
    while (*str)
        LCD_Char((uint8_t)(*str++));
}

static void LCD_Clear(void)
{
    LCD_Cmd(0x01u);
    Delay(20000);
}

/*------------------------------------------------------------------
 *  Main — basic LCD test
 *------------------------------------------------------------------*/
int main(void)
{
    I2C_Init();
    LCD_Init();

    /* Line 0 */
    LCD_SetCursor(0, 0);
    LCD_Print("  Hello World!  ");

    /* Line 1 */
    LCD_SetCursor(1, 0);
    LCD_Print(" PSoC 4100S Plus");

    /* Blink the text every 2 seconds to confirm it's alive */
    for (;;)
    {
        LCD_Cmd(0x08u);   /* display OFF */
        Delay(400000);
        LCD_Cmd(0x0Cu);   /* display ON  */
        Delay(400000);
    }

    return 0;
}