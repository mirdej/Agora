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
        Serial.printf("Send %s ", filename);
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
    Agora.begin("Kitchen Sink Master", true);

    Agora.establish("Secret Cult", secretCultCallback, false); // Establish a cult but don't automatically accept members
    Agora.establish("Popular Cult", popCultCallback);          // Establish another cult open to anyone, register callback function

    Agora.forgetFriends(); // Forget all gurus and all memebrs this device has paired with in the past

    Agora.enableFileSharing(SPIFFS, fileshareCallback);
    Agora.eavesdrop = true; // Print all messages to the serial monitor

    Agora.setPingInterval(1000);

    Serial.printf("Agora Version: %s\n", Agora.getVersion());
}

//-----------------------------------------------------------------------------------------------------------------------------

void loop()
{
    AGORA_LOG_STATUS(30000); // log status to serial monitor every 30 seconds

    // Send a message to all followers (if there are any)
    if (millis() - lastMessage > 3000)
    {
        secret_message_t message;
        message.aFloat = (float)random() / 65536.;
        message.timestamp = millis();
        Agora.tell("Secret Cult", "Psst, don't share this secret!");
        lastMessage = millis();
    }

    // Start pairing on press of button
    int buttonState = digitalRead(PIN_PAIRING);
    if (!buttonState && buttonState != lastButtonState)
    {
        Agora.conspire(); // allow new members to join for default amount of time
        // Agora.conspire(20); // allow new members to join for 20 seconds
        // Agora.conspire("Secret Cult") // allow new members to join specific cult
    }
    lastButtonState = buttonState;

    delay(20);
}