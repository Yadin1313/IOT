/*
 * SYSTEM_MANAGER.h
 *
 * Módulo principal de gestión del sistema (nivel debajo de main).
 * - Inicializa drivers de hardware.
 * - Crea colas, mutex y event groups de FreeRTOS.
 * - Crea las tareas principales:
 * * CONTROL_Task
 * * SENSORS_Task
 * * COMM_Task
 * * LOGGER_Task
 * * STATUS_Task
 */

#ifndef SYSTEM_MANAGER_H_
#define SYSTEM_MANAGER_H_

#include <stdint.h>
#include "BITS.h"
#include "SYSTEM_TYPES.h"
#include "fsl_debug_console.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "board.h"
#include "clock_config.h"
#include "pin_mux.h"
#include "fsl_common.h"
#include "math.h"

/* Drivers del Proyecto */
#include "GPIO_RW612.h"
#include "MOTOR_PWM.h"
#include "PID_CONTROL.h"
#include "I2C_RW612.h"
#include "VL53L0X.h"
#include "MPU6050.h"

/*-----------------------------------------------------------------------------
 * DEFINES: PRIORIDADES Y PERIODOS
 *----------------------------------------------------------------------------*/

/* Prioridades (ajustadas para FreeRTOS) */
#define CONTROL_TASK_PRIORITY       (configMAX_PRIORITIES - 2)  /* Alta: Control en tiempo real */
#define SENSORS_TASK_PRIORITY       (configMAX_PRIORITIES - 3)  /* Media: Adquisición de datos */
#define COMM_TASK_PRIORITY          (configMAX_PRIORITIES - 3)  /* Media: Comunicaciones */
#define LOGGER_TASK_PRIORITY        (tskIDLE_PRIORITY + 1)      /* Baja: Logging a consola */
#define STATUS_TASK_PRIORITY        (tskIDLE_PRIORITY + 1)      /* Baja: Parpadeo LEDs */

/* Periodos en milisegundos */
#define CONTROL_TASK_PERIOD_MS      (1u)   /* 1 kHz loop */
#define SENSORS_TASK_PERIOD_MS      (10u)  /* 100 Hz loop */
#define STATUS_TASK_PERIOD_MS       (200u) /* 5 Hz loop */

/* * Stack de cada tarea.
 * NOTA: Si experimentas Stack Overflow (LED Rojo + mensaje en consola),
 * incrementa estos multiplicadores (ej. cambiar *4u por *6u).
 */
#define CONTROL_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE * 4u)
#define SENSORS_TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE * 3u)
#define COMM_TASK_STACK_SIZE        (configMINIMAL_STACK_SIZE * 4u)
#define LOGGER_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE * 3u)
#define STATUS_TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE * 2u)

/*-----------------------------------------------------------------------------
 * HANDLES GLOBALES DE FREERTOS
 *----------------------------------------------------------------------------*/

extern QueueHandle_t      g_qSensorsToControl;
extern QueueHandle_t      g_qRefsToControl;
extern QueueHandle_t      g_qLogSamples;
extern SemaphoreHandle_t  g_mutexI2C;
extern EventGroupHandle_t g_evtSystemStatus;

/* Bits de estado para g_evtSystemStatus */
#define EVT_SYS_OK_BIT              (1u << 0)
#define EVT_SYS_ERROR_SENSOR_BIT    (1u << 1)
#define EVT_SYS_ERROR_COMM_BIT      (1u << 2)
#define EVT_SYS_LOGGING_ACTIVE_BIT  (1u << 3)

/*-----------------------------------------------------------------------------
 * API PÚBLICA
 *----------------------------------------------------------------------------*/

/*!
 * @brief Inicializa drivers, colas y tareas de FreeRTOS.
 * Debe llamarse antes de arrancar el scheduler.
 */
void SYSTEM_MANAGER_Init(void);

/*!
 * @brief Inyecta una nueva referencia proveniente de la Mac/YOLO.
 *
 * @param slide  Desplazamiento deseado (unidades arbitrarias o mm)
 * @param yaw    Ángulo Pan deseado (grados)
 * @param pitch  Ángulo Tilt deseado (grados)
 *
 * Esta función es llamada desde main.c cuando llega un paquete UDP.
 */
void SYSTEM_MANAGER_SetYoloRef(int32_t slide, int32_t yaw, int32_t pitch);

#endif /* SYSTEM_MANAGER_H_ */
