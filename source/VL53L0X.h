#ifndef VL53L0X_H_
#define VL53L0X_H_

#include <stdint.h>
#include "BITS.h"
#include "I2C_RW612.h"

/* Dirección I2C 7-bit por defecto del VL53L0X */
#define VL53L0X_I2C_ADDR_7BIT      (0x29u)

/* Registros útiles (ver datasheet / register map) */
#define VL53L0X_REG_SYSRANGE_START         (0x00u)
#define VL53L0X_REG_RESULT_MSRANGE_HIGH    (0x1Eu)  /* MSB distancia */
#define VL53L0X_REG_RESULT_MSRANGE_LOW     (0x1Fu)  /* LSB distancia */

/* Modo back-to-back / continuo (bit1 = 1) */
#define VL53L0X_SYSRANGE_MODE_BACKTOBACK   (0x02u)

/*!
 * @brief Inicializa el VL53L0X en modo de adquisición continua simple.
 *
 *  - Supone que el sensor ya está alimentado y con pines configurados.
 *  - Usa I2C_RW612 para escribir la configuración mínima.
 */
status_t VL53L0X_Init(void);

/*!
 * @brief Lee la distancia actual medida por el sensor.
 *
 * @param distance_mm  Puntero donde se guardará la distancia [mm].
 *
 * @return kStatus_Success si la lectura fue correcta.
 */
status_t VL53L0X_ReadDistanceMm(uint16_t *distance_mm);

#endif /* VL53L0X_H_ */
