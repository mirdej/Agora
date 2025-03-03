#include "Agora.h"

void callback(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    Serial.printf("Message from a Member: %s\n",incomingData);
}

void setup()
{
    Serial.begin(115200);

    // Join the Agora as an anonymous device
    Agora.begin();

    // Establish a named cult and register its callback function
    Agora.establish("Mycult", callback);
}

void loop()
{
    //Send a message to all followers (if there are any)
    Agora.tell("It's so easy");
    delay(2000);
}