/*
    mpu6050.h -- Library for MPU-6050
    Created by Yizheng Liao, 9/6/2016
*/
#ifndef MPU6050_H
#define MPU6050_H
#include "Arduino.h"

#define _BV(bit) (1 << (bit))

// Default I2C address for the MPU-6050 is 0x68.
// But only if the AD0 pin is low.
// Some sensor boards have AD0 high, and the
// I2C address thus becomes 0x69.
#define MPU6050_I2C_ADDRESS 0x68

// MPU6050 Configuration bits
#define MPU6050_SMPLRT_DIV             0x19
#define MPU6050_CONFIG                 0x1a
#define MPU6050_GYRO_CONFIG            0x1b
#define MPU6050_ACCEL_CONFIG           0x1c

// address for data buffer
#define MPU6050_ACCEL_XOUT_H       0x3B   // R
#define MPU6050_ACCEL_XOUT_L       0x3C   // R
#define MPU6050_ACCEL_YOUT_H       0x3D   // R
#define MPU6050_ACCEL_YOUT_L       0x3E   // R
#define MPU6050_ACCEL_ZOUT_H       0x3F   // R
#define MPU6050_ACCEL_ZOUT_L       0x40   // R
#define MPU6050_TEMP_OUT_H         0x41   // R
#define MPU6050_TEMP_OUT_L         0x42   // R
#define MPU6050_GYRO_XOUT_H        0x43   // R
#define MPU6050_GYRO_XOUT_L        0x44   // R
#define MPU6050_GYRO_YOUT_H        0x45   // R
#define MPU6050_GYRO_YOUT_L        0x46   // R
#define MPU6050_GYRO_ZOUT_H        0x47   // R
#define MPU6050_GYRO_ZOUT_L        0x48   // R

// power buffer
#define MPU6050_PWR_MGMT_1         0x6B   // R/W
#define MPU6050_PWR_MGMT_2         0x6C   // R/W

// SWAP function
#define SWAP(a,b) a = a^b; b = a^b; a = a^b;

// data structure that holds low byte and upper byte
typedef struct
{
    /* data */
    int8_t x_accel_h;
    int8_t x_accel_l;
    int8_t y_accel_h;
    int8_t y_accel_l;
    int8_t z_accel_h;
    int8_t z_accel_l;
    int8_t t_h;
    int8_t t_l;
    int8_t x_gyro_h;
    int8_t x_gyro_l;
    int8_t y_gyro_h;
    int8_t y_gyro_l;
    int8_t z_gyro_h;
    int8_t z_gyro_l;
}MPU6050_LH;

typedef struct
{
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t temperature;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
}MPU6050_VALUE;

typedef union
{
    MPU6050_LH reg;
    MPU6050_VALUE value;
}MPU6050_DATA_UNION;

typedef struct
{
    uint32_t ms_time;
    MPU6050_VALUE value;
}MPU6050_VALUE_TS;

#define MPU6050_DATA_SIZE (sizeof(MPU6050_VALUE))

#define MPU6050_VALUE_TS_SIZE (sizeof(MPU6050_VALUE_TS))

class MPU6050
{
    public:
        MPU6050();
        void enable();
        void reset();
        int sample(void);
        MPU6050_VALUE get(void);
        uint8_t setAccRange(uint8_t range);
        uint8_t setGyroRange(uint8_t range);
        int16_t write(int start, const uint8_t *pData, int size);
        int16_t writeReg(int reg, uint8_t data);
        int16_t read(int start, uint8_t *buffer, int size);
        uint8_t sleepMode(uint8_t status);
        uint8_t cycleMode(uint8_t status);
        int32_t sampleSum(void);
        MPU6050_VALUE_TS getTs(void);
        void getTs(MPU6050_VALUE_TS* data_ts);
        String getString(MPU6050_VALUE data);
        String getString(MPU6050_VALUE_TS data);
        void disp(MPU6050_VALUE data);
        void disp(MPU6050_VALUE_TS data);
    private:
        int16_t _error;
        MPU6050_DATA_UNION _data;
        uint32_t _ts;
        // char _data_array[MPU6050_DATA_SIZE];
        // char _data_ts_array[MPU6050_VALUE_TS_SIZE];

};

#endif /*MPU6050_H*/
