#include <Arduino.h>
#include <SPI.h>
#include <ArduinoHttpClient.h>
#include <WiFi101.h>
#include <Wire.h>
#include <MPU6050.h>
#include <QueueArray.h>
#include <network_conf.h>
#include <MPU6050_util.h>

// Initialize the Wifi client library

String response;
int statusCode = 0;
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
static MPU6050 mpu6050;
static QueueArray <MPU6050_VALUE_TS> QueueOne(500);
static QueueArray <MPU6050_VALUE_TS> QueueTwo(500);
static QueueArray <MPU6050_VALUE_TS> *QueueToAdd;
static QueueArray <MPU6050_VALUE_TS> *QueueToPost;
static uint16_t counter = 0;
static MPU6050_VALUE_TS mpu6050_data;

void getMPUdata(void)
{
    mpu6050.sample();
    if (0 == mpu6050.sampleSum())
    {
        MPU6050_init(&mpu6050);
        mpu6050.sample();
    }
    mpu6050_data = mpu6050.getTs();
    if(!QueueToAdd->push(mpu6050_data))
    {
        QueueToAdd->pop();
        QueueToAdd->push(mpu6050_data);
    }
    counter++;
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
}

/********************************************/
void loop() {
    QueueArray<MPU6050_VALUE_TS>* temp;
    temp = QueueToPost;
    QueueToPost = QueueToAdd;
    QueueToAdd = temp;
    QueueToAdd->reset();

    if(QueueToPost->count() == 0)
        return;

    Serial.print(QueueToPost->count());
    Serial.print(", ");
    Serial.println(counter);

    // create data string to send to ThingSpeak
    // String content = "N=" + String(QueueToPost->count(), DEC);
    // content += "&D=";
    MPU6050_VALUE_TS data_ts;
    // MPU6050_HTTP_init();
    uint16_t Q_counter = 0;
    int8_t send_code;
    String content = "N=" + String(QueueToPost->count(), DEC) + "&D=";
    while (!QueueToPost->isEmpty())
    {
        data_ts = QueueToPost->pop();
        content += mpu6050.getString(data_ts) + "|";
        if(MPU6050_HTTP_post(data_ts) == 0)
        {
            // buffer is full, need to send data
            MPU6050_HTTP_finalize(Q_counter, WiFi.RSSI());

            Serial.print("Full: ");
            Serial.print(Q_counter,DEC);
            Serial.print(", ");
            Serial.println(buffer_size, DEC);

            send_code = client.post(URL, contentType, buffer_size, &MPU_HTTP_POST[0]);
            Serial.print("Send Status code: ");
            Serial.println(send_code);

            statusCode = client.responseStatusCode();
            Serial.print("Status code: ");
            Serial.println(statusCode);

            response = client.responseBody();
            if(statusCode != 200)
            {
                Serial.println(response);
            }

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
        MPU6050_HTTP_finalize(Q_counter, WiFi.RSSI());
        Serial.print("Buffer Counter: ");
        Serial.print(buffer_size, DEC);
        Serial.print(", Q counter: ");
        Serial.println(Q_counter, DEC);

        MPU6050_VALUE_TS temp_ts = ((MPU6050_VALUE_TS *)MPU_HTTP_POST)[0];
        mpu6050.disp(temp_ts);

        // Serial.print("Size: ");
        // Serial.println(sizeof(MPU6050_VALUE_TS), DEC);

        send_code = client.post(URL, contentType, buffer_size, &MPU_HTTP_POST[0]);
        Serial.print("Send Status code: ");
        Serial.println(send_code);

        statusCode = client.responseStatusCode();
        Serial.print("Status code: ");
        Serial.println(statusCode);

        response = client.responseBody();
        if(statusCode != 200)
        {
            Serial.println(response);
        }
    }

    delay(100);
}
