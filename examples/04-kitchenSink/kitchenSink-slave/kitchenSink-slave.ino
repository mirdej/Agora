/*========================================================================================

AGORA. ESP-NOW Utility Library for ESP32

© 2024-2025 Michael Egger AT anyma.ch
https://github.com/mirdej/Agora

==========================================================================================*/

#include "Agora.h"
#include "SPIFFS.h"

// GPIO for pairing button
#define PIN_PAIRING 2
int lastButtonState;

long lastMessage;

typedef struct
{
    float aFloat;
    unsigned long timestamp;
} secret_message_t;

//-----------------------------------------------------------------------------------------------------------------------------
// Callback Functions
//-----------------------------------------------------------------------------------------------------------------------------
void fileshareCallback(const uint8_t *mac, ftpStatus_t status, const char *filename, size_t filesize, size_t bytesRemaining)
{
    int percent = 100 - (100 * bytesRemaining / filesize);

    if (status == RUNNING)
    {
        Serial.printf("Receive %s ", filename);
        Serial.printf("to %02x:%02x:%02x:%02x:%02x:%02x : ",
                      mac[0], mac[1], mac[2],
                      mac[3], mac[4], mac[5]);
        Serial.printf(" %u% done\n", percent);
    }

    else if (status == DONE)
    {
        Serial.println("File successfully transferred.\n\n\n");
    }

    else if (status == ERROR)
    {
        Serial.printf("\n\nAn ERROR occurred while transferring file %s !\n\n\n", filename);
    }
}
//-----------------------------------------------------------------------------------------------------------------------------
void popCultCallback(const uint8_t *mac, const uint8_t *incomingData, int len)
{
}

//-----------------------------------------------------------------------------------------------------------------------------
void secretCultCallback(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    secret_message_t message;
    if (len == sizeof(message))
    {
        memcpy((uint8_t *)&message, incomingData, len); // copy received bytes to the memory address where my struct lives
        // now message has data and we can access its fields
        Serial.printf("%f was seen at timestamp %u", message.aFloat, message.timestamp);
    }
}

//-----------------------------------------------------------------------------------------------------------------------------
// SETUP
//-----------------------------------------------------------------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    pinMode(PIN_PAIRING, INPUT_PULLUP);
    lastButtonState = digitalRead(PIN_PAIRING);

    // Start SPIFFS File System
    SPIFFS.begin();

    // Join the Agora with a distinct name
    //  second argument = true:
    Agora.begin("Kitchen Sink Slave", true);

    Agora.join("Secret Cult", secretCultCallback, false); // Establish a cult but don't automatically accept members
    Agora.join("Popular Cult", popCultCallback);          // Establish another cult open to anyone, register callback function

    //Agora.forgetFriends(); // Forget all gurus and all memebrs this device has paired with in the past

    Agora.enableFileSharing(SPIFFS, fileshareCallback);
    Agora.eavesdrop = true; // Print all messages to the serial monitor

    Agora.setPingInterval(1000);  

    Serial.printf("Agora Version: %s\n", Agora.getVersion());
}

//-----------------------------------------------------------------------------------------------------------------------------

void loop()
{
    AGORA_LOG_STATUS(30000); // log status to serial monitor every 30 seconds


    // Start pairing on press of button
    int buttonState = digitalRead(PIN_PAIRING);
    if (!buttonState && buttonState != lastButtonState)
    {
        Agora.conspire(40,"Secret Cult");
    }
    lastButtonState = buttonState;

    delay(20);
}