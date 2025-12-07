/* GPIO_RW612.h */

#ifndef GPIO_RW612_H_
#define GPIO_RW612_H_

#include "fsl_gpio.h"
#include "BITS.h"
#include "board.h"

/* Base única de GPIO en este MCU */
#define GPIO_RW612_BASE        GPIO
#define GPIO_RW612_PORT        (0U)

/* --- RGB LED on-board (D2) --- */
#define GPIO_LED_RED_GPIO      GPIO_RW612_BASE
#define GPIO_LED_RED_PORT      GPIO_RW612_PORT
#define GPIO_LED_RED_PIN       (1U)   /* GPIO_1_LED_RED  */

#define GPIO_LED_GREEN_GPIO    GPIO_RW612_BASE
#define GPIO_LED_GREEN_PORT    GPIO_RW612_PORT
#define GPIO_LED_GREEN_PIN     (12U)  /* GPIO_12_LED_GREEN */

#define GPIO_LED_BLUE_GPIO     GPIO_RW612_BASE
#define GPIO_LED_BLUE_PORT     GPIO_RW612_PORT
#define GPIO_LED_BLUE_PIN      (0U)   /* GPIO_0_LED_BLUE */

/* ==== I2C SENSORES (VL53L0X + MPU6050) ==== */
#define CTRL_I2C_INSTANCE        BOARD_CODEC_I2C_INSTANCE   /* I2C2 */
#define CTRL_I2C_SDA_PORT        BOARD_CODEC_I2C_SDA_PORT   /* 0 */
#define CTRL_I2C_SDA_PIN         BOARD_CODEC_I2C_SDA_PIN    /* 16 */
#define CTRL_I2C_SCL_PORT        BOARD_CODEC_I2C_SCL_PORT   /* 0 */
#define CTRL_I2C_SCL_PIN         BOARD_CODEC_I2C_SCL_PIN    /* 17 */

/* ==== PWM MOTORES ==== */
/* Slider */
#define CTRL_PWM_SLIDER_A_PORT   0u      /* D3 */
#define CTRL_PWM_SLIDER_A_PIN    15u
#define CTRL_PWM_SLIDER_B_PORT   0u      /* D5 */
#define CTRL_PWM_SLIDER_B_PIN    27u

/* Pan */
#define CTRL_PWM_PAN_A_PORT      0u      /* D6 (LED azul compartido) */
#define CTRL_PWM_PAN_A_PIN       0u
#define CTRL_PWM_PAN_B_PORT      0u      /* D9 */
#define CTRL_PWM_PAN_B_PIN       52u

/* Tilt */
#define CTRL_PWM_TILT_A_PORT     0u      /* D10 */
#define CTRL_PWM_TILT_A_PIN      6u
#define CTRL_PWM_TILT_B_PORT     0u      /* D11 */
#define CTRL_PWM_TILT_B_PIN      9u

/* API */
void GPIO_RW612_Init(void);

/* Control de LEDs */
void GPIO_RW612_LED_RedSet(boolean_t on);
void GPIO_RW612_LED_GreenSet(boolean_t on);
void GPIO_RW612_LED_BlueSet(boolean_t on);

static inline void GPIO_RW612_LED_RedOn(void)   { GPIO_RW612_LED_RedSet(TRUE); }
static inline void GPIO_RW612_LED_RedOff(void)  { GPIO_RW612_LED_RedSet(FALSE); }

static inline void GPIO_RW612_LED_GreenOn(void)  { GPIO_RW612_LED_GreenSet(TRUE); }
static inline void GPIO_RW612_LED_GreenOff(void) { GPIO_RW612_LED_GreenSet(FALSE); }

static inline void GPIO_RW612_LED_BlueOn(void)  { GPIO_RW612_LED_BlueSet(TRUE); }
static inline void GPIO_RW612_LED_BlueOff(void) { GPIO_RW612_LED_BlueSet(FALSE); }
void GPIO_RW612_LED_YellowOn(void);

#endif /* GPIO_RW612_H_ */
