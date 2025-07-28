#include "Agora.h"


// GPIO for pairing button
#define PIN_PAIRING 2
int lastButtonState;


//-----------------------------------------------------------------------------------------------------------------------------

void myCallback(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    Serial.printf("Message from my guru: %s\n",incomingData);
}

//-----------------------------------------------------------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);

    // Join the Agora as an anonymous device
    Agora.begin();

    // Join a cult and register a function that is called when our guru sends a message
    // but do not pair automatically 
    Agora.join("Secret Cult",myCallback,false);
}

//-----------------------------------------------------------------------------------------------------------------------------

void loop()
{

    // Start pairing on press of button
    int buttonState = digitalRead(PIN_PAIRING);
    if (!buttonState && buttonState != lastButtonState)
    {
        Agora.conspire(); // look for a guru for default amount of time
        //Agora.conspire(20); // look for a guru for 20 seconds
        //Agora.conspire("Secret Cult") // look for a guru for a specific cult
    }
    lastButtonState = buttonState;

    delay(10);
}