#include "Agora.h"

//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    delay(3000);
    Serial.println("Agora Wifi Provisioning Example");
    Serial.println("Slave waiting to be guided to the Internet");

    // Join the Agora as an anonymous device
    Agora.begin();

    // Join a cult
    Agora.join("internets");
}

//------------------------------------------------------------------
void loop()
{

    if (WiFi.isConnected())
    {
        Serial.printf("Connected to Wifi: %s\n", WiFi.SSID());
    }
    else
    {
        Serial.printf("I'm not connected to Wifi\n");
    }
    delay(10000);
}