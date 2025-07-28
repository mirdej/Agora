#include "Agora.h"

/*
    The Agora Library provides easy means to send simple messages from a "guru" (master) to all "members (slaves) 
    or from a member back to the guru.

    The tell() function accepts up to three integers (unsigned longs, actually), 
    and on the receiver side you can register callback functions to handle these.

    see next example "Structs" if you need to send floats or more elaborate data

*/

int counter;

//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    delay(3000);
    Serial.println("Agora Example 2 : Message Types");
    Serial.println("(see slave for callback functions)");

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

    // Send an two numbers at once
    Agora.tell(millis(), random());

    // Send an three numbers at once. Wireless MIDI, anyone?
    Agora.tell(millis(), random(),counter);

    // Send a message (up to 249 chars)
    Agora.tell("Hi there");



    // wait a bit
    delay(2000);

}