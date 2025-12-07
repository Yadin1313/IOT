/*
 * PID_CONTROL.c
 *
 *  Implementación de controlador PID discreto para 3 ejes.
 */

#include "PID_CONTROL.h"

/*-----------------------------------------------------------------------------
 *  MACROS PRIVADOS
 *----------------------------------------------------------------------------*/

#define SATURAR(x, min, max)              \
    do {                                  \
        if ((x) > (max)) (x) = (max);     \
        else if ((x) < (min)) (x) = (min);\
    } while (0)

/*-----------------------------------------------------------------------------
 *  VARIABLES INTERNAS
 *----------------------------------------------------------------------------*/

/* Un PID por cada motor (Slider, Pan, Tilt).
 * Usamos MOTOR_ID_COUNT como tamaño del arreglo.
 */
static PID_CONTROL_T g_pid_control[MOTOR_ID_COUNT];

/*-----------------------------------------------------------------------------
 *  FUNCIONES PRIVADAS
 *----------------------------------------------------------------------------*/

/*!
 * @brief Helper interno para obtener puntero al PID correcto.
 */
static PID_CONTROL_T * PID_CONTROL_GetPtr(MOTOR_ID_T motor_id)
{
    if (motor_id >= MOTOR_ID_COUNT)
    {
        return NULL;
    }
    return &g_pid_control[motor_id];
}

/*-----------------------------------------------------------------------------
 *  IMPLEMENTACIÓN DE API
 *----------------------------------------------------------------------------*/

void PID_CONTROL_Init(float Ts)
{
    uint8_t i;

    for (i = 0u; i < (uint8_t)MOTOR_ID_COUNT; i++)
    {
        g_pid_control[i].Kp    = 0.0f;
        g_pid_control[i].Ki    = 0.0f;
        g_pid_control[i].Kd    = 0.0f;
        g_pid_control[i].Ts    = Ts;

        g_pid_control[i].e_prev = 0.0f;
        g_pid_control[i].uI     = 0.0f;

        /* Límites por defecto: salida normalizada [-1, 1] */
        g_pid_control[i].u_min = -1.0f;
        g_pid_control[i].u_max =  1.0f;
    }
}

void PID_CONTROL_SetGains(MOTOR_ID_T motor_id, float Kp, float Ki, float Kd)
{
    PID_CONTROL_T *pid = PID_CONTROL_GetPtr(motor_id);

    if (pid == NULL)
    {
        return;
    }

    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
}

void PID_CONTROL_SetLimits(MOTOR_ID_T motor_id, float u_min, float u_max)
{
    PID_CONTROL_T *pid = PID_CONTROL_GetPtr(motor_id);

    if (pid == NULL)
    {
        return;
    }

    pid->u_min = u_min;
    pid->u_max = u_max;
}

void PID_CONTROL_Reset(MOTOR_ID_T motor_id)
{
    PID_CONTROL_T *pid = PID_CONTROL_GetPtr(motor_id);

    if (pid == NULL)
    {
        return;
    }

    pid->e_prev = 0.0f;
    pid->uI     = 0.0f;
}

/*!
 * @brief Implementación directa del PID discreto visto en clase:
 *
 *   e_k   = ref - y;
 *   uP_k  = Kp * e_k;
 *   uI_k  = uI_{k-1} + Ki * Ts * e_k;
 *   uD_k  = Kd / Ts * (e_k - e_{k-1});
 *   u_k   = uP_k + uI_k + uD_k;
 */
float PID_CONTROL_Update(MOTOR_ID_T motor_id, float ref, float y)
{
    PID_CONTROL_T *pid = PID_CONTROL_GetPtr(motor_id);
    float e_k;
    float uP, uI, uD;
    float u_raw;
    float u_sat;

    if (pid == NULL)
    {
        return 0.0f;
    }

    /* 1) Error actual */
    e_k = ref - y;

    /* 2) Término proporcional */
    uP = pid->Kp * e_k;

    /* 3) Término integral (estado interno uI) */
    uI = pid->uI + pid->Ki * pid->Ts * e_k;

    /* 4) Término derivativo (en el error) */
    if (pid->Ts > 0.0f)
    {
        uD = pid->Kd * (e_k - pid->e_prev) / pid->Ts;
    }
    else
    {
        uD = 0.0f;
    }

    /* 5) Suma de términos */
    u_raw = uP + uI + uD;
    u_sat = u_raw;

    /* 6) Saturación de la salida */
    SATURAR(u_sat, pid->u_min, pid->u_max);

    /* 7) Anti-windup simple:
     *    Si se saturó, ajustamos el integrador para que
     *    uP + uI + uD coincida con u_sat.
     */
    if (u_raw != u_sat)
    {
        uI = u_sat - (uP + uD);
    }

    /* 8) Actualizar estados internos para la próxima muestra */
    pid->uI     = uI;
    pid->e_prev = e_k;

    return u_sat;
}
