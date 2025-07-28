#include "Agora.h"
#include "FastLED.h"

#define PIN_PIX 7
#define NUM_PIXELS 1
CRGB pixel[NUM_PIXELS];
CRGB lastPixel[NUM_PIXELS];

// GPIO for pairing button
#define PIN_PAIRING 2
int lastButtonState;

long lastMessage;

//-----------------------------------------------------------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);

    FastLED.addLeds<SK6812, PIN_PIX, GRB>(pixel, NUM_PIXELS);

    pinMode(PIN_PAIRING, INPUT_PULLUP);
    lastButtonState = digitalRead(PIN_PAIRING);

    // Agora.forgetFriends();
    //  Join the Agora with a distinct name
    //   second argument = true:
    Agora.begin("Picky Master", true);

    // Establish a cult without callback (NULL) and don't automatically accept members (false)
    Agora.establish("Secret Cult", NULL, false);

    // If you change pingInterval, make sure to do so on all Slaves too
    // Agora.setPingInterval(200)
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
        Agora.conspire(); // allow new members to join for default amount of time (20 seconds)
        // Agora.conspire(30000); // allow new members to join for 30 seconds
        // Agora.conspire("Secret Cult") // allow new members to join specific cult
        Serial.printf("Pairing Started.\n");
    }
    lastButtonState = buttonState;

    delay(20);
}