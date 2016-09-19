#include "Arduino.h"
#include "Wire.h"
#include "MPU6050.h"

MPU6050::MPU6050(void)
{
}

// --------------------------------------------------------
void MPU6050::reset(void)
{
    writeReg(MPU6050_PWR_MGMT_1, 0);
}

// --------------------------------------------------------
void MPU6050::enable(void)
{
    reset();
}

// --------------------------------------------------------
int MPU6050::sample(void)
{
    int error;
    _ts = millis();
    error = read(MPU6050_ACCEL_XOUT_H, (uint8_t*)&_data, MPU6050_DATA_SIZE);

    SWAP(_data.reg.x_accel_h, _data.reg.x_accel_l);
    SWAP(_data.reg.y_accel_h, _data.reg.y_accel_l);
    SWAP(_data.reg.z_accel_h, _data.reg.z_accel_l);
    SWAP(_data.reg.t_h, _data.reg.t_l);
    SWAP(_data.reg.x_gyro_h, _data.reg.x_gyro_l);
    SWAP(_data.reg.y_gyro_h, _data.reg.y_gyro_l);
    SWAP(_data.reg.z_gyro_h, _data.reg.z_gyro_l);

    return error;
}

// --------------------------------------------------------
int32_t MPU6050::sampleSum(void)
{
    int32_t sample_sum = 0;
    sample_sum +=  _data.value.accel_x;
    sample_sum +=  _data.value.accel_y;
    sample_sum +=  _data.value.accel_z;
    sample_sum +=  _data.value.temperature;
    sample_sum +=  _data.value.gyro_x;
    sample_sum +=  _data.value.gyro_y;
    sample_sum +=  _data.value.gyro_z;

    return sample_sum;
}

// --------------------------------------------------------
MPU6050_VALUE MPU6050::get(void)
{
    return _data.value;
}

// --------------------------------------------------------
MPU6050_VALUE_TS MPU6050::getTs(void)
{
    MPU6050_VALUE_TS data_ts;
    data_ts.ms_time = _ts;
    data_ts.value = _data.value;
    return data_ts;
}

// --------------------------------------------------------
String MPU6050::getString(MPU6050_VALUE data)
{
    String data_str = String(data.accel_x, DEC);
    data_str += "," + String(data.accel_y, DEC);
    data_str += "," + String(data.accel_z, DEC);
    data_str += "," + String(data.temperature, DEC);
    data_str += "," + String(data.gyro_x, DEC);
    data_str += "," + String(data.gyro_y, DEC);
    data_str += "," + String(data.gyro_z, DEC);

    return data_str;
}

// --------------------------------------------------------
String MPU6050::getString(MPU6050_VALUE_TS data)
{
    String data_str = String(data.ms_time, DEC) + ",";
    data_str += getString(data.value);

    return data_str;
}

// --------------------------------------------------------
void MPU6050::disp(MPU6050_VALUE data)
{
    Serial.println(getString(data));
}

// --------------------------------------------------------
void MPU6050::disp(MPU6050_VALUE_TS data)
{
    Serial.println(getString(data));
}

// --------------------------------------------------------
uint8_t MPU6050::setAccRange(uint8_t range)
{
    uint8_t MPU_config = 0;
    read(MPU6050_ACCEL_CONFIG, &MPU_config, 1);

    switch (range)
    {
        case 0: // -/+2g
            MPU_config = MPU_config & ~_BV(3);
            break;
        case 1: // -/+4g
            MPU_config = MPU_config | _BV(3);
            break;
        case 2: // -/+8g
            MPU_config = MPU_config & ~_BV(4);
            break;
        case 3: // -/+16g
            MPU_config = MPU_config | _BV(4);
            break;
    }

    writeReg(MPU6050_ACCEL_CONFIG, MPU_config);
    return MPU_config;
}

// --------------------------------------------------------
uint8_t MPU6050::setGyroRange(uint8_t range)
{
    uint8_t MPU_config = 0;
    read(MPU6050_GYRO_CONFIG, &MPU_config, 1);

    switch (range)
    {
        case 0: // -/+250 o/s
            MPU_config = MPU_config & ~_BV(3);
            break;
        case 1: // -/+500 o/s
            MPU_config = MPU_config | _BV(3);
            break;
        case 2: // -/+1000 o/s
            MPU_config = MPU_config & ~_BV(4);
            break;
        case 3: // -/+2000 o/s
            MPU_config = MPU_config | _BV(4);
            break;
    }

    writeReg(MPU6050_GYRO_CONFIG, MPU_config);
    return MPU_config;
}

// --------------------------------------------------------
uint8_t MPU6050::sleepMode(uint8_t status)
{
    // status = 0, disable
    // status = 1, enable
    uint8_t MPU_config = 0;
    read(MPU6050_PWR_MGMT_1, &MPU_config, 1);

    if (status == 0)
        MPU_config = MPU_config & ~_BV(6);
    else
        MPU_config = MPU_config | _BV(6);

    writeReg(MPU6050_PWR_MGMT_1, MPU_config);
    return MPU_config;

}

// --------------------------------------------------------
uint8_t MPU6050::cycleMode(uint8_t status)
{
    // status = 0, disable
    // status = 1, enable
    uint8_t MPU_config = 0;
    read(MPU6050_PWR_MGMT_1, &MPU_config, 1);

    if (status == 0)
        MPU_config = MPU_config & ~_BV(5);
    else
        MPU_config = MPU_config | _BV(5);

    writeReg(MPU6050_PWR_MGMT_1, MPU_config);
    return MPU_config;

}

// --------------------------------------------------------
// MPU6050_write
//
// This is a common function to write multiple bytes to an I2C device.
//
// If only a single register is written,
// use the function MPU_6050_write_reg().
//
// Parameters:
//   start : Start address, use a define for the register
//   pData : A pointer to the data to write.
//   size  : The number of bytes to write.
//
// If only a single register is written, a pointer
// to the data has to be used, and the size is
// a single byte:
//   int data = 0;        // the data to write
//   MPU6050_write (MPU6050_PWR_MGMT_1, &c, 1);
//
int16_t MPU6050::write(int start, const uint8_t *pData, int size)
{
  int n, error;

  Wire.beginTransmission(MPU6050_I2C_ADDRESS);
  n = Wire.write(start);        // write the start address
  if (n != 1)
    return (-20);

  n = Wire.write(pData, size);  // write data bytes
  if (n != size)
    return (-21);

  error = Wire.endTransmission(true); // release the I2C-bus
  if (error != 0)
    return (error);

  return (0);         // return : no error
}

// --------------------------------------------------------
// MPU6050_write_reg
//
// An extra function to write a single register.
// It is just a wrapper around the MPU_6050_write()
// function, and it is only a convenient function
// to make it easier to write a single register.
//
int16_t MPU6050::writeReg(int reg, uint8_t data)
{
  int error;

  error = write(reg, &data, 1);

  return (error);
}

// --------------------------------------------------------
// MPU6050_read
//
// This is a common function to read multiple bytes
// from an I2C device.
//
// It uses the boolean parameter for Wire.endTransMission()
// to be able to hold or release the I2C-bus.
// This is implemented in Arduino 1.0.1.
//
// Only this function is used to read.
// There is no function for a single byte.
//
int16_t MPU6050::read(int start, uint8_t *buffer, int size)
{
  int i, n;
  int error = 0;

  Wire.beginTransmission(MPU6050_I2C_ADDRESS);
  n = Wire.write(start);
  if (n != 1)
    return (-10);

  n = Wire.endTransmission(false);    // hold the I2C-bus
  if (n != 0)
    return (n);

  // Third parameter is true: relase I2C-bus after data is read.
  Wire.requestFrom(MPU6050_I2C_ADDRESS, size, true);
  i = 0;
  while(Wire.available() && i<size)
  {
    buffer[i++]=Wire.read();
  }
  if ( i != size)
    error = -11;

  return error;  // return : no error
}
