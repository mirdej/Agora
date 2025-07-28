#include "Agora.h"

typedef struct
{
    int id;
    float x, y, z;
    char u, v, w;
    long timestamp;
} accelerometerData_t;

accelerometerData_t myData;

//------------------------------------------------------------------
void onDataRecv(const uint8_t *senderMac, const uint8_t *data, int len)
{
    if (len == sizeof(myData))
    {
        // copy received bytes into struct
        memcpy(&myData, data, len);

        // now we can access the data comfortably
        Serial.printf("\nSensor ID: %d", myData.id);
        Serial.printf("\nAccel (xyz): %.2f %.2f %.2f\n", myData.x, myData.y, myData.z);
        Serial.printf("U: %d, V: %d, W: %d\n", myData.u, myData.v, myData.w);
        Serial.printf("Timestamp: %u\n------------\n", myData.timestamp);

        // Agora.tell() sends messages to all followers
        // Agora.answer() lets you specify to which mac address you're sending
        Agora.answer(senderMac, "Thank you!");
    }
    else
    {
        Serial.printf("Weird number of bytes received: %u", len);
    }
}

//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);

    delay(3000);
    Serial.println("Agora Example 03 - Sending an receiving complex data using structs");

    Agora.forgetFriends(); // force pairing every time we start
                           // this is not needed in a stable environment,
                           // but when we're checking out different examples with different tribe names
                           // it's safer to restart from scratch each time

    Agora.begin("Structured Guru");

    Agora.establish("Life is complex", onDataRecv); // yes, followers can also send data back to their guru...
}

//------------------------------------------------------------------
void loop()
{
    // Nothing to do here, just wait
    delay(10000);
    Serial.println("Agora Example 03 - Structured data");
}