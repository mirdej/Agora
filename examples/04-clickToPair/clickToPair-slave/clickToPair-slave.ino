#include "Agora.h"
#include "FastLED.h"

#define PIN_PIX 7
#define NUM_PIXELS 1
CRGB pixel[NUM_PIXELS];
CRGB lastPixel[NUM_PIXELS];

// GPIO for pairing button
#define PIN_PAIRING 2
int lastButtonState;

//-----------------------------------------------------------------------------------------------------------------------------

void myCallback(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    Serial.printf("Message from my guru: %s\n", incomingData);
}

//-----------------------------------------------------------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    FastLED.addLeds<SK6812, PIN_PIX, GRB>(pixel, NUM_PIXELS);

    //Agora.forgetFriends();
    Agora.begin("Sheep",true);

    // Aspire to a cult and register a function that is called when our guru sends a message
    // but do not pair automatically
    Agora.join("Secret Cult", myCallback, false);

    //Agora.setPingInterval(200)
}

//-----------------------------------------------------------------------------------------------------------------------------

void loop()
{
    // Neopixel as status indicator
    pixel[0] = Agora.isPairing() ? CRGB::Yellow : Agora.isConnected() ? CRGB::Green
                                                                      : CRGB::Red;
    if (pixel[0] != lastPixel[0])
    {
        FastLED.show();
        lastPixel[0] = pixel[0];
    }

    // Start pairing on press of button
    int buttonState = digitalRead(PIN_PAIRING);
    if (!buttonState && buttonState != lastButtonState)
    {
        Agora.conspire(); // look for a guru for default amount of time
                          // Agora.conspire(20); // look for a guru for 20 seconds
                          // Agora.conspire("Secret Cult") // look for a guru for a specific cult
        Serial.printf("Pairing Started.\n");
    }
    lastButtonState = buttonState;

    delay(10);
}