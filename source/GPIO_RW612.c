/* GPIO_RW612.c */

#include "GPIO_RW612.h"

void GPIO_RW612_Init(void)
{
    gpio_pin_config_t led_cfg = {
        .pinDirection     = kGPIO_DigitalOutput,
        .outputLogic   = LOGIC_LED_OFF
    };

    /* Un solo puerto: 0 */
    GPIO_PortInit(GPIO_RW612_BASE, GPIO_RW612_PORT);

    /* RGB LED */
    GPIO_PinInit(GPIO_LED_RED_GPIO,   GPIO_LED_RED_PORT,   GPIO_LED_RED_PIN,   &led_cfg);
    GPIO_PinInit(GPIO_LED_GREEN_GPIO, GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN, &led_cfg);
    GPIO_PinInit(GPIO_LED_BLUE_GPIO,  GPIO_LED_BLUE_PORT,  GPIO_LED_BLUE_PIN,  &led_cfg);
}

void GPIO_RW612_LED_RedSet(boolean_t on)
{
    GPIO_PinWrite(GPIO_LED_RED_GPIO, GPIO_LED_RED_PORT, GPIO_LED_RED_PIN,
                  (on == TRUE) ? LOGIC_LED_ON : LOGIC_LED_OFF);
}

void GPIO_RW612_LED_GreenSet(boolean_t on)
{
    GPIO_PinWrite(GPIO_LED_GREEN_GPIO, GPIO_LED_GREEN_PORT, GPIO_LED_GREEN_PIN,
                  (on == TRUE) ? LOGIC_LED_ON : LOGIC_LED_OFF);
}

void GPIO_RW612_LED_BlueSet(boolean_t on)
{
    GPIO_PinWrite(GPIO_LED_BLUE_GPIO, GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN,
                  (on == TRUE) ? LOGIC_LED_ON : LOGIC_LED_OFF);
}

void GPIO_RW612_LED_YellowOn(void)
{
    GPIO_RW612_LED_RedSet(TRUE);
    GPIO_RW612_LED_GreenSet(TRUE);
    GPIO_RW612_LED_BlueSet(FALSE);
}
