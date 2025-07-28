#include "Agora.h"

//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(3000);
    Serial.println("Agora Example 01 - Simple Receiver");

    // Join the Agora as an anonymous device
    Agora.begin();

    // Join a cult
    Agora.join("Mycult");

    // Add callback directly as a lamda function, note the collection of different brackets: [](int myInt){}
    Agora.onInt([](int myInt)
                { Serial.printf("Received an integer: %u\n", myInt); });
}

//------------------------------------------------------------------
void loop()
{
    // Nothing to do here, just wait
    delay(10000);
    Serial.println("Agora Example 01 - Simple Receiver (Lamda)");
}