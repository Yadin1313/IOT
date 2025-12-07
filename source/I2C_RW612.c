#include "I2C_RW612.h"
#include "clock_config.h"
#include "board.h"
#include "pin_mux.h"

#define I2C_RW612_MASTER ((I2C_Type *)I2C_RW612_BASE)

void I2C_RW612_Init(void)
{
    i2c_master_config_t masterConfig;
    uint32_t srcClock_Hz;

    /* Adjuntar reloj a FLEXCOMM2 (ajusta si usas otro flexcomm) */
    CLOCK_AttachClk(kSFRO_to_FLEXCOMM2);

    BOARD_InitBootPins();
    BOARD_InitBootClocks();

    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = I2C_RW612_BAUDRATE_HZ;

    srcClock_Hz = CLOCK_GetFlexCommClkFreq(I2C_RW612_INSTANCE_INDEX);
    I2C_MasterInit(I2C_RW612_MASTER, &masterConfig, srcClock_Hz);
}

/* Escritura: START + addr(w) + reg + data[...] + STOP */
status_t I2C_RW612_WriteReg(uint8_t devAddr7,
                            uint8_t regAddr,
                            const uint8_t *data,
                            size_t dataLength)
{
    status_t status;

    status = I2C_MasterStart(I2C_RW612_MASTER, devAddr7, kI2C_Write);
    if (status != kStatus_Success)
        return status;

    /* Enviar dirección de registro */
    status = I2C_MasterWriteBlocking(I2C_RW612_MASTER,
                                     &regAddr,
                                     1u,
                                     kI2C_TransferNoStopFlag);
    if (status != kStatus_Success)
    {
        I2C_MasterStop(I2C_RW612_MASTER);
        return status;
    }

    /* Enviar datos */
    status = I2C_MasterWriteBlocking(I2C_RW612_MASTER,
                                     data,
                                     dataLength,
                                     kI2C_TransferDefaultFlag);
    if (status != kStatus_Success)
    {
        I2C_MasterStop(I2C_RW612_MASTER);
        return status;
    }

    return I2C_MasterStop(I2C_RW612_MASTER);
}

/* Lectura: START + addr(w) + reg + REP_START + addr(r) + data[...] + STOP */
status_t I2C_RW612_ReadReg(uint8_t devAddr7,
                           uint8_t regAddr,
                           uint8_t *data,
                           size_t dataLength)
{
    status_t status;

    status = I2C_MasterStart(I2C_RW612_MASTER, devAddr7, kI2C_Write);
    if (status != kStatus_Success)
        return status;

    status = I2C_MasterWriteBlocking(I2C_RW612_MASTER,
                                     &regAddr,
                                     1u,
                                     kI2C_TransferNoStopFlag);
    if (status != kStatus_Success)
    {
        I2C_MasterStop(I2C_RW612_MASTER);
        return status;
    }

    status = I2C_MasterRepeatedStart(I2C_RW612_MASTER, devAddr7, kI2C_Read);
    if (status != kStatus_Success)
    {
        I2C_MasterStop(I2C_RW612_MASTER);
        return status;
    }

    status = I2C_MasterReadBlocking(I2C_RW612_MASTER,
                                    data,
                                    dataLength,
                                    kI2C_TransferDefaultFlag);
    if (status != kStatus_Success)
    {
        I2C_MasterStop(I2C_RW612_MASTER);
        return status;
    }

    return I2C_MasterStop(I2C_RW612_MASTER);
}
