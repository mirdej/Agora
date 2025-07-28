#include "Agora.h"

//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    // Join the Agora as an anonymous device
    Agora.forgetFriends();
    Agora.begin();

    // Same example as before but callbacks are defined in line as lamda functions
    // Note the different signatures

    Agora.join("Mycult", [](const uint8_t *mac, const uint8_t *data, int len)
               {
                    AGORA_LOG_MAC(mac); // Utility function to print mac address
                    Serial.printf(" sent us %u bytes: %s\n\n", len, data); 
                });

    Agora.onInt([](int a)
                { Serial.printf("Received an integer: %u\n", a); });

    Agora.onTwoInt([](int time, int number)
                   { 
                    Serial.printf("Master generated random number: %u\n", number);
                    Serial.printf("Master time (%u) minus my time (%u) gives %d\n", time, millis(),time-millis()); });

    Agora.onThreeInt([](int a, int b, int c)
                     { Serial.printf("Three ints: %u, %u, %u\n", a, b, c); });
}

//------------------------------------------------------------------
void loop()
{
    // Nothing to do here, just wait
    delay(10000);
    Serial.println("Agora Example 02 - Receiving different messages");
}