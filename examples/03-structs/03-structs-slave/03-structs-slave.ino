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
void myCallback(const uint8_t *mac, const uint8_t *data, int len)
{
    Serial.printf("Received %s\n", data);
}

//------------------------------------------------------------------
void setup()
{
    Serial.begin(115200);
    delay(3000);
    Serial.println("Agora Example 03 - Sending structured data to the Guru");

    Agora.forgetFriends(); // force pairing every time we start
                           // this is not needed in a stable environment,
                           // but when we're checking out different examples with different tribe names
                           // it's safer to restart from scratch each time

    Agora.begin("Structured follower");

    Agora.join("Life is complex", myCallback);

    // Agora creates a DMX-Style address between 1 and 512 for each device (based on their MAC address)
    // that you can use to identify individual devices
    myData.id = Agora.address;
}

//------------------------------------------------------------------
void loop()
{
    // generate some random data
    myData.u = random(255);
    myData.v = random(255);
    myData.w = random(255);

    myData.x = (float)random(-1000, 1000) / 1000.;
    myData.y = (float)random(-1000, 1000) / 1000.;
    myData.z = (float)random(-1000, 1000) / 1000.;

    myData.timestamp = millis();

    // create a temporary byte buffer big enough to hold data structure .
    // then copy len bytes starting from the address of the myData structure (&myData) into the buffer
    // as if it were simply a collection of unsigned bytes (which it actually is, in the end...)
    int len = sizeof(myData);
    uint8_t buf[len];
    memcpy(buf, (uint8_t *)&myData, len);

    // send this buffer to my GURU
    Agora.tell(buf, len);

    // wait a bit
    delay(2000);
}