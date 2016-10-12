#include <Arduino.h>
#include <SPI.h>
#include <ArduinoHttpClient.h>
#include <WiFi101.h>
#include <Wire.h>
#include <MPU6050.h>
#include <QueueArray.h>
#include <network_conf.h>
#include <MPU6050_util.h>

//#define DEBUG

// Initialize the Wifi client library
const int led = LED_BUILTIN;  // the pin with a LED
int ledState = LOW;

void blinkLED(void)
{
  if (ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }
  digitalWrite(led, ledState);
}

// Initializ the MPU6050
#define MAX_QUEUE_SIZE 500
MPU6050 mpu6050;
QueueArray <MPU6050_VALUE_TS> QueueOne(MAX_QUEUE_SIZE);
QueueArray <MPU6050_VALUE_TS> QueueTwo(MAX_QUEUE_SIZE);
QueueArray <MPU6050_VALUE_TS> *QueueToAdd;
QueueArray <MPU6050_VALUE_TS> *QueueToPost;
uint16_t MPU_counter = 0;
byte mac_addr[6];                     // the MAC address of your Wifi shield

void getMPUdata(void)
{
    mpu6050.sample();
    if (0 == mpu6050.sampleSum())
    {
        MPU6050_init(&mpu6050);
        mpu6050.sample();
        Serial.println("Reset MPU");
    }

    MPU6050_VALUE_TS mpu6050_data;
    mpu6050.getTs(&mpu6050_data);

    // update AR related variables
    float data_var[6];
    data_var[0] = (float)mpu6050_data.value.accel_x;
    data_var[1] = (float)mpu6050_data.value.accel_y;
    data_var[2] = (float)mpu6050_data.value.accel_z;
    data_var[3] = (float)mpu6050_data.value.gyro_x;
    data_var[4] = (float)mpu6050_data.value.gyro_y;
    data_var[5] = (float)mpu6050_data.value.gyro_z;

    uint8_t i;
    for(i = 0; i < 6; i++)
    {
        X[i] += data_var[i];
        X2[i] += data_var[i] * data_var[i];
    }


    if(!QueueToAdd->push(mpu6050_data))
    {
        Serial.println("Queue is Full");

        MPU6050_VALUE_TS data_pop = QueueToAdd->pop();
        data_var[0] = (float)data_pop.value.accel_x;
        data_var[1] = (float)data_pop.value.accel_y;
        data_var[2] = (float)data_pop.value.accel_z;
        data_var[3] = (float)data_pop.value.gyro_x;
        data_var[4] = (float)data_pop.value.gyro_y;
        data_var[5] = (float)data_pop.value.gyro_z;

        for(i = 0; i < 6; i++)
        {
            X[i] -= data_var[i];
            X2[i] -= data_var[i] * data_var[i];
        }

        QueueToAdd->push(mpu6050_data);
    }

    MPU_counter++;
}

static int sendStatusCode;
static String response;
static int statusCode = 0;

void MPU6050_HTTP_send(const char *url)
{
    uint8_t send_counter;

    Serial.print("Send: ");
    Serial.print(buffer_size);
    Serial.print(" Bytes");

    sendStatusCode = client.post(url, contentType, buffer_size, &MPU_HTTP_POST[0]);
    Serial.print("Send Status code: ");
    Serial.println(sendStatusCode);

    if(sendStatusCode != HTTP_ERROR_SOCKET_FAILED)
    {
        statusCode = client.responseStatusCode();
        response = client.responseBody();
        if(statusCode != 200)
        {
            Serial.println("===Incorrect Status===");
            Serial.print(statusCode,DEC);
            Serial.println(";\n" + response);
        }
    }

    for(send_counter = 0; (send_counter < 10) && (sendStatusCode < 0 || statusCode != 200); send_counter++)
    {
        // Serial.println("===Start Resend===");
        checkWiFiClient(sendStatusCode);
        sendStatusCode = client.post(URL, contentType, buffer_size, &MPU_HTTP_POST[0]);
        Serial.print("ReSend Status code: ");
        Serial.print(sendStatusCode, DEC);
        Serial.print(", ");
        Serial.println(send_counter, DEC);

        statusCode = client.responseStatusCode();
        response = client.responseBody();
        if(statusCode != 200)
        {
            Serial.print("Status code: ");
            Serial.print(statusCode,DEC);
            Serial.println(";\n" + response);
        }
    }
    blinkLED();
}

/********************************************/
void setup() {
    Serial.begin(9600);
    // attempt to connect to Wifi network
    connectWiFi();

    pinMode(led, OUTPUT);

    // enable sensor
    Wire.begin();
    MPU6050_init(&mpu6050);
    pinMode(A1, INPUT);
    attachInterrupt(A1, getMPUdata, HIGH);

    //assign queue pointer
    QueueOne.setStream(Serial);
    QueueTwo.setStream(Serial);
    QueueToAdd = &QueueOne;
    QueueToPost = &QueueTwo;

    // get mac address
    WiFi.macAddress(mac_addr);
    mac_addr_str = String("&&&M=") + String(mac_addr[1], HEX)+String(mac_addr[0], HEX);
}

/********************************************/
void loop() {
    if(QueueToAdd->count() < 300)
        return;

    QueueArray<MPU6050_VALUE_TS>* temp;
    temp = QueueToPost;
    QueueToPost = QueueToAdd;
    QueueToAdd = temp;
    QueueToAdd->reset();

    // compute AR mean & variance
    float AR_mean[6], AR_std[6], temp_AR;
    uint16_t i,j;
    for(i = 0; i < 6; i++)
    {
        temp_AR = X[i]/300.0;
        AR_mean[i] = temp_AR;
        AR_std[i] = sqrtf(X2[i]/300.0 - temp_AR*temp_AR);
    }

    //reset AR variables
    memset(X,0,6);
    memset(X2,0,6);

    // if(QueueToPost->count() == 0)
    //     return;

#ifdef DEBUG
    Serial.print("Queue Status: ");
    Serial.print(QueueToPost->count(), DEC);
    Serial.print(", ");
    Serial.println(MPU_counter, DEC);
#endif

    // create data string to send to ThingSpeak
    MPU6050_VALUE_TS data_ts;
    MPU6050_HTTP_init();
    uint16_t Q_counter = 0;
    float y[6];
    float X_mat[6][AR_ORDER];
    uint16_t SGD_counter = 0;
    float error = 0;
    float AR_coeff[6][AR_ORDER] = {0};

    while(!QueueToPost->isEmpty())
    {
        data_ts = QueueToPost->pop();
        // data_ts = QueueToAdd->pop();

        // AR model -- normalize
        y[0] = ((float)data_ts.value.accel_x-AR_mean[0])/AR_std[0];
        y[1] = ((float)data_ts.value.accel_y-AR_mean[1])/AR_std[1];
        y[2] = ((float)data_ts.value.accel_z-AR_mean[2])/AR_std[2];
        y[3] = ((float)data_ts.value.gyro_x-AR_mean[3])/AR_std[3];
        y[4] = ((float)data_ts.value.gyro_y-AR_mean[4])/AR_std[4];
        y[5] = ((float)data_ts.value.gyro_z-AR_mean[5])/AR_std[5];

        if(SGD_counter < AR_ORDER)
        {
            for(i = 0; i < 6; i++)
            {
                X_mat[i][SGD_counter] = y[i];
            }
        }
        else
        {
            for(i = 0; i < 6; i++) // over variable
            {
                error = 0;
                for(j=0; j< AR_ORDER; j++) // over data
                {
                    error += X_mat[i][j] * AR_coeff[i][j];
                }
                error = error - y[i];

                // learning rate
                error = error * MU / SGD_counter;

                // update coefficient & shift data
                for(j=0; j< (AR_ORDER-1); j++)
                {
                    AR_coeff[i][j] = AR_coeff[i][j] - error * X_mat[i][j];
                    X_mat[i][j] = X_mat[i][j+1];
                }
                AR_coeff[i][AR_ORDER-1] = AR_coeff[i][AR_ORDER-1] - error * X_mat[i][AR_ORDER-1];
                X_mat[i][AR_ORDER-1] = y[i];
            }
        }
        SGD_counter++;


        // push data
        if(MPU6050_HTTP_post(data_ts) == 0)
        {
            // buffer is full, need to send data
            MPU6050_HTTP_finalize(Q_counter, WiFi.RSSI());

#ifdef DEBUG
            Serial.print("Send: ");
            Serial.print(Q_counter,DEC);
            Serial.print(", ");
            Serial.println(buffer_size, DEC);
#endif

            MPU6050_HTTP_send(URL);
            delay(100);
            Serial.print("Status code: ");
            Serial.println(statusCode);

            // reset buffer_counter
            MPU6050_HTTP_init();
            MPU6050_HTTP_post(data_ts);
            Q_counter = 1;
        }
        else
        {
            Q_counter++;
        }
    }

    if(Q_counter > 0)
    {
#ifdef DEBUG
        Serial.print("Send: ");
        Serial.print(Q_counter,DEC);
        Serial.print(", ");
        Serial.println(buffer_size, DEC);
#endif
        MPU6050_HTTP_finalize(Q_counter, WiFi.RSSI());
        MPU6050_HTTP_send(URL);
        delay(100);

        Serial.print("Status code: ");
        Serial.println(statusCode);
    }

    // send AR coefficient
    memset(&MPU_HTTP_POST[0],0,MAX_MPU6050_BUFFER_SIZE);
    buffer_size = 0;
    MPU_HTTP_POST[buffer_size++] = _BYTE('A');
    MPU_HTTP_POST[buffer_size++] = _BYTE('=');
    memcpy(&MPU_HTTP_POST[buffer_size],(byte*)&AR_coeff[0][0],AR_DATA_SIZE);
    buffer_size += AR_DATA_SIZE;

    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('&');
    MPU_HTTP_POST[buffer_size++] = _BYTE('I');
    MPU_HTTP_POST[buffer_size++] = _BYTE('=');
    String TX_counter_str = String(httpTXCounter, DEC);
    memcpy(&MPU_HTTP_POST[buffer_size], (byte*)TX_counter_str.c_str(), TX_counter_str.length());
    buffer_size += TX_counter_str.length();
    httpTXCounter++;

    // add mac address
    // only need last two bytes of mac address
    memcpy(&MPU_HTTP_POST[buffer_size], mac_addr_str.c_str(), mac_addr_str.length());
    buffer_size += mac_addr_str.length();

#ifdef DEBUG
        Serial.print("AR coeff: ");
        Serial.print(AR_coeff[0][0]);
        Serial.print(", ");
        Serial.print(AR_coeff[0][1]);
        Serial.print(", ");
        Serial.println(AR_coeff[1][0]);
#endif

        MPU6050_HTTP_send(AR_URL);
        //delay(100);

        Serial.print("Status code: ");
        Serial.println(statusCode);
}
