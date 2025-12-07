/*
 * MPU6050.h
 *
 *  Driver sencillo para sensor inercial MPU6050 sobre FRDM-RW612.
 *
 *  - Lectura por I2C (bus de codec: I2C2)
 *  - Conversión a unidades físicas (m/s^2 y deg/s)
 *  - Cálculo de ángulos pitch/roll con filtro complementario
 */

#ifndef MPU6050_H_
#define MPU6050_H_

#include <stdint.h>
#include "fsl_common.h"
#include "BITS.h"

/*******************************************************************************
 *  Definiciones
 ******************************************************************************/

/*! Dirección I2C 7 bits (AD0 = GND → 0x68) */
#define MPU6050_I2C_ADDR_7BIT      (0x68u)

/*! Frecuencia de I2C para el MPU6050 */
#define MPU6050_I2C_BAUDRATE       (100000u)

/*! Frecuencia de la gravedad para conversión a m/s^2 */
#define MPU6050_G_CONST            (9.81f)

/*! Sensibilidad por defecto con +/-2g y +/-250 deg/s */
#define MPU6050_ACCEL_LSB_PER_G    (16384.0f)
#define MPU6050_GYRO_LSB_PER_DPS   (131.0f)

/*******************************************************************************
 *  Estructuras de datos
 ******************************************************************************/

/*!
 * @brief Datos crudos y procesados del MPU6050
 */
typedef struct
{
    /* Lecturas crudas de registros (ADC) */
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;

    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;

    int16_t temp_raw;

    /* Valores físicos */
    float accel_x_mps2;
    float accel_y_mps2;
    float accel_z_mps2;

    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;

    float temperature_degC;

    /* Estimación de orientación (convención típica pitch/roll) */
    float pitch_deg;
    float roll_deg;

} MPU6050_DATA_T;

/*******************************************************************************
 *  API PÚBLICA
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Inicializa el bus I2C y configura el MPU6050.
 *
 * - Inicializa I2C2 si aún no está inicializado.
 * - Verifica registro WHO_AM_I.
 * - Configura PWR_MGMT_1, SMPLRT_DIV, CONFIG, GYRO_CONFIG, ACCEL_CONFIG.
 *
 * @return kStatus_Success si todo sale bien, otro status en caso de error.
 */
status_t MPU6050_Init(void);

/*!
 * @brief Lee una muestra nueva del sensor y actualiza filtros internos.
 *
 * @param dt_s Periodo de muestreo en segundos (ej. 0.01f para 100 Hz).
 *
 * @return kStatus_Success si la lectura por I2C fue correcta.
 */
status_t MPU6050_Update(float dt_s);

/*!
 * @brief Copia la última muestra disponible (cruda + procesada).
 *
 * @param out_data Puntero donde se almacenará la copia.
 */
void MPU6050_GetLastData(MPU6050_DATA_T *out_data);

/*!
 * @brief Obtiene únicamente los ángulos estimados.
 *
 * @param pitch_deg Puntero para devolver pitch en grados.
 * @param roll_deg  Puntero para devolver roll en grados.
 */
void MPU6050_GetAngles(float *pitch_deg, float *roll_deg);

#ifdef __cplusplus
}
#endif

#endif /* MPU6050_H_ */
