#include "Agora.h"

void setup()
{
    Serial.begin(115200);

    // Join the Agora as an anonymous device
    Agora.begin();

    // Establish a cult
    Agora.establish("Mycult");
}

void loop()
{
    //Send a message to all followers (if there are any)
    Agora.tell("Hello my dear followers, that's easy !");
    delay(2000);
}