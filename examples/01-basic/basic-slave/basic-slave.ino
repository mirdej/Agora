#include "Agora.h"

void myCallback(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    Serial.printf("Message from a Member: %s\n",incomingData);
}

void setup()
{
    Serial.begin(115200);

    // Join the Agora as an anonymous device
    Agora.begin();

    // Join a cult and register a function that is called when our guru sends a message
    Agora.join("Mycult",myCallback);
}

void loop()
{
    //Nothing to do here, just wait
    delay(10000);
}