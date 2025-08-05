/*

ESP32 goes to sleep after SLEEP_TIMEOUT milliseconds.
Wake up on pin change interrup when button is pressed
Measure time it takes to get ready in microseconds, connect to guru and send button state
Guru answers with button state (int) -> time again and send all measurements back to guru for serial printing

*/

#include <Arduino.h>

#define AGORA_LOG_LEVEL 0
#include <Agora.h>

#define PIN_LED 10
#define PIN_BTN 2
#define SLEEP_TIMEOUT 8000

typedef struct
{
  long setupstart;
  long agoraready;
  long setupend;
  long messageSent;
  long answerReceived;
} timingMeasurements_t;

timingMeasurements_t timing;
int lastButton;
long lastInteractionTime;

//--------------------------------------------------------------------------------
void onInt(int a)
{
  timing.answerReceived = micros();
  int len = sizeof(timing);
  uint8_t buf[len];
  memcpy(buf, (uint8_t *)&timing, len);
  Agora.tell(buf, len);
}

//--------------------------------------------------------------------------------

void setup()
{
  timing.setupstart = micros();

  Agora.begin("Profiler-Slave");
  Agora.join("profiling");
  Agora.onInt(onInt);

  timing.agoraready = micros();

  Serial.begin(115200);
  Serial.println("AGORA Deep Sleep example - SLAVE");

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  lastButton = -1;
  pinMode(PIN_BTN, INPUT_PULLUP);
  esp_deep_sleep_enable_gpio_wakeup((1 << PIN_BTN), ESP_GPIO_WAKEUP_GPIO_LOW);

  // put you setup code here...

  Serial.println("Setup Done");
  timing.setupend = micros();
}
//--------------------------------------------------------------------------------

void loop()
{
  int btn = digitalRead(PIN_BTN);
  digitalWrite(PIN_LED, !btn);

  if (btn != lastButton)
  {
    lastButton = btn;
    lastInteractionTime = millis();
    timing.messageSent = micros();
    Agora.tell(btn);
  }

  if (millis() - lastInteractionTime > SLEEP_TIMEOUT)
  {
    // go to sleep
    Serial.println("Going to sleep. Bye.");
    esp_wifi_stop();
    pinMode(PIN_LED, INPUT); // turn off led

    esp_deep_sleep_start();
  }
}
