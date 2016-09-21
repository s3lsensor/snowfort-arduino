#include <MPU6050.h>

static uint16_t httpTXCounter = 0;

void MPU6050_init(MPU6050* mpu6050)
{
    mpu6050->enable();

    mpu6050->sleepMode(0);
    mpu6050->cycleMode(0);

    // set acc range: +/- 2g
    mpu6050->setAccRange(0);

    // set gyro range: +/- 250 o/s
    mpu6050->setGyroRange(0);

    uint8_t MPU_config = 0;

    // set clock source -- X axis gyro reference
    // mpu6050.read(MPU6050_PWR_MGMT_1, &MPU_config, 1);
    // MPU_config = MPU_config | _BV(1);
    // mpu6050.write_reg(MPU6050_PWR_MGMT_1 ,MPU_config);


    // set digital LPF -- BW = 21Hz for acc, BW = 20Hz for gyro
    mpu6050->read(MPU6050_CONFIG, &MPU_config, 1);
    MPU_config = MPU_config | _BV(2);
    mpu6050->writeReg(MPU6050_CONFIG, MPU_config);

    // set sampling rate -- DLPF_CFG is enabled
    // sampling rate 1kHz (internal)
    const uint8_t SMPLRT_DIV = 0x13; // 19 DEC -- 50Hz
    // const uint8_t SMPLRT_DIV = 0x27; // 39 DEC -- 25Hz
    // const uint8_t SMPLRT_DIV = 0x63; // 99 DEC -- 10Hz
    mpu6050->writeReg(MPU6050_SMPLRT_DIV, SMPLRT_DIV);

    // enable interrupt -- 50Hz
    mpu6050->writeReg(0x38, _BV(0));

    //
    // // disable DFPG_CFG
    // mpu6050.writeReg(MPU6050_CONFIG, 0);
    //
    // // set sampling rate -- DLPF_CFG 0s disabled
    // const uint8_t SMPLRT_DIV = 0x9F; // 159 DEC
    // mpu6050.writeReg(MPU6050_SMPLRT_DIV, SMPLRT_DIV); //50Hz
    //
    // // enable interrupt -- 50Hz
    // mpu6050.writeReg(0x38, _BV(0));
}

// this is the maximum size of WiFi101 socket
// defined in WiFI101/socket.h
#define MAX_MPU6050_BUFFER_SIZE 1400

// reserve several bytes for meta info
#define MAX_MPU6050_BUFFER_SIZE_USEABLE 1350
#define _BYTE(V) (byte)(V)
static byte MPU_HTTP_buffer_1[MAX_MPU6050_BUFFER_SIZE], MPU_HTTP_buffer_2[MAX_MPU6050_BUFFER_SIZE];
static byte* MPU_HTTP_ADD = MPU_HTTP_buffer_1;
static byte* MPU_HTTP_POST = MPU_HTTP_buffer_2;
static uint16_t buffer_counter = 0, buffer_size = 0;
void MPU6050_HTTP_init(void)
{
    memset(MPU_HTTP_ADD,0,MAX_MPU6050_BUFFER_SIZE);
    MPU_HTTP_ADD[0] = _BYTE('D');
    MPU_HTTP_ADD[1] = _BYTE('=');
    buffer_counter = 2;
}

uint16_t MPU6050_HTTP_post(MPU6050_VALUE_TS data)
{
    // if ((buffer_counter+MPU6050_VALUE_TS_SIZE) <= MAX_MPU6050_BUFFER_SIZE_USEABLE)
    if ((buffer_counter+18) <= MAX_MPU6050_BUFFER_SIZE_USEABLE)
    {
        // memcpy(&MPU_HTTP_ADD[buffer_counter], (byte*)&data, MPU6050_VALUE_TS_SIZE);
        // buffer_counter += MPU6050_VALUE_TS_SIZE;

        // MPU6050_VALUE_TS_SIZE is 20 because of alignment. To save bandwidth for communication,
        // we use 18 bytes here, which is the actual structure size.
        memcpy(&MPU_HTTP_ADD[buffer_counter], (byte*)&data, 18);
        buffer_counter += 18;
        // uint32_t ts_time = data.ms_time;
        // MPU_HTTP_ADD[buffer_counter++] = (uint8_t)(ts_time>>24);
        // MPU_HTTP_ADD[buffer_counter++] = (uint8_t)(ts_time>>16);
        // MPU_HTTP_ADD[buffer_counter++] = (uint8_t)(ts_time>>8);
        // MPU_HTTP_ADD[buffer_counter++] = (uint8_t)(ts_time);
        //
        // // memcpy(&MPU_HTTP_ADD[buffer_counter], (byte*)&data.ms_time, 4);
        // // buffer_counter += 4;
        // memcpy(&MPU_HTTP_ADD[buffer_counter], (byte*)&data.value, 14);
        // buffer_counter += 14;

        MPU_HTTP_ADD[buffer_counter++] = _BYTE('|');
        MPU_HTTP_ADD[buffer_counter++] = _BYTE('|');
        return buffer_counter;
    }
    else
        return 0;
}

uint16_t MPU6050_HTTP_finalize(uint16_t Qcounter, int16_t RSSI, byte* mac_addr)
{
    // exchange buffer
    buffer_size = buffer_counter;
    buffer_counter = 0;
    byte * tmp_ptr;
    tmp_ptr = MPU_HTTP_ADD;
    MPU_HTTP_ADD = MPU_HTTP_POST;
    MPU_HTTP_POST = tmp_ptr;

    MPU6050_HTTP_init();

    // append finalize data
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('N');
    MPU_HTTP_POST[buffer_size++] = _BYTE('=');
    //memcpy(&MPU_HTTP_buffer[buffer_counter], (byte*)&Qcounter, sizeof(uint16_t));
    //buffer_counter += sizeof(uint16_t);
    String Qstr = String(Qcounter, DEC);
    memcpy(&MPU_HTTP_POST[buffer_size], (byte*)Qstr.c_str(), Qstr.length());
    buffer_size += Qstr.length();

    // add RSSI
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('R');
    MPU_HTTP_POST[buffer_size++] = _BYTE('=');
    String RSSI_str = String(RSSI, DEC);
    memcpy(&MPU_HTTP_POST[buffer_size], (byte*)RSSI_str.c_str(), RSSI_str.length());
    buffer_size += RSSI_str.length();

    // add packet counter -- trace packet loss
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('I');
    MPU_HTTP_POST[buffer_size++] = _BYTE('=');
    String TX_counter_str = String(httpTXCounter, DEC);
    memcpy(&MPU_HTTP_POST[buffer_size], (byte*)TX_counter_str.c_str(), TX_counter_str.length());
    buffer_size += TX_counter_str.length();
    httpTXCounter++;

    // add packet counter -- trace packet loss
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('M');
    MPU_HTTP_POST[buffer_size++] = _BYTE('=');
    // only need last two bytes of mac address
    String mac_addr_str = String(mac_addr[1], HEX)+String(mac_addr[0], HEX);
    memcpy(&MPU_HTTP_POST[buffer_size], mac_addr_str.c_str(), mac_addr_str.length());
    buffer_size += mac_addr_str.length();

    return buffer_size;
}
