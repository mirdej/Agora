#include "Agora.h"

//------------------------------------------------------------------
void myCallback(int myInt)
{
    Serial.printf("Received an integer: %u\n",myInt);
}


//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    // Join the Agora as an anonymous device
    Agora.begin();

    // Join a cult
    Agora.join("Mycult");

    // Add callback function for receiving single integers
    Agora.onInt(myCallback);
}


//------------------------------------------------------------------
void loop()
{
    //Nothing to do here, just wait
    delay(10000);
}