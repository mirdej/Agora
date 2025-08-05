#include <Arduino.h>
#include <Agora.h>

#define PING_INTERVAL 1000

typedef struct
{
  long setupstart;
  long agoraready;
  long setupend;
  long messageSent;
  long answerReceived;
} timingMeasurements_t;

timingMeasurements_t timing;

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len)
{
  if (len == sizeof(timing))
  {
    memcpy(&timing, data, len);
    if (timing.messageSent - timing.agoraready < 5000)
    {
      Serial.printf("Setup begins after %u microseconds\n", timing.setupstart);
      Serial.printf("Agora ready after %u microseconds\n", timing.agoraready);
      Serial.printf("Message sent at %u microseconds\n", timing.messageSent);
      Serial.printf("Answer received  at %u microseconds\n", timing.answerReceived);
    }
    Serial.printf("Latency %u microseconds\n\n", timing.answerReceived - timing.messageSent);
  }
}

//--------------------------------------------------------------------------------

void setup()
{
  Serial.printf("________________________\nSetup\n");

  // Agora.forgetFriends();
  Agora.begin("Profiler");
  Agora.establish("profiling", OnDataRecv);
  Agora.onInt([](int a)
              { 
                Agora.answer(a); 
                Serial.printf("Button %s\n", a ? "released":"pressed"); });

  Serial.printf("Setup Done\n________________________\n");
}
//--------------------------------------------------------------------------------

void loop()
{
  // AGORA_LOG_STATUS(10000);
  delay(10);
}
