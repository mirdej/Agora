#include "Agora.h"

//--------------------------------------------------------------------
// Callback function for data received for cult "MyCult"

void myCallback(const uint8_t *mac, const uint8_t *data, int len)
{
    AGORA_LOG_MAC(mac); // Utility function to print mac address
    Serial.printf(" sent us %u bytes: %s\n\n", len, data);
}

//--------------------------------------------------------------------
// Callback function for data received for cult "OtherCult"

void anOtherCallback(const uint8_t *mac, const uint8_t *data, int len)
{
}

//--------------------------------------------------------------------
// Simple Integer Callbacks do not differtiate between cults

void intCallback(int myInt)
{
    Serial.printf("Received an integer: %u\n", myInt);
}
//--------------------------------------------------------------------
void twoIntsReceived(int time, int number)
{
    Serial.printf("Master generated random number: %u\n", number);
    Serial.printf("Master time: %u vs my time:%u\n", time, millis());
}
//--------------------------------------------------------------------
void threeIntsReceived(int a, int b, int c)
{
    Serial.printf("Three ints: %u, %u, %u\n", a, b, c);
}





//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    // Join the Agora as an anonymous device
    Agora.begin();

    // Join a cult with default callback
    Agora.join("Mycult", myCallback);

    // Join another cult with another callback
    // Agora.join("OtherCult", anOtherCallback);

    // Add callback function for receiving single integers
    Agora.onInt(intCallback);
    Agora.onTwoInt(twoIntsReceived);
    Agora.onThreeInt(threeIntsReceived);

    delay(20000);

    Serial.println("\n\n--------------- Unregister callback 3 -----");
    Agora.onThreeInt(NULL); // unregister a callback
    // now the three ints get caught by the more general myCallback function (ints produce garbage output)
}

//------------------------------------------------------------------
void loop()
{
    // Nothing to do here, just wait
    delay(10000);
}