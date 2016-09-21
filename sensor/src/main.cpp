#include <Arduino.h>
#include <SPI.h>
#include <ArduinoHttpClient.h>
#include <WiFi101.h>
#include <Wire.h>
#include <MPU6050.h>
#include <QueueArray.h>
#include <network_conf.h>
#include <MPU6050_util.h>
#include <RTCZero.h>

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

    if(!QueueToAdd->push(mpu6050_data))
    {
        Serial.println("Queue is Full");
        QueueToAdd->pop();
        QueueToAdd->push(mpu6050_data);
    }
    MPU_counter++;
}

static int sendStatusCode;
static String response;
static int statusCode = 0;
RTCZero rtc;

void systemReset(void)
{

}

void MPU6050_HTTP_send(void)
{
    uint8_t send_counter;
    // Serial.print("Time: ");
    // Serial.print(MPU_HTTP_POST[3], HEX);
    // Serial.print(",");
    // Serial.print(MPU_HTTP_POST[4], HEX);
    // Serial.print(",");
    // Serial.print(MPU_HTTP_POST[5], HEX);
    // Serial.print(",");
    // Serial.print(MPU_HTTP_POST[6], HEX);
    // uint32_t ts = (uint32_t)MPU_HTTP_POST[3]<<24 | (uint32_t)MPU_HTTP_POST[4]<<16 | (uint32_t)MPU_HTTP_POST[5]<<8 | (uint32_t)MPU_HTTP_POST[6];
    // Serial.print(",");
    // Serial.println(ts, DEC);

    sendStatusCode = client.post(URL, contentType, buffer_size, &MPU_HTTP_POST[0]);
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

    //enable RTC
    rtc.begin();
    rtc.setTime(0, 0, 0);
    rtc.setDate(20, 9, 16);

    // get mac address
    WiFi.macAddress(mac_addr);
}

/********************************************/
void loop() {
    if(QueueToAdd->count() < 100)
        return;

    QueueArray<MPU6050_VALUE_TS>* temp;
    temp = QueueToPost;
    QueueToPost = QueueToAdd;
    QueueToAdd = temp;
    QueueToAdd->reset();

    if(QueueToPost->count() == 0)
        return;

    Serial.print("Queue Status: ");
    Serial.print(QueueToPost->count(), DEC);
    Serial.print(", ");
    Serial.println(MPU_counter, DEC);

    // create data string to send to ThingSpeak
    // String content = "N=" + String(QueueToPost->count(), DEC);
    // content += "&D=";
    MPU6050_VALUE_TS data_ts;
    MPU6050_HTTP_init();
    uint16_t Q_counter = 0;
    while (!QueueToPost->isEmpty())
    {
        data_ts = QueueToPost->pop();
        if(MPU6050_HTTP_post(data_ts) == 0)
        {
            // buffer is full, need to send data

            MPU6050_HTTP_finalize(Q_counter, WiFi.RSSI(), mac_addr);

            Serial.print("Send: ");
            Serial.print(Q_counter,DEC);
            Serial.print(", ");
            Serial.println(buffer_size, DEC);

            // send_code = client.post(URL, contentType, buffer_size, &MPU_HTTP_POST[0]);
            // Serial.print("Send Status code: ");
            // Serial.println(send_code);
            // statusCode = client.responseStatusCode();
            //
            // for(send_counter = 0; (send_counter < 3) && (send_code < 0 || statusCode != 200); send_counter++)
            // {
            //     send_code = checkWiFiClient(send_code, buffer_size, &MPU_HTTP_POST[0]);
            //     Serial.print("Send Status code: ");
            //     Serial.print(send_code);
            //     Serial.print(", ");
            //     Serial.println(send_counter, DEC);
            //
            //     statusCode = client.responseStatusCode();
            // }

            MPU6050_HTTP_send();
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
        Serial.print("Send: ");
        Serial.print(Q_counter,DEC);
        Serial.print(", ");
        Serial.println(buffer_size, DEC);
        MPU6050_HTTP_finalize(Q_counter, WiFi.RSSI(), mac_addr);
        // Serial.print("Buffer Counter: ");
        // Serial.print(buffer_size, DEC);
        // Serial.print(", Q counter: ");
        // Serial.println(Q_counter, DEC);

        // Serial.print("Size: ");
        // Serial.println(sizeof(MPU6050_VALUE_TS), DEC);

        // send_code = client.post(URL, contentType, buffer_size, &MPU_HTTP_POST[0]);
        // Serial.print("Send Status code: ");
        // Serial.println(send_code);
        // statusCode = client.responseStatusCode();
        //
        // for(send_counter = 0; (send_counter < 3) && (send_code < 0 || statusCode != 200); send_counter++)
        // {
        //     send_code = checkWiFiClient(send_code, buffer_size, &MPU_HTTP_POST[0]);
        //     Serial.print("Send Status code: ");
        //     Serial.print(send_code);
        //     Serial.print(", ");
        //     Serial.println(send_counter, DEC);
        //
        //     statusCode = client.responseStatusCode();
        // }

        MPU6050_HTTP_send();

        Serial.print("Status code: ");
        Serial.println(statusCode);

    }

    delay(200);
}
