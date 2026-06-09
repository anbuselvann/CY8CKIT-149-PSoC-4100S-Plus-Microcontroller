#include <stdint.h>

uint8_t app_heap[512]   __attribute__((section(".heap")));
uint8_t app_stack[4096] __attribute__((section(".stack")));

/* Port 1 — TRIG=P1.1, ECHO=P1.2 */
#define GPIO_PRT1_DR   (*((volatile uint32_t *)0x40040100))
#define GPIO_PRT1_PS   (*((volatile uint32_t *)0x40040104))
#define GPIO_PRT1_PC   (*((volatile uint32_t *)0x40040108))
#define HSIOM_PRT1_SEL (*((volatile uint32_t *)0x40020100))

/* Port 2 — Soil=P2.0, LDR=P2.1 */
#define GPIO_PRT2_DR   (*((volatile uint32_t *)0x40040200))
#define GPIO_PRT2_PS   (*((volatile uint32_t *)0x40040204))
#define GPIO_PRT2_PC   (*((volatile uint32_t *)0x40040208))
#define HSIOM_PRT2_SEL (*((volatile uint32_t *)0x40020200))

/* Port 3 — SCL=P3.0, SDA=P3.1 */
#define GPIO_PRT3_DR   (*((volatile uint32_t *)0x40040300))
#define GPIO_PRT3_PS   (*((volatile uint32_t *)0x40040304))
#define GPIO_PRT3_PC   (*((volatile uint32_t *)0x40040308))


#define SAR_CTRL        (*((volatile uint32_t *)0x403A0000))
#define SAR_SAMPLE_CTRL (*((volatile uint32_t *)0x403A0004))
#define SAR_SAMPLE_TIME (*((volatile uint32_t *)0x403A0010))
#define SAR_CHAN0_CFG   (*((volatile uint32_t *)0x403A0080))
#define SAR_CHAN_EN     (*((volatile uint32_t *)0x403A0020))
#define SAR_START_CTRL  (*((volatile uint32_t *)0x403A0024))
#define SAR_INTR        (*((volatile uint32_t *)0x403A0210))
#define SAR_CHAN_RESULT (*((volatile uint32_t *)0x403A0180))
#define SAR_MUX_SWITCH0 (*((volatile uint32_t *)0x403A0300))


#define LCD_ADDR  0x27u
#define LCD_RS    (1u << 0)
#define LCD_EN    (1u << 2)
#define LCD_BL    (1u << 3)
#define LCD_D4    (1u << 4)
#define LCD_D5    (1u << 5)
#define LCD_D6    (1u << 6)
#define LCD_D7    (1u << 7)


/* Soil sensor */
#define SOIL_PIN     0u
#define SOIL_READ()  ((GPIO_PRT2_PS >> SOIL_PIN) & 1u)

/* Ultrasonic — from friend's working code (P1.1=TRIG, P1.2=ECHO) */
#define TRIG_PIN     1u
#define ECHO_PIN     2u
#define TRIG_HIGH()  (GPIO_PRT1_DR |=  (1u << TRIG_PIN))
#define TRIG_LOW()   (GPIO_PRT1_DR &= ~(1u << TRIG_PIN))
#define ECHO_READ()  ((GPIO_PRT1_PS >> ECHO_PIN) & 1u)

#define US_LOOPS_PER_US  6u
#define US_TIMEOUT       180000u   /* 30 000 µs * 6 loops/µs */

/* I2C */
#define SCL_PIN  0u
#define SDA_PIN  1u
#define SCL_HIGH()  (GPIO_PRT3_DR |=  (1u << SCL_PIN))
#define SCL_LOW()   (GPIO_PRT3_DR &= ~(1u << SCL_PIN))
#define SDA_HIGH()  (GPIO_PRT3_DR |=  (1u << SDA_PIN))
#define SDA_LOW()   (GPIO_PRT3_DR &= ~(1u << SDA_PIN))

static void Delay(volatile uint32_t n)      { while (n--); }
static void I2C_Delay(void)                 { Delay(50u); }

static void DelayUs(uint32_t us)
{
    for (volatile uint32_t i = 0; i < us * US_LOOPS_PER_US; i++);
}

static void DelayMs(uint32_t ms)
{
    for (uint32_t m = 0; m < ms; m++)
        for (volatile uint32_t i = 0; i < 6000u; i++);
}

static void clock_config(void)
{
    *((uint32_t *)0x40030F08) = 0u;   /* IMO = 24 MHz */
    *((uint32_t *)0x40030028) = 0u;   /* HFCLK = IMO, div 1 */
}

static void peri_clock_config(void)
{
    /* ADC peripheral clock — Divider 1 → 12 MHz */
    *((uint32_t *)0x40010000)  = (1u << 30) | (1u << 6) | (1u << 0);
    *((uint32_t *)0x40010304)  = (2u - 1u) << 8;
    *((uint32_t *)0x40010000) |= (1u << 31) | (3u << 14) | (63u << 8)
                               | (1u << 6)  | (1u << 0);
    *((uint32_t *)0x40010148)  = (1u << 6) | (1u << 0);
}

static void I2C_Init(void)
{
    GPIO_PRT3_PC = (GPIO_PRT3_PC & ~0x3Fu) | (4u << 0) | (4u << 3);
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
    SDA_HIGH(); I2C_Delay();
    SCL_HIGH(); I2C_Delay();
    SCL_LOW();  I2C_Delay();
}

static void PCF8574_Write(uint8_t data)
{
    I2C_Start();
    I2C_WriteByte((LCD_ADDR << 1u) | 0u);
    I2C_WriteByte(data);
    I2C_Stop();
}

static void LCD_PulseEnable(uint8_t data)
{
    PCF8574_Write(data | LCD_EN);
    I2C_Delay();
    PCF8574_Write(data & ~LCD_EN);
    Delay(2000u);
}

static void LCD_SendNibble(uint8_t nibble, uint8_t mode)
{
    uint8_t d = LCD_BL | mode;
    if (nibble & 0x8u) d |= LCD_D7;
    if (nibble & 0x4u) d |= LCD_D6;
    if (nibble & 0x2u) d |= LCD_D5;
    if (nibble & 0x1u) d |= LCD_D4;
    LCD_PulseEnable(d);
}

static void LCD_SendByte(uint8_t byte, uint8_t mode)
{
    LCD_SendNibble(byte >> 4u,   mode);
    LCD_SendNibble(byte & 0x0Fu, mode);
    Delay(2000u);
}

static void LCD_Cmd(uint8_t cmd) { LCD_SendByte(cmd, 0u);     }
static void LCD_Char(uint8_t ch) { LCD_SendByte(ch,  LCD_RS); }

static void LCD_Init(void)
{
    Delay(500000u);
    LCD_SendNibble(0x03u, 0u); Delay(50000u);
    LCD_SendNibble(0x03u, 0u); Delay(5000u);
    LCD_SendNibble(0x03u, 0u); Delay(2000u);
    LCD_SendNibble(0x02u, 0u);
    LCD_Cmd(0x28u);
    LCD_Cmd(0x0Cu);
    LCD_Cmd(0x06u);
    LCD_Cmd(0x01u);
    Delay(20000u);
}

static void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0u) ? 0x00u : 0x40u;
    LCD_Cmd(0x80u | (addr + col));
}

static void LCD_Print(const char *s)
{
    while (*s) LCD_Char((uint8_t)(*s++));
}

static void LCD_Clear(void)
{
    LCD_Cmd(0x01u);
    Delay(20000u);
}

static void LCD_PrintUInt(uint16_t val, uint8_t width)
{
    char buf[6] = {' ',' ',' ',' ',' ','\0'};
    uint8_t i = width - 1u;
    if (val == 0u)
        buf[i] = '0';
    else
        while (val > 0u && i < width)
        {
            buf[i--] = '0' + (val % 10u);
            val /= 10u;
        }
    LCD_Print(buf);
}

static void Soil_Init(void)
{
    /* P2.0 = hi-Z digital input */
    GPIO_PRT2_PC   &= ~(0x7u << 0);
    HSIOM_PRT2_SEL &= ~(0xFu << 0);
}

static void Ultrasonic_Init(void)
{
    /* Clear drive-mode bits for TRIG (P1.1) and ECHO (P1.2) */
    GPIO_PRT1_PC &= ~(0x3Fu << (TRIG_PIN * 3u));

    /* TRIG = push-pull output (DM=6), ECHO = digital input (DM=1) */
    GPIO_PRT1_PC |= (0x6u << (TRIG_PIN * 3u))
                  | (0x1u << (ECHO_PIN  * 3u));

    HSIOM_PRT1_SEL &= ~(0xFu << (TRIG_PIN * 4u));
    HSIOM_PRT1_SEL &= ~(0xFu << (ECHO_PIN  * 4u));

    TRIG_LOW();
}

static uint32_t Ultrasonic_GetDistanceCm(void)
{
    uint32_t echo_count = 0u;
    uint32_t timeout    = 0u;

    /* Clean LOW, then 10 µs trigger pulse */
    TRIG_LOW();
    DelayUs(2u);
    TRIG_HIGH();
    DelayUs(10u);
    TRIG_LOW();

    /* Wait for ECHO to go HIGH */
    timeout = 0u;
    while (ECHO_READ() == 0u)
    {
        timeout++;
        if (timeout >= US_TIMEOUT) return 0u;
    }

    /* Count while ECHO is HIGH */
    echo_count = 0u;
    while (ECHO_READ() == 1u)
    {
        echo_count++;
        if (echo_count >= US_TIMEOUT) break;
    }

    /* echo_count / US_LOOPS_PER_US = pulse µs; pulse µs / 58 = cm */
    uint32_t pulse_us = echo_count / US_LOOPS_PER_US;
    return pulse_us / 58u;
}

static void ADC_Init(void)
{
    /* P2.1 = analog input (drive mode 0 = hi-Z analog) */
    GPIO_PRT2_PC   &= ~(0x7u << 3u);
    HSIOM_PRT2_SEL &= ~(0xFu << 4u);

    /* SAR_CTRL: Vref=VDDA, bypass, NEG=Vref, enabled */
    SAR_CTRL |= (0x6u << 4) | (1u << 7) | (0x7u << 9)
              | (1u << 30)  | (1u << 31);

    /* Mux: Vplus = P2.1 */
    SAR_MUX_SWITCH0 = (1u << 1);

    /* Sample time = 10 cycles */
    SAR_SAMPLE_TIME |= 0xAu;

    /* Chan0: P2.1, 12-bit */
    SAR_CHAN0_CFG = (1u << 0);

    /* Enable channel 0 */
    SAR_CHAN_EN = (1u << 0);
}

static uint16_t LDR_Read(void)
{
    SAR_START_CTRL = (1u << 0);
    while ((SAR_INTR & 0x1u) != 0x1u);
    SAR_INTR |= 0x1u;
    return (uint16_t)(SAR_CHAN_RESULT & 0xFFFu);
}


static void Screen_SoilLDR(uint8_t soilDry, uint16_t ldr)
{
    LCD_Clear();

    LCD_SetCursor(0, 0);
    LCD_Print("Soil:");
    LCD_Print(soilDry ? "DRY " : "WET ");
    LCD_Print(ldr > 2000u ? "Sun:HI" : "Sun:LO");

    LCD_SetCursor(1, 0);
    LCD_Print("LDR:");
    LCD_PrintUInt(ldr, 4);
    LCD_Print("       ");
}

static void Screen_Tank(uint32_t dist_cm)
{
    LCD_Clear();
    LCD_SetCursor(0, 0);
    LCD_Print("Tank Level:     ");
    LCD_SetCursor(1, 0);

    if (dist_cm == 0u)
    {
        LCD_Print("  Out of range  ");
        return;
    }

    LCD_PrintUInt((uint16_t)dist_cm, 3);
    LCD_Print("cm ");
    if      (dist_cm < 5u)  LCD_Print("FULL ");
    else if (dist_cm < 15u) LCD_Print("GOOD ");
    else if (dist_cm < 25u) LCD_Print("LOW! ");
    else                    LCD_Print("EMTY!");
}

#define SCREEN_HOLD  18000000u   /* ~3 s at 24 MHz */

int main(void)
{
    clock_config();
    peri_clock_config();
    I2C_Init();
    LCD_Init();
    Soil_Init();
    Ultrasonic_Init();
    ADC_Init();

    /* Splash */
    LCD_Clear();
    LCD_SetCursor(0, 0); LCD_Print(" Smart Farm Sys ");
    LCD_SetCursor(1, 0); LCD_Print(" PSoC 4100S Plus");
    DelayMs(2000u);

    uint8_t  screen  = 0u;
    uint16_t ldr     = 0u;
    uint32_t dist_cm = 0u;

    for (;;)
    {
        switch (screen)
        {
            case 0u:
                ldr = LDR_Read();
                Screen_SoilLDR(SOIL_READ(), ldr);
                break;

            case 1u:
                dist_cm = Ultrasonic_GetDistanceCm();
                Screen_Tank(dist_cm);
                break;

            default:
                screen = 0u;
                break;
        }

        Delay(SCREEN_HOLD);
        if (++screen > 1u) screen = 0u;
    }

    return 0;
}