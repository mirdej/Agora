#include "Agora.h"
#include "WiFi.h"
#include <esp_wifi.h>

#define PIN_BTN 3

int lastButton;

//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    delay(3000);
    Serial.println("Agora Wifi Provisioning Example");

    Serial.println("Connecting to WiFi");
    WiFi.begin("Your SSID", "Your PASS");
    while (!WiFi.isConnected())
    {
        delay(100);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Join the Agora
    Agora.begin("master of the");

    // Establish a cult
    Agora.establish("internets");

    pinMode(PIN_BTN, INPUT_PULLUP);
    lastButton = digitalRead(PIN_BTN);
}

//------------------------------------------------------------------
void loop()
{

    int button = digitalRead(PIN_BTN);
    if (button != lastButton){
        lastButton = button;
        Serial.println("Button Pressed");
        
        // Make members connect to the same WiFi
        Agora.openTheGate();
    }
    // wait a bit
    delay(100);
}