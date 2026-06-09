#include <stdint.h>

uint8_t app_heap[512]   __attribute__((section(".heap")));
uint8_t app_stack[4096] __attribute__((section(".stack")));

/* ============================================================
 *  SMART DUSTBIN — PIN ASSIGNMENT
 * ============================================================
 *  LCD  I2C bit-bang via PCF8574:
 *    P3.0 = SCL   0x40040300  DM=4 open-drain
 *    P3.1 = SDA   0x40040300  DM=4 open-drain
 *
 *  HC-SR04 Ultrasonic:
 *    P2.0 = TRIG  0x40040200  DM=6 push-pull output
 *    P2.1 = ECHO  0x40040204  DM=1 digital input
 *
 *  Servo Motor (SG90 / MG995):
 *    P1.0 = PWM   0x40040100  DM=6 push-pull output
 *
 *  TRM ref: Section 15, pp.749-752 (Doc 002-21159 Rev.*D)
 * ============================================================ */

/* ---- Register macros ---- */
#define GPIO_PRT1_DR  (*((volatile uint32_t *)0x40040100u))
#define GPIO_PRT1_PC  (*((volatile uint32_t *)0x40040108u))

#define GPIO_PRT2_DR  (*((volatile uint32_t *)0x40040200u))
#define GPIO_PRT2_PS  (*((volatile uint32_t *)0x40040204u))
#define GPIO_PRT2_PC  (*((volatile uint32_t *)0x40040208u))

#define GPIO_PRT3_DR  (*((volatile uint32_t *)0x40040300u))
#define GPIO_PRT3_PS  (*((volatile uint32_t *)0x40040304u))
#define GPIO_PRT3_PC  (*((volatile uint32_t *)0x40040308u))

/* ---- Pin aliases ---- */
#define SERVO_HIGH()  (GPIO_PRT1_DR |=  (1u << 0))
#define SERVO_LOW()   (GPIO_PRT1_DR &= ~(1u << 0))
#define TRIG_HIGH()   (GPIO_PRT2_DR |=  (1u << 0))
#define TRIG_LOW()    (GPIO_PRT2_DR &= ~(1u << 0))
#define ECHO_READ()   ((GPIO_PRT2_PS >> 1) & 1u)
#define SCL_HIGH()    (GPIO_PRT3_DR |=  (1u << 0))
#define SCL_LOW()     (GPIO_PRT3_DR &= ~(1u << 0))
#define SDA_HIGH()    (GPIO_PRT3_DR |=  (1u << 1))
#define SDA_LOW()     (GPIO_PRT3_DR &= ~(1u << 1))
#define SDA_READ()    ((GPIO_PRT3_PS >> 1) & 1u)

/* ---- LCD backpack ---- */
#define LCD_ADDR  0x27u
#define LCD_RS  (1u<<0)
#define LCD_EN  (1u<<2)
#define LCD_BL  (1u<<3)
#define LCD_D4  (1u<<4)
#define LCD_D5  (1u<<5)
#define LCD_D6  (1u<<6)
#define LCD_D7  (1u<<7)

/* ============================================================
 *  TIMING
 * ============================================================
 *  PSoC 4100S Plus default IMO = 24 MHz (Cortex-M0+).
 *
 *  Cortex-M0+ loop body (volatile counter, -O0):
 *    STR, LDR, CMP, BLT  ≈ 6-8 cycles each iteration.
 *  At 24 MHz → ~3-4 iterations per µs.
 *
 *  US_LOOPS_PER_US is set conservatively at 3.
 *  If your servo STILL does not move, change it to 2.
 *  If it moves too fast / wrong angle, change it to 4.
 *
 *  SERVO PERIOD = 20 000 µs (50 Hz standard hobby servo).
 *  Valid HIGH pulse range for most hobby servos:
 *    500 µs  = full CCW  / 0°
 *   1000 µs  = 0°   for many SG90s
 *   1500 µs  = 90°  (centre)
 *   2000 µs  = 180°
 *   2500 µs  = full CW
 *  Start with 1500 µs for centre, then adjust.
 *
 *  OPEN_THRESHOLD_CM: open lid when object is closer than this.
 * ============================================================ */
#define US_LOOPS_PER_US       3u

#define SERVO_PERIOD_US       20000u
#define SERVO_PULSE_CLOSED_US  1000u   /* 0°  – lid CLOSED */
#define SERVO_PULSE_OPEN_US    2000u   /* 90°+ – lid OPEN  */
#define SERVO_HOLD_PULSES        15u   /* pulses per cycle to hold angle */

#define OPEN_THRESHOLD_CM        20u

/* ============================================================
 *  DELAY FUNCTIONS
 * ============================================================ */
static void Delay(volatile uint32_t n)
{
    while (n--);
}

/*  DelayUs: blocks for approximately 'us' microseconds.
 *  The loop runs US_LOOPS_PER_US iterations per microsecond.
 *  Marked noinline so the compiler cannot optimize the loop away. */
__attribute__((noinline))
static void DelayUs(uint32_t us)
{
    volatile uint32_t count = us * US_LOOPS_PER_US;
    while (count--);
}

/* ============================================================
 *  I2C BIT-BANG
 * ============================================================ */
static void I2C_Delay(void) { for (volatile uint32_t i = 0u; i < 50u; i++); }

static void I2C_Init(void)
{
    /* P3.0/P3.1: open-drain (DM=4). PC bits: pin0=[2:0], pin1=[5:3] */
    GPIO_PRT3_PC = (GPIO_PRT3_PC & ~0x3Fu) | (4u<<3) | (4u<<0);
    GPIO_PRT3_DR |= (1u<<0) | (1u<<1);
}

static void I2C_Start(void)  { SDA_HIGH(); SCL_HIGH(); I2C_Delay(); SDA_LOW(); I2C_Delay(); SCL_LOW(); I2C_Delay(); }
static void I2C_Stop(void)   { SDA_LOW(); I2C_Delay(); SCL_HIGH(); I2C_Delay(); SDA_HIGH(); I2C_Delay(); }

static void I2C_WriteBit(uint8_t bit)
{
    if (bit) SDA_HIGH(); else SDA_LOW();
    I2C_Delay(); SCL_HIGH(); I2C_Delay(); SCL_LOW(); I2C_Delay();
}

static void I2C_WriteByte(uint8_t byte)
{
    for (int8_t i = 7; i >= 0; i--) I2C_WriteBit((byte >> i) & 1u);
    SDA_HIGH(); I2C_Delay(); SCL_HIGH(); I2C_Delay(); SCL_LOW(); I2C_Delay();
}

static void PCF8574_Write(uint8_t data)
{
    I2C_Start();
    I2C_WriteByte((LCD_ADDR << 1u) | 0u);
    I2C_WriteByte(data);
    I2C_Stop();
}

/* ============================================================
 *  LCD DRIVER
 * ============================================================ */
static void LCD_PulseEnable(uint8_t d) { PCF8574_Write(d|LCD_EN); I2C_Delay(); PCF8574_Write(d&~LCD_EN); Delay(2000u); }

static void LCD_SendNibble(uint8_t n, uint8_t mode)
{
    uint8_t d = LCD_BL | mode;
    if (n&8u) d|=LCD_D7; if (n&4u) d|=LCD_D6; if (n&2u) d|=LCD_D5; if (n&1u) d|=LCD_D4;
    LCD_PulseEnable(d);
}

static void LCD_SendByte(uint8_t b, uint8_t mode) { LCD_SendNibble(b>>4u, mode); LCD_SendNibble(b&0xFu, mode); Delay(2000u); }
static void LCD_Cmd(uint8_t c)  { LCD_SendByte(c, 0u); }
static void LCD_Char(uint8_t c) { LCD_SendByte(c, LCD_RS); }

static void LCD_Init(void)
{
    Delay(500000u);
    LCD_SendNibble(0x03u,0u); Delay(50000u);
    LCD_SendNibble(0x03u,0u); Delay(5000u);
    LCD_SendNibble(0x03u,0u); Delay(2000u);
    LCD_SendNibble(0x02u,0u);
    LCD_Cmd(0x28u); LCD_Cmd(0x0Cu); LCD_Cmd(0x06u); LCD_Cmd(0x01u);
    Delay(20000u);
}

static void LCD_SetCursor(uint8_t row, uint8_t col)
{
    LCD_Cmd(0x80u | ((row ? 0x40u : 0x00u) + col));
}

static void LCD_Print(const char *s) { while (*s) LCD_Char((uint8_t)(*s++)); }
static void LCD_Clear(void)          { LCD_Cmd(0x01u); Delay(20000u); }

/* ============================================================
 *  SERVO DRIVER
 * ============================================================
 *  P1.0: DM = 0x6 (push-pull), PC bits [2:0].
 *
 *  Servo_SendPulse():
 *    Drive P1.0 HIGH for pulse_us µs,
 *    then LOW for the remainder of the 20 ms period.
 *
 *  Servo_Hold():
 *    Repeat N pulses so the motor actually reaches the angle
 *    and holds against load torque.
 * ============================================================ */
static void Servo_Init(void)
{
    GPIO_PRT1_PC &= ~(0x7u);   /* clear DM[2:0] for pin 0 */
    GPIO_PRT1_PC |=  (0x6u);   /* DM = 6: push-pull output */
    SERVO_LOW();
}

__attribute__((noinline))
static void Servo_SendPulse(uint32_t pulse_us)
{
    SERVO_HIGH();
    DelayUs(pulse_us);
    SERVO_LOW();
    /* REST of 20 ms period — subtract pulse already spent */
    DelayUs(SERVO_PERIOD_US - pulse_us);
}

static void Servo_Hold(uint32_t pulse_us)
{
    for (uint8_t i = 0u; i < SERVO_HOLD_PULSES; i++)
        Servo_SendPulse(pulse_us);
}

/* ============================================================
 *  SERVO SWEEP TEST
 * ============================================================
 *  Call this ONCE at boot (before normal operation) to
 *  verify the servo works independently of the sensor.
 *
 *  It sweeps the pulse from 500 µs → 2500 µs in 500 µs steps,
 *  pausing 30 pulses (~600 ms) at each step.
 *  Watch which pulse makes the servo move — that tells you
 *  the correct range to put in SERVO_PULSE_OPEN/CLOSED_US.
 *
 *  LCD shows the current test pulse width while sweeping.
 * ============================================================ */
static void IntToStr(uint32_t v, char *buf, uint8_t w)
{
    for (uint8_t i = 0u; i < w; i++) buf[i] = ' ';
    buf[w] = '\0';
    if (v == 0u) { buf[w-1u] = '0'; return; }
    uint8_t p = w;
    while (v > 0u && p > 0u) { buf[--p] = (char)('0' + v%10u); v /= 10u; }
}

static void Servo_SweepTest(void)
{
    char buf[5];
    uint32_t pulses[] = {500u, 1000u, 1500u, 2000u, 2500u};
    uint8_t  n = sizeof(pulses) / sizeof(pulses[0]);

    LCD_SetCursor(0u, 0u); LCD_Print("Servo sweep test");

    for (uint8_t i = 0u; i < n; i++)
    {
        /* Show pulse width on LCD row 1 */
        LCD_SetCursor(1u, 0u);
        LCD_Print("Pulse:");
        IntToStr(pulses[i], buf, 4u);
        LCD_Print(buf);
        LCD_Print("us  ");

        /* Hold this angle for 30 pulses (~600 ms) */
        for (uint8_t p = 0u; p < 30u; p++)
            Servo_SendPulse(pulses[i]);
    }

    LCD_Clear();
}

/* ============================================================
 *  ULTRASONIC SENSOR
 * ============================================================
 *  P2.0 TRIG: DM=6 push-pull  PC[2:0] = 0x6
 *  P2.1 ECHO: DM=1 input      PC[5:3] = 0x1
 *  Combined: (0x1<<3)|(0x6<<0) = 0x0E
 * ============================================================ */
#define US_TIMEOUT  180000u   /* 30 ms guard */

static void Ultrasonic_Init(void)
{
    GPIO_PRT2_PC &= ~(0x3Fu);
    GPIO_PRT2_PC |=  (0x1u<<3) | (0x6u<<0);
    TRIG_LOW();
}

static uint32_t Ultrasonic_GetDistanceCm(void)
{
    uint32_t count = 0u, timeout = 0u;
    TRIG_LOW();  DelayUs(2u);
    TRIG_HIGH(); DelayUs(10u);
    TRIG_LOW();
    while (!ECHO_READ()) { if (++timeout >= US_TIMEOUT) return 0u; }
    while ( ECHO_READ()) { if (++count   >= US_TIMEOUT) break; }
    return (count / US_LOOPS_PER_US) / 58u;
}

/* ============================================================
 *  LCD UPDATE
 * ============================================================
 *  Row 0:  Dist:  XX cm
 *  Row 1:  Lid : OPEN      (or CLOSED)
 * ============================================================ */
static void LCD_UpdateDisplay(uint32_t dist, uint8_t open)
{
    char buf[5];
    LCD_SetCursor(0u, 0u);
    if      (dist == 0u)    { LCD_Print("Dist: --  cm    "); }
    else if (dist > 400u)   { LCD_Print("Dist: >400 cm   "); }
    else
    {
        IntToStr(dist, buf, 3u);
        LCD_Print("Dist: "); LCD_Print(buf); LCD_Print(" cm      ");
    }

    LCD_SetCursor(1u, 0u);
    LCD_Print(open ? "Lid : OPEN      " : "Lid : CLOSED    ");
}

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(void)
{
    I2C_Init();
    Ultrasonic_Init();
    Servo_Init();
    LCD_Init();

    /* ---- Splash ---- */
    LCD_SetCursor(0u,0u); LCD_Print(" Smart Dustbin  ");
    LCD_SetCursor(1u,0u); LCD_Print("PSoC 4100S Plus ");
    Delay(1200000u);
    LCD_Clear();

    /* ===========================================================
     *  SERVO SWEEP TEST
     *  Uncomment the line below to run the sweep at boot.
     *  Watch which pulse step makes the motor move, then set
     *  SERVO_PULSE_OPEN_US / SERVO_PULSE_CLOSED_US accordingly
     *  and comment it out again before your final build.
     * =========================================================== */
    Servo_SweepTest();   /* <-- comment out after calibration */

    /* ---- Startup: close lid ---- */
    Servo_Hold(SERVO_PULSE_CLOSED_US);

    uint8_t  lid_open = 0u;
    uint32_t dist     = 0u;

    for (;;)
    {
        /* 1. Measure distance */
        dist = Ultrasonic_GetDistanceCm();

        /* 2. Decide lid state */
        lid_open = (dist > 0u && dist < OPEN_THRESHOLD_CM) ? 1u : 0u;

        /* 3. Drive servo to position and hold */
        Servo_Hold(lid_open ? SERVO_PULSE_OPEN_US : SERVO_PULSE_CLOSED_US);

        /* 4. Update LCD */
        LCD_UpdateDisplay(dist, lid_open);
    }

    return 0;
}
