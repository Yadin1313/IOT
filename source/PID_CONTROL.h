/*
 * PID_CONTROL.h
 *
 *  Módulo de Control PID discreto para 3 ejes:
 *  - Slider
 *  - Pan
 *  - Tilt
 */

#ifndef PID_CONTROL_H_
#define PID_CONTROL_H_

#include <stdint.h>
#include "BITS.h"
#include "MOTOR_PWM.h"   /* Para MOTOR_ID_T y MOTOR_ID_COUNT */

/*-----------------------------------------------------------------------------
 *  TIPOS PÚBLICOS
 *----------------------------------------------------------------------------*/

typedef struct
{
    float Kp;      /*!< Ganancia proporcional            */
    float Ki;      /*!< Ganancia integral                */
    float Kd;      /*!< Ganancia derivativa             */
    float Ts;      /*!< Periodo de muestreo [s]         */

    float e_prev;  /*!< Error en la muestra anterior     */
    float uI;      /*!< Estado integrador (uI_k)         */

    float u_min;   /*!< Límite inferior de salida        */
    float u_max;   /*!< Límite superior de salida        */
} PID_CONTROL_T;

/* Nota importante:
 *  El arreglo interno de PIDs se maneja dentro de PID_CONTROL.c.
 *  No exponemos g_pid_control aquí para evitar compartir datos
 *  globales entre módulos. Se accede vía las funciones de esta API.
 */

/*-----------------------------------------------------------------------------
 *  API PÚBLICA
 *----------------------------------------------------------------------------*/

/*!
 * @brief Inicializa las estructuras PID con valores por defecto.
 *
 * @param Ts   Periodo de muestreo en segundos (mismo para los 3 ejes).
 */
void PID_CONTROL_Init(float Ts);

/*!
 * @brief Configura las ganancias de un PID específico.
 */
void PID_CONTROL_SetGains(MOTOR_ID_T motor_id, float Kp, float Ki, float Kd);

/*!
 * @brief Configura los límites de salida (saturación) de un PID.
 */
void PID_CONTROL_SetLimits(MOTOR_ID_T motor_id, float u_min, float u_max);

/*!
 * @brief Reinicia el estado interno (error previo e integrador) de un PID.
 */
void PID_CONTROL_Reset(MOTOR_ID_T motor_id);

/*!
 * @brief Ejecuta un paso de control PID discreto para el motor indicado.
 *
 * @return u_k  Salida del PID (ya saturada entre [u_min, u_max]).
 */
float PID_CONTROL_Update(MOTOR_ID_T motor_id, float ref, float y);

#endif /* PID_CONTROL_H_ */
