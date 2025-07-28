#include "Agora.h"

int counter;

//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    delay(3000);
    Serial.println("Agora Example 01 - Simple Sender");

    // Join the Agora as an anonymous device
    Agora.begin();

    // Establish a cult
    Agora.establish("Mycult");
}

//------------------------------------------------------------------
void loop()
{
    // increment counter
    counter++;

    // Send an int to all followers (if there are any)
    Agora.tell(counter);

    // wait a bit
    delay(2000);
}