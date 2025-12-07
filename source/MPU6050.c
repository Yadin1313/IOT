/*
 * MPU6050.c
 */

#include "MPU6050.h"
#include <string.h>
#include <math.h>

#include "fsl_i2c.h"
#include "board.h"
#include "clock_config.h"
#include "BITS.h"

/*******************************************************************************
 *  Definiciones internas
 ******************************************************************************/

/* Usamos el mismo bus que el codec del board (I2C2 / FLEXCOMM2) */
#define MPU6050_I2C_BASE       BOARD_CODEC_I2C_BASEADDR
#define MPU6050_I2C_CLK_FREQ   BOARD_CODEC_I2C_CLOCK_FREQ

/* Registros principales del MPU6050 (según datasheet) */
#define MPU6050_REG_SMPLRT_DIV     (0x19u)
#define MPU6050_REG_CONFIG         (0x1Au)
#define MPU6050_REG_GYRO_CONFIG    (0x1Bu)
#define MPU6050_REG_ACCEL_CONFIG   (0x1Cu)
#define MPU6050_REG_INT_ENABLE     (0x38u)
#define MPU6050_REG_ACCEL_XOUT_H   (0x3Bu)
#define MPU6050_REG_TEMP_OUT_H     (0x41u)
#define MPU6050_REG_GYRO_XOUT_H    (0x43u)
#define MPU6050_REG_PWR_MGMT_1     (0x6Bu)
#define MPU6050_REG_WHO_AM_I       (0x75u)

/* Conversión de radianes a grados */
#define MPU6050_RAD2DEG            (57.2957795f)

/* Ganancia del filtro complementario (cercano a 1: más peso al giroscopio) */
#define MPU6050_COMPLEMENTARY_ALPHA (0.98f)

/*******************************************************************************
 *  Variables estáticas
 ******************************************************************************/

/*! Estructura con la última lectura */
static MPU6050_DATA_T s_mpu_data;

/*! Bandera de inicialización I2C */
static boolean_t s_i2c_initialized = FALSE;

/*! Bandera de inicialización del propio sensor */
static boolean_t s_mpu_initialized = FALSE;

/*******************************************************************************
 *  Funciones privadas (helpers)
 ******************************************************************************/

/*!
 * @brief Inicializa el I2C2 si aún no está listo.
 */
static status_t MPU6050_I2C_Init(void)
{
    if (s_i2c_initialized == TRUE)
    {
        return kStatus_Success;
    }

    i2c_master_config_t masterConfig;

    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = MPU6050_I2C_BAUDRATE;

    I2C_MasterInit(MPU6050_I2C_BASE, &masterConfig, MPU6050_I2C_CLK_FREQ);

    s_i2c_initialized = TRUE;
    return kStatus_Success;
}

/*!
 * @brief Escribe un byte en un registro del MPU6050.
 */
static status_t MPU6050_WriteReg(uint8_t reg_addr, uint8_t value)
{
    i2c_master_transfer_t xfer;
    memset(&xfer, 0, sizeof(xfer));

    xfer.slaveAddress   = MPU6050_I2C_ADDR_7BIT;
    xfer.direction      = kI2C_Write;
    xfer.subaddress     = reg_addr;
    xfer.subaddressSize = 1u;
    xfer.data           = &value;
    xfer.dataSize       = 1u;
    xfer.flags          = kI2C_TransferDefaultFlag;

    return I2C_MasterTransferBlocking(MPU6050_I2C_BASE, &xfer);
}

/*!
 * @brief Lee varios bytes empezando en reg_addr.
 */
static status_t MPU6050_ReadRegs(uint8_t reg_addr, uint8_t *buffer, size_t length)
{
    i2c_master_transfer_t xfer;
    memset(&xfer, 0, sizeof(xfer));

    xfer.slaveAddress   = MPU6050_I2C_ADDR_7BIT;
    xfer.direction      = kI2C_Read;
    xfer.subaddress     = reg_addr;
    xfer.subaddressSize = 1u;
    xfer.data           = buffer;
    xfer.dataSize       = (uint32_t)length;
    xfer.flags          = kI2C_TransferDefaultFlag;

    return I2C_MasterTransferBlocking(MPU6050_I2C_BASE, &xfer);
}

/*!
 * @brief Combina dos bytes alto/bajo en un int16_t.
 */
static int16_t MPU6050_CombineInt16(uint8_t msb, uint8_t lsb)
{
    return (int16_t)((((int16_t)msb) << 8) | (int16_t)lsb);
}

/*******************************************************************************
 *  Implementación de API
 ******************************************************************************/

status_t MPU6050_Init(void)
{
    status_t status;
    uint8_t who_am_i;

    status = MPU6050_I2C_Init();
    if (status != kStatus_Success)
    {
        return status;
    }

    /* Verificar WHO_AM_I */
    status = MPU6050_ReadRegs(MPU6050_REG_WHO_AM_I, &who_am_i, 1u);
    if (status != kStatus_Success)
    {
        return status;
    }

    /* WHO_AM_I debe ser 0x68 (ignorar bits reservados si fuera necesario) */
    if ((who_am_i & 0x7Eu) != 0x68u)
    {
        return kStatus_Fail;
    }

    /* Salir de sleep y seleccionar reloj interno */
    status = MPU6050_WriteReg(MPU6050_REG_PWR_MGMT_1, 0x00u);
    if (status != kStatus_Success)
    {
        return status;
    }

    /* Sample rate: con DLPF activado, Fs = 1kHz / (1 + SMPLRT_DIV)
     * SMPLRT_DIV = 9 → ~100 Hz
     */
    status = MPU6050_WriteReg(MPU6050_REG_SMPLRT_DIV, 9u);
    if (status != kStatus_Success)
    {
        return status;
    }

    /* CONFIG: DLPF = 3 (~44 Hz para accel/gyro) */
    status = MPU6050_WriteReg(MPU6050_REG_CONFIG, 0x03u);
    if (status != kStatus_Success)
    {
        return status;
    }

    /* GYRO_CONFIG: FS_SEL = 0 → ±250 deg/s */
    status = MPU6050_WriteReg(MPU6050_REG_GYRO_CONFIG, 0x00u);
    if (status != kStatus_Success)
    {
        return status;
    }

    /* ACCEL_CONFIG: AFS_SEL = 0 → ±2 g */
    status = MPU6050_WriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00u);
    if (status != kStatus_Success)
    {
        return status;
    }

    /* (Opcional) Habilitar interrupción de Data Ready si luego se usa por pin INT */
    status = MPU6050_WriteReg(MPU6050_REG_INT_ENABLE, 0x00u);
    if (status != kStatus_Success)
    {
        return status;
    }

    /* Inicializar estructura interna */
    memset(&s_mpu_data, 0, sizeof(s_mpu_data));
    s_mpu_data.pitch_deg = 0.0f;
    s_mpu_data.roll_deg  = 0.0f;

    s_mpu_initialized = TRUE;

    return kStatus_Success;
}

status_t MPU6050_Update(float dt_s)
{
    status_t status;
    uint8_t raw_buf[14];
    float accel_x_g, accel_y_g, accel_z_g;
    float gyro_x_dps, gyro_y_dps, gyro_z_dps;
    float acc_pitch_deg, acc_roll_deg;
    float gyro_pitch, gyro_roll;

    if ((s_mpu_initialized == FALSE) || (dt_s <= 0.0f))
    {
        return kStatus_Fail;
    }

    /* Leer: ACCEL_X/Y/Z, TEMP, GYRO_X/Y/Z (14 bytes desde ACCEL_XOUT_H) */
    status = MPU6050_ReadRegs(MPU6050_REG_ACCEL_XOUT_H, raw_buf, sizeof(raw_buf));
    if (status != kStatus_Success)
    {
        return status;
    }

    /* Reconstruir crudos */
    s_mpu_data.accel_x_raw = MPU6050_CombineInt16(raw_buf[0],  raw_buf[1]);
    s_mpu_data.accel_y_raw = MPU6050_CombineInt16(raw_buf[2],  raw_buf[3]);
    s_mpu_data.accel_z_raw = MPU6050_CombineInt16(raw_buf[4],  raw_buf[5]);

    s_mpu_data.temp_raw    = MPU6050_CombineInt16(raw_buf[6],  raw_buf[7]);

    s_mpu_data.gyro_x_raw  = MPU6050_CombineInt16(raw_buf[8],  raw_buf[9]);
    s_mpu_data.gyro_y_raw  = MPU6050_CombineInt16(raw_buf[10], raw_buf[11]);
    s_mpu_data.gyro_z_raw  = MPU6050_CombineInt16(raw_buf[12], raw_buf[13]);

    /* Conversión a "g" y luego a m/s^2 */
    accel_x_g = (float)s_mpu_data.accel_x_raw / MPU6050_ACCEL_LSB_PER_G;
    accel_y_g = (float)s_mpu_data.accel_y_raw / MPU6050_ACCEL_LSB_PER_G;
    accel_z_g = (float)s_mpu_data.accel_z_raw / MPU6050_ACCEL_LSB_PER_G;

    s_mpu_data.accel_x_mps2 = accel_x_g * MPU6050_G_CONST;
    s_mpu_data.accel_y_mps2 = accel_y_g * MPU6050_G_CONST;
    s_mpu_data.accel_z_mps2 = accel_z_g * MPU6050_G_CONST;

    /* Conversión a deg/s */
    gyro_x_dps = (float)s_mpu_data.gyro_x_raw / MPU6050_GYRO_LSB_PER_DPS;
    gyro_y_dps = (float)s_mpu_data.gyro_y_raw / MPU6050_GYRO_LSB_PER_DPS;
    gyro_z_dps = (float)s_mpu_data.gyro_z_raw / MPU6050_GYRO_LSB_PER_DPS;

    s_mpu_data.gyro_x_dps = gyro_x_dps;
    s_mpu_data.gyro_y_dps = gyro_y_dps;
    s_mpu_data.gyro_z_dps = gyro_z_dps;

    /* Temperatura (fórmula del datasheet) */
    s_mpu_data.temperature_degC = ((float)s_mpu_data.temp_raw / 340.0f) + 36.53f;

    /* Cálculo de ángulos a partir del acelerómetro */
    acc_roll_deg  = atan2f(s_mpu_data.accel_y_mps2,
                           sqrtf(s_mpu_data.accel_x_mps2 * s_mpu_data.accel_x_mps2 +
                                 s_mpu_data.accel_z_mps2 * s_mpu_data.accel_z_mps2))
                    * MPU6050_RAD2DEG;

    acc_pitch_deg = atan2f(-s_mpu_data.accel_x_mps2,
                           sqrtf(s_mpu_data.accel_y_mps2 * s_mpu_data.accel_y_mps2 +
                                 s_mpu_data.accel_z_mps2 * s_mpu_data.accel_z_mps2))
                    * MPU6050_RAD2DEG;

    /* Integración del giroscopio (ejes a revisar según montaje físico):
     *  - Supongamos pitch alrededor de eje Y
     *  - roll  alrededor de eje X
     */
    gyro_pitch = s_mpu_data.pitch_deg + gyro_y_dps * dt_s;
    gyro_roll  = s_mpu_data.roll_deg  + gyro_x_dps * dt_s;

    /* Filtro complementario: combinamos gyro (alta frecuencia) + accel (baja) */
    s_mpu_data.pitch_deg = MPU6050_COMPLEMENTARY_ALPHA * gyro_pitch
                         + (1.0f - MPU6050_COMPLEMENTARY_ALPHA) * acc_pitch_deg;

    s_mpu_data.roll_deg  = MPU6050_COMPLEMENTARY_ALPHA * gyro_roll
                         + (1.0f - MPU6050_COMPLEMENTARY_ALPHA) * acc_roll_deg;

    return kStatus_Success;
}

void MPU6050_GetLastData(MPU6050_DATA_T *out_data)
{
    if (out_data == NULL)
    {
        return;
    }

    *out_data = s_mpu_data;
}

void MPU6050_GetAngles(float *pitch_deg, float *roll_deg)
{
    if (pitch_deg != NULL)
    {
        *pitch_deg = s_mpu_data.pitch_deg;
    }

    if (roll_deg != NULL)
    {
        *roll_deg = s_mpu_data.roll_deg;
    }
}
