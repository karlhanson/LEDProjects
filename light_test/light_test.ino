#include <FastLED.h>

#define CLOCK_PIN 8
#define DATA_PIN 9
#define NUM_LEDS 56

CRGB leds[NUM_LEDS];

void setup() {
    // Initialize the LED Strip.
    /* FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN>( */
    /*     leds, NUM_LEDS).setCorrection(TypicalSMD5050); */
    FastLED.addLeds<APA102>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);

    // Set the color temperature
    FastLED.setTemperature(CarbonArc);
}

CRGB getColor(uint8_t colorIndex) {
    switch(colorIndex) {
        case 0:
            return CRGB::Red;
            break;
        case 1:
            return CRGB::Blue;
            break;
        case 2:
        default:
            return CRGB::Green;
            break;
    }
}

static uint8_t iteration = 0;

void loop() {
    uint8_t i = 0;
    for (i = 0; i < NUM_LEDS; ++i) {
        leds[i] = CRGB::Black;
    }
    FastLED.show();
    for (i = 0; i < NUM_LEDS; ++i) {
        leds[i] = getColor(iteration);
        FastLED.show();
        delay(25);
    }
    for (i = 0; i < NUM_LEDS; ++i) {
        leds[i] = CRGB::Black;
    }
    FastLED.show();
    iteration = (iteration + 1) % 3;
    delay(100);
}
