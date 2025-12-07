#ifndef I2C_RW612_H_
#define I2C_RW612_H_

#include <stdint.h>
#include "BITS.h"
#include "fsl_i2c.h"

/* Se usará FLEXCOMM2 / I2C2, igual que el ejemplo del SDK */
#define I2C_RW612_BASE              I2C2
#define I2C_RW612_INSTANCE_INDEX    (2u)

/* Frecuencia del bus I2C (puedes subir luego a 400kHz si el sensor lo soporta) */
#define I2C_RW612_BAUDRATE_HZ       (100000u)

/* Inicializa el I2C maestro (clock, baudrate, etc.) */
void I2C_RW612_Init(void);

/* Escritura de registros: reg + data[...] */
status_t I2C_RW612_WriteReg(uint8_t devAddr7,
                            uint8_t regAddr,
                            const uint8_t *data,
                            size_t dataLength);

/* Lectura de registros: reg -> data[...] */
status_t I2C_RW612_ReadReg(uint8_t devAddr7,
                           uint8_t regAddr,
                           uint8_t *data,
                           size_t dataLength);

#endif /* I2C_RW612_H_ */
