/*
 * MOTOR_PWM.c
 *
 * PWM de 3 motores usando CTIMER0 y CTIMER3 en FRDM-RW612.
 *  - 1 PWM por motor (IN2)
 *  - IN1 como GPIO de dirección.
 *
 * Mapeo de pines:
 *
 *  SLIDER:
 *    IN2  -> GPIO0_MATCH0_TIMER0  (CTIMER0_MAT0, salida PWM)
 *    IN1  -> GPIO51               (GPIO para dirección)
 *
 *  PAN:
 *    IN2  -> GPIO52_MATCH0_TIMER3 (CTIMER3_MAT0, salida PWM)
 *    IN1  -> GPIO53               (GPIO para dirección)
 *
 *  TILT:
 *    IN2  -> GPIO1_MATCH1_TIMER0  (CTIMER0_MAT1, salida PWM)
 *    IN1  -> GPIO54               (GPIO para dirección)
 */

#include "MOTOR_PWM.h"
#include "fsl_ctimer.h"
#include "fsl_gpio.h"
#include "clock_config.h"
#include "pin_mux.h"

/*===============================================================
 *  DEFINICIÓN DE HARDWARE
 *==============================================================*/

/* Frecuencia de PWM (Hz). */
#define MOTOR_PWM_FREQUENCY_HZ        (20000U)

/* --- CTIMER0: SLIDER (IN2) + TILT (IN2) --------------------- */
#define MOTOR_PWM_CTIMER0             CTIMER0
#define MOTOR_PWM_CTIMER0_PERIOD_CH   kCTIMER_Match_3
#define MOTOR_PWM_CTIMER0_SLIDER_CH   kCTIMER_Match_0   /* GPIO0_MATCH0_TIMER0 */
#define MOTOR_PWM_CTIMER0_TILT_CH     kCTIMER_Match_1   /* GPIO1_MATCH1_TIMER0 */

/* --- CTIMER3: PAN (IN2) ------------------------------------- */
#define MOTOR_PWM_CTIMER3             CTIMER3
#define MOTOR_PWM_CTIMER3_PERIOD_CH   kCTIMER_Match_3
#define MOTOR_PWM_CTIMER3_PAN_CH      kCTIMER_Match_0   /* GPIO52_MATCH0_TIMER3 */

/* --- Pines de dirección (IN1, GPIO) ------------------------- */
/* Todos son PORT 0 en RW612 */
#define MOTOR_PWM_GPIO_BASE           GPIO
#define MOTOR_PWM_DIR_PORT            0U

#define MOTOR_PWM_SLIDER_DIR_PIN      51U    /* GPIO51 -> IN1 Slider */
#define MOTOR_PWM_PAN_DIR_PIN         53U    /* GPIO53 -> IN1 Pan    */
#define MOTOR_PWM_TILT_DIR_PIN        54U    /* GPIO54 -> IN1 Tilt   */

/* --- IO_MUX para las salidas PWM (IN2) ---------------------- */
#define MOTOR_PWM_MUX_SLIDER_PWM()    IO_MUX_SetPinMux(IO_MUX_CT0_MAT0_OUT)
#define MOTOR_PWM_MUX_TILT_PWM()      IO_MUX_SetPinMux(IO_MUX_CT0_MAT1_OUT)
#define MOTOR_PWM_MUX_PAN_PWM()       IO_MUX_SetPinMux(IO_MUX_CT3_MAT0_OUT)

/*===============================================================
 *  VARIABLES GLOBALES / ESTÁTICAS
 *==============================================================*/

static uint32_t g_timerClock = 0U;   /* Frecuencia de conteo del CTIMER */

/*===============================================================
 *  MAPEO POR MOTOR
 *==============================================================*/

typedef struct
{
    uint8_t          baseId;      /* 0 -> CTIMER0, 3 -> CTIMER3 */
    ctimer_match_t   pwmMatchCh;  /* Canal de PWM (MATx) */
    uint32_t         dirPin;      /* GPIO pin para IN1 */
} MOTOR_HW_MAP_T;

static const MOTOR_HW_MAP_T g_motorHw[MOTOR_ID_COUNT] =
{
    /* MOTOR_ID_SLIDER */
    {
        .baseId     = 0u,
        .pwmMatchCh = MOTOR_PWM_CTIMER0_SLIDER_CH,
        .dirPin     = MOTOR_PWM_SLIDER_DIR_PIN
    },
    /* MOTOR_ID_PAN */
    {
        .baseId     = 3u,
        .pwmMatchCh = MOTOR_PWM_CTIMER3_PAN_CH,
        .dirPin     = MOTOR_PWM_PAN_DIR_PIN
    },
    /* MOTOR_ID_TILT */
    {
        .baseId     = 0u,
        .pwmMatchCh = MOTOR_PWM_CTIMER0_TILT_CH,
        .dirPin     = MOTOR_PWM_TILT_DIR_PIN
    }
};

/*===============================================================
 *  FUNCIONES PRIVADAS
 *==============================================================*/

/*!
 * @brief Convierte salida normalizada [-1,1] a (dir, duty%)
 *
 * dirHigh = TRUE  -> IN1 = 1, PWM en IN2 (sentido +)
 * dirHigh = FALSE -> IN1 = 0, PWM en IN2 (sentido -)
 */
static void MOTOR_PWM_ComputeDirDuty(float u_norm,
                                     boolean_t *dirHigh,
                                     uint8_t   *dutyPercent)
{
    float u_sat = u_norm;

    /* Saturar a rango [-1,1] */
    if (u_sat > 1.0f)
    {
        u_sat = 1.0f;
    }
    else if (u_sat < -1.0f)
    {
        u_sat = -1.0f;
    }

    /* Pequeña zona muerta para evitar ruido muy pequeño */
    if (u_sat < 0.02f && u_sat > -0.02f)
    {
        u_sat = 0.0f;
    }

    if (u_sat >= 0.0f)
    {
        *dirHigh = TRUE;
    }
    else
    {
        *dirHigh = FALSE;
        u_sat = -u_sat;
    }

    if (u_sat < 0.0f)
    {
        u_sat = 0.0f;
    }
    if (u_sat > 1.0f)
    {
        u_sat = 1.0f;
    }

    float duty = u_sat * 100.0f;
    if (duty > 100.0f)
    {
        duty = 100.0f;
    }

    *dutyPercent = (uint8_t)(duty + 0.5f);
}

/*!
 * @brief Aplica duty en un canal PWM concreto usando CTIMER_SetupPwmPeriod.
 *
 *  dutyPercent = 0  -> 0% (apagado)
 *  dutyPercent = 50 -> 50%
 *  dutyPercent = 100-> 100%
 */
static void MOTOR_PWM_ApplyPwm(uint8_t baseId,
                               ctimer_match_t periodCh,
                               ctimer_match_t pwmCh,
                               uint8_t dutyPercent)
{
    uint32_t pwmPeriod;
    uint32_t pulse;

    if (g_timerClock == 0U)
    {
        /* Algo fue mal en Init, por seguridad no hacemos nada */
        return;
    }

    /* Periodo en cuentas de timer */
    pwmPeriod = (g_timerClock / MOTOR_PWM_FREQUENCY_HZ) - 1U;

    /* Duty directo (NO invertido).
     * CTIMER_SetupPwmPeriod espera que 'pulse' sea la parte activa.
     */
    pulse = (pwmPeriod + 1U) * (uint32_t)dutyPercent / 100U;

    if (baseId == 0u)
    {
        CTIMER_SetupPwmPeriod(MOTOR_PWM_CTIMER0,
                              periodCh,
                              pwmCh,
                              pwmPeriod,
                              pulse,
                              false);
        CTIMER_StartTimer(MOTOR_PWM_CTIMER0);
    }
    else if (baseId == 3u)
    {
        CTIMER_SetupPwmPeriod(MOTOR_PWM_CTIMER3,
                              periodCh,
                              pwmCh,
                              pwmPeriod,
                              pulse,
                              false);
        CTIMER_StartTimer(MOTOR_PWM_CTIMER3);
    }
}

/*===============================================================
 *  API PÚBLICA
 *==============================================================*/

void MOTOR_PWM_Init(void)
{
    ctimer_config_t config;
    gpio_pin_config_t gpioCfg = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic  = 0u
    };

    /* 1) Attach del reloj a los CTIMER que vamos a usar */
    CLOCK_AttachClk(kSFRO_to_CTIMER0);
    CLOCK_AttachClk(kSFRO_to_CTIMER3);

    /* 2) Config por defecto e init de CTIMER0 y CTIMER3 */
    CTIMER_GetDefaultConfig(&config);
    CTIMER_Init(MOTOR_PWM_CTIMER0, &config);
    CTIMER_Init(MOTOR_PWM_CTIMER3, &config);

    /* 3) Obtener frecuencia del reloj del CTIMER
     *    (ambos usan SFRO, así que usamos el de CTIMER0 para todos)
     */
    {
        uint32_t srcClock_Hz = CLOCK_GetCTimerClkFreq(0U);
        g_timerClock = srcClock_Hz / (config.prescale + 1U);
    }

    /* 4) Mux de las salidas PWM (IN2) */
    MOTOR_PWM_MUX_SLIDER_PWM();  /* CTIMER0_MAT0 -> GPIO0 (Slider IN2) */
    MOTOR_PWM_MUX_TILT_PWM();    /* CTIMER0_MAT1 -> GPIO1 (Tilt IN2)   */
    MOTOR_PWM_MUX_PAN_PWM();     /* CTIMER3_MAT0 -> GPIO52 (Pan IN2)   */

    /* 5) Pines de dirección (IN1) como GPIO */
    GPIO_PinInit(MOTOR_PWM_GPIO_BASE,
                 MOTOR_PWM_DIR_PORT,
                 MOTOR_PWM_SLIDER_DIR_PIN,
                 &gpioCfg);

    GPIO_PinInit(MOTOR_PWM_GPIO_BASE,
                 MOTOR_PWM_DIR_PORT,
                 MOTOR_PWM_PAN_DIR_PIN,
                 &gpioCfg);

    GPIO_PinInit(MOTOR_PWM_GPIO_BASE,
                 MOTOR_PWM_DIR_PORT,
                 MOTOR_PWM_TILT_DIR_PIN,
                 &gpioCfg);

    /* Dejar IN1 en bajo por defecto (sentido - apagado);
     * con duty 0% no se moverá.
     */
    GPIO_PinWrite(MOTOR_PWM_GPIO_BASE, MOTOR_PWM_DIR_PORT, MOTOR_PWM_SLIDER_DIR_PIN, 0u);
    GPIO_PinWrite(MOTOR_PWM_GPIO_BASE, MOTOR_PWM_DIR_PORT, MOTOR_PWM_PAN_DIR_PIN,    0u);
    GPIO_PinWrite(MOTOR_PWM_GPIO_BASE, MOTOR_PWM_DIR_PORT, MOTOR_PWM_TILT_DIR_PIN,   0u);

    /* 6) Inicializar los 3 canales con duty 0%, así ya arranca el CTIMER
     *    y tenemos una base limpia.
     */
    MOTOR_PWM_ApplyPwm(0u,
                       MOTOR_PWM_CTIMER0_PERIOD_CH,
                       MOTOR_PWM_CTIMER0_SLIDER_CH,
                       0u);

    MOTOR_PWM_ApplyPwm(0u,
                       MOTOR_PWM_CTIMER0_PERIOD_CH,
                       MOTOR_PWM_CTIMER0_TILT_CH,
                       0u);

    MOTOR_PWM_ApplyPwm(3u,
                       MOTOR_PWM_CTIMER3_PERIOD_CH,
                       MOTOR_PWM_CTIMER3_PAN_CH,
                       0u);
}

void MOTOR_PWM_SetOutput(MOTOR_ID_T motor, float u_norm)
{
    if (motor >= MOTOR_ID_COUNT)
    {
        return;
    }

    boolean_t dirHigh;
    uint8_t duty;
    const MOTOR_HW_MAP_T *cfg = &g_motorHw[motor];

    /* 1) Convertir u_norm a dirección + duty% */
    MOTOR_PWM_ComputeDirDuty(u_norm, &dirHigh, &duty);

    /* 2) Dirección en IN1 (GPIO) */
    GPIO_PinWrite(MOTOR_PWM_GPIO_BASE,
                  MOTOR_PWM_DIR_PORT,
                  cfg->dirPin,
                  (dirHigh == TRUE) ? 1u : 0u);

    /* 3) Duty en IN2 (PWM CTIMER) */
    if (cfg->baseId == 0u)
    {
        MOTOR_PWM_ApplyPwm(0u,
                           MOTOR_PWM_CTIMER0_PERIOD_CH,
                           cfg->pwmMatchCh,
                           duty);
    }
    else if (cfg->baseId == 3u)
    {
        MOTOR_PWM_ApplyPwm(3u,
                           MOTOR_PWM_CTIMER3_PERIOD_CH,
                           cfg->pwmMatchCh,
                           duty);
    }
}
