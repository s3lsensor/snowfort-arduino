#include <WiFi101.h>
#include <ArduinoHttpClient.h>

const char ssid[] = "snowfort"; //  your network SSID (name)
const char pass[] = "stanfordS3L"; // your network password
// char ssid[] = "挖掘技术哪家强"; //  your network SSID (name)
// char pass[] = "13772069150"; // your network password

const char serverAddress[] = "192.168.50.3";
// char serverAddress[] = "192.168.1.100";
const int serverPort = 5000;
// char contentType[] = "application/x-www-form-urlencoded";
const char contentType[] = "application/octet-stream";
const char URL[] = "/data";
const char AR_URL[] = "/AR";

int WiFi_status = WL_IDLE_STATUS;
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, serverPort);

void connectWiFi(void)
{
    uint8_t counter;
    for( counter = 0; (counter < 20) && (WiFi_status != WL_CONNECTED); counter++)
    {
        // Connect to WPA/WPA2 Wi-Fi network
        WiFi_status = WiFi.begin(ssid, pass);

        // wait 1 seconds for connection
        delay(1000);
        Serial.print("Connect WiFi -- ");
        Serial.println(counter, DEC);
    }
    //WiFi.maxLowPowerMode();

    // sleep mode
    if(counter == 19)
    {
        Serial.println("Into WiFi Sleep Mode");
        while(WiFi_status != WL_CONNECTED)
        {
            // WiFi.maxLowPowerMode();
            Serial.println("Sleep model");
            delay(30*1000);

            // Connect to WPA/WPA2 Wi-Fi network
            // WiFi.noLowPowerMode();
            WiFi_status = WiFi.begin(ssid, pass);
        }
        Serial.println("Leave WiFi Sleep Mode");

    }
}

void checkWiFi(void)
{
    WiFi_status = WiFi.status();
    connectWiFi();
}

void checkWiFiClient(int8_t sendCode)
{
    client.stop();
    if(sendCode == HTTP_ERROR_API || sendCode == HTTP_ERROR_SOCKET_FAILED)
    {
        client = HttpClient(wifi, serverAddress, serverPort);
    }

    if(sendCode == HTTP_ERROR_CONNECTION_FAILED || sendCode == HTTP_ERROR_TIMED_OUT || sendCode == HTTP_ERROR_INVALID_RESPONSE)
    {
        checkWiFi();
        client = HttpClient(wifi, serverAddress, serverPort);
    }
}
