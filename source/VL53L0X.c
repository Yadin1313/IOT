#include "VL53L0X.h"

status_t VL53L0X_Init(void)
{
    status_t status;
    uint8_t mode;

    /* Config mínima: poner SYSRANGE_START en modo back-to-back (continuo simple)
     *
     * En una implementación más completa se debería:
     *  - Hacer soft reset
     *  - Cargar ajustes recomendados de ST
     *  - Realizar calibración si es necesaria (SPAD, VHV, etc.)
     *  Para el prototipo, esto suele ser suficiente para empezar.
     */

    mode = VL53L0X_SYSRANGE_MODE_BACKTOBACK;
    status = I2C_RW612_WriteReg(VL53L0X_I2C_ADDR_7BIT,
                                VL53L0X_REG_SYSRANGE_START,
                                &mode,
                                1u);
    return status;
}

status_t VL53L0X_ReadDistanceMm(uint16_t *distance_mm)
{
    status_t status;
    uint8_t buf[2];

    if (distance_mm == NULL)
    {
        return kStatus_InvalidArgument;
    }

    /* Leer MSB y LSB de la distancia */
    status = I2C_RW612_ReadReg(VL53L0X_I2C_ADDR_7BIT,
                               VL53L0X_REG_RESULT_MSRANGE_HIGH,
                               buf,
                               2u);
    if (status != kStatus_Success)
    {
        return status;
    }

    *distance_mm = ((uint16_t)buf[0] << 8) | buf[1];

    return kStatus_Success;
}
