#include "Agora.h"

// GPIO for pairing button
#define PIN_PAIRING 2
int lastButtonState;

long lastMessage;

//-----------------------------------------------------------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_PAIRING, INPUT_PULLUP);
    lastButtonState = digitalRead(PIN_PAIRING);

    // Join the Agora with a distinct name
    //  second argument = true:
    Agora.begin("Picky Master", true);

    // Establish a cult but don't automatically accept members
    Agora.establish("Secret Cult", false);
}

//-----------------------------------------------------------------------------------------------------------------------------

void loop()
{
    // Send a message to all followers (if there are any)
    if (millis() - lastMessage > 3000)
    {
        Agora.tell("Hello my dear followers, that's easy !");
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