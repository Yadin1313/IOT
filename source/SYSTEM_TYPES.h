/*
 * SYSTEM_TYPES.h
 *
 *  Definición de estructuras de datos de alto nivel para el sistema PanTrack:
 *   - Mensajes de sensores  (SENSOR_SAMPLE_T)
 *   - Mensajes de referencia (CONTROL_REF_T)
 *   - Mensajes de logging   (LOG_SAMPLE_T)
 *
 *  Estas estructuras se usarán principalmente en colas de FreeRTOS:
 *    - g_qSensorsToControl
 *    - g_qRefsToControl
 *    - g_qLogSamples
 */

#ifndef SYSTEM_TYPES_H_
#define SYSTEM_TYPES_H_

/*-----------------------------------------------------------------------------
 *  INCLUDES
 *----------------------------------------------------------------------------*/

#include <stdint.h>
#include "BITS.h"

/*-----------------------------------------------------------------------------
 *  ENUMS Y TIPOS ASOCIADOS
 *----------------------------------------------------------------------------*/

/*!
 * @brief Origen de la referencia de control.
 *
 *  CONTROL_REF_SOURCE_MANUAL : referencia impuesta localmente (ej. pruebas).
 *  CONTROL_REF_SOURCE_VISION : referencia proveniente de la PicoPi / YOLO.
 *  CONTROL_REF_SOURCE_TEST   : referencias generadas internamente (senos,
 *                              cosenos, escalones, etc. para sintonización).
 */
typedef enum
{
    CONTROL_REF_SOURCE_MANUAL = 0u,
    CONTROL_REF_SOURCE_VISION,
    CONTROL_REF_SOURCE_TEST
} CONTROL_REF_SOURCE_T;

/*-----------------------------------------------------------------------------
 *  MENSAJE: SENSORES → CONTROL
 *----------------------------------------------------------------------------*/

/*!
 * @brief Muestra de sensores enviada desde SENSORS_Task hacia CONTROL_Task.
 *
 *  Contiene la información medida del sistema en un instante:
 *    - Posición del slider (en metros).
 *    - Ángulos pan/tilt (en grados).
 *    - Medida cruda del VL53L0X (en mm).
 *    - Flags de validez de los sensores.
 */
typedef struct
{
    uint32_t timestamp_ms;   /*!< Marca de tiempo en milisegundos */

    /* --- Slider (eje lineal) --- */
    float    slider_pos_m;   /*!< Posición del slider en metros */
    uint16_t slider_raw_mm;  /*!< Distancia cruda del VL53L0X [mm] */
    uint8_t  slider_valid;   /*!< TRUE si la medición de slider es válida */

    /* --- Pan/Tilt (ejes angulares) --- */
    float    pan_deg;        /*!< Ángulo PAN medido [deg] */
    float    tilt_deg;       /*!< Ángulo TILT medido [deg] */
    uint8_t  imu_valid;      /*!< TRUE si los datos del IMU (MPU6050) son válidos */

} SENSOR_SAMPLE_T;

/*-----------------------------------------------------------------------------
 *  MENSAJE: REFERENCIAS → CONTROL
 *----------------------------------------------------------------------------*/

/*!
 * @brief Referencias de control enviadas hacia CONTROL_Task.
 *
 *  Puede provenir de:
 *    - Modo manual (joystick, teclas, GUI local).
 *    - Visión (YOLO en PicoPi).
 *    - Rutinas de prueba internas (barridos seno/coseno).
 */
typedef struct
{
    uint32_t timestamp_ms;   /*!< Marca de tiempo de recepción/generación */
    uint32_t frame_id;       /*!< ID de frame o secuencia (desde PicoPi, opcional) */

    float    slider_ref_m;   /*!< Referencia de posición del slider [m] */
    float    pan_ref_deg;    /*!< Referencia de ángulo PAN [deg] */
    float    tilt_ref_deg;   /*!< Referencia de ángulo TILT [deg] */

    CONTROL_REF_SOURCE_T source; /*!< Origen de la referencia */
    uint8_t              valid;  /*!< TRUE si la referencia es válida */

} CONTROL_REF_T;

/*-----------------------------------------------------------------------------
 *  MENSAJE: CONTROL → LOGGER (LOGGING PARA MATLAB)
 *----------------------------------------------------------------------------*/

/*!
 * @brief Muestra de logging generada por CONTROL_Task para análisis en MATLAB.
 *
 *  Incluye, para cada eje:
 *    - Referencia.
 *    - Medida.
 *    - Señal de control (u).
 *
 *  Además, contiene flags de validez y saturación, útiles para el análisis
 *  de desempeño y la sintonización de PID sobre el sistema real.
 */
typedef struct
{
    uint32_t timestamp_ms;   /*!< Marca de tiempo del ciclo de control */

    /* --- Eje SLIDER --- */
    float    slider_ref_m;   /*!< Referencia de posición [m] */
    float    slider_meas_m;  /*!< Posición medida [m] */
    float    slider_u;       /*!< Salida del PID (normalizada, p.ej. [-1, 1]) */

    /* --- Eje PAN --- */
    float    pan_ref_deg;    /*!< Referencia de ángulo [deg] */
    float    pan_meas_deg;   /*!< Medida de ángulo [deg] */
    float    pan_u;          /*!< Salida del PID (normalizada) */

    /* --- Eje TILT --- */
    float    tilt_ref_deg;   /*!< Referencia de ángulo [deg] */
    float    tilt_meas_deg;  /*!< Medida de ángulo [deg] */
    float    tilt_u;         /*!< Salida del PID (normalizada) */

    /* --- Flags de estado para análisis --- */
    uint8_t  sensors_ok;     /*!< TRUE si las mediciones usadas eran válidas */
    uint8_t  refs_ok;        /*!< TRUE si la referencia usada era válida */
    uint8_t  sat_slider;     /*!< TRUE si u_slider estuvo saturado en este ciclo */
    uint8_t  sat_pan;        /*!< TRUE si u_pan saturó en este ciclo */
    uint8_t  sat_tilt;       /*!< TRUE si u_tilt saturó en este ciclo */

} LOG_SAMPLE_T;

#endif /* SYSTEM_TYPES_H_ */
