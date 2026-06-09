#include <stdint.h>
#include "i2c_lcd.h"

/* ================= GPIO ================= */
#define GPIO_PRT2_BASE 0x40040200UL
#define GPIO_PRT3_BASE 0x40040300UL

#define DR3 (*((volatile uint32_t *)(GPIO_PRT3_BASE + 0x00)))
#define PS3 (*((volatile uint32_t *)(GPIO_PRT3_BASE + 0x04)))
#define PC3 (*((volatile uint32_t *)(GPIO_PRT3_BASE + 0x08)))

#define DR2 (*((volatile uint32_t *)(GPIO_PRT2_BASE + 0x00)))
#define PC2 (*((volatile uint32_t *)(GPIO_PRT2_BASE + 0x08)))

#define READ_SW1() ((PS3 >> 7u) & 1u)

#define LED_ON()  (DR2 &= ~(1u << 2))
#define LED_OFF() (DR2 |=  (1u << 2))

/* ================= Variables ================= */
volatile uint32_t tick = 0;

/* ================= Delay ================= */
void Delay(uint32_t d)
{
    while(d--);
}


/* ================= GPIO ================= */
void GPIO_Init(void)
{
    /* SW1 input pull-up */
    PC3 = (PC3 & ~(7u << 21)) | (2u << 21);
    DR3 |= (1u << 7);

    /* LED output */
    PC2 = (PC2 & ~(7u << 6)) | (6u << 6);
    DR2 |= (1u << 2);
}

/* ================= Main ================= */
int main(void)
{
    GPIO_Init();
    LCD_Init();

    LCD_Clear();
    LCD_SetCursor(0,0);
    LCD_Print(" Digital Dice ");

    uint8_t last = 1;

    while(1)
    {
        tick++;

        uint8_t sw = READ_SW1();

        if(sw == 0 && last == 1)
        {
            Delay(20000);

            if(READ_SW1() == 0)
            {
                uint8_t dice = (tick % 6) + 1;

                /* LED blink */
                LED_ON();
                Delay(30000);
                LED_OFF();

                /* LCD display */
                LCD_Clear();
                LCD_SetCursor(0,0);
                LCD_Print(" Roll Result ");

                LCD_SetCursor(1,0);

                char num[2];
                num[0] = '0' + dice;
                num[1] = '\0';

                LCD_Print(num);

                while(READ_SW1() == 0);
                Delay(20000);
            }
        }

        last = sw;
    }
}