#ifndef MOTOR_PWM_H_
#define MOTOR_PWM_H_

#include "fsl_common.h"
#include "fsl_io_mux.h"
#include "BITS.h"

/* Identificadores de motor */
typedef enum
{
    MOTOR_ID_SLIDER = 0u,
    MOTOR_ID_PAN,
    MOTOR_ID_TILT,
    MOTOR_ID_COUNT
} MOTOR_ID_T;

/*!
 * @brief Inicializa CTIMER0/3 y los pines PWM/dir para los 3 motores.
 */
void MOTOR_PWM_Init(void);

/*!
 * @brief Aplica una salida normalizada al motor.
 *
 * @param motor   MOTOR_ID_SLIDER / MOTOR_ID_PAN / MOTOR_ID_TILT
 * @param u_norm  Rango [-1.0, 1.0]
 *                signo = sentido, |u| = duty (0..100%)
 */
void MOTOR_PWM_SetOutput(MOTOR_ID_T motor, float u_norm);

#endif /* MOTOR_PWM_H_ */
