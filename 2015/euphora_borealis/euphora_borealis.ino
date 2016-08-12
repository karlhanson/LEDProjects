#include <FastLED.h>

const uint8_t CLOCK_PIN = 13;
const uint8_t DATA_PIN = 11;
const uint16_t NUM_LEDS = (5 * 30);

const uint8_t DEFAULT_HUE = 5;
const uint8_t DEFAULT_BRIGHTNESS = 127;
const uint8_t DEFAULT_SAT = 191;
const uint8_t LED_GROUP_SIZE = 6;
const uint8_t MAX_BRIGHTNESS = 223;
const uint8_t MAX_SAT = 255;
const uint8_t MIN_BRIGHTNESS = 63;
const uint8_t MIN_SAT = 191;
const uint8_t NUM_HUE_DISTANCES = 6;

const uint8_t LEFT_CENTER = (
    LED_GROUP_SIZE % 2 == 0 ? (LED_GROUP_SIZE / 2) - 1 : LED_GROUP_SIZE / 2);
const uint8_t RIGHT_CENTER = LED_GROUP_SIZE / 2;
const uint8_t DIST_CENTER = (
    LED_GROUP_SIZE % 2 == 0 ? LED_GROUP_SIZE / 2 : (LED_GROUP_SIZE / 2) + 1);

const uint8_t HUE_DISTANCES[NUM_HUE_DISTANCES] = {0, 127, 42, 170, 234, 191};

CRGB leds[NUM_LEDS];
CHSV hsvs[NUM_LEDS];

uint8_t calcHueForGroup(uint8_t baseHue, uint8_t group) {
    uint8_t hueDistance = (group / LED_GROUP_SIZE) % NUM_HUE_DISTANCES;
    return baseHue + HUE_DISTANCES[hueDistance];
}

void fadeLightsOff() {
    while (hsvs[0].val != 0) {
        for (uint8_t i = 0; i < NUM_LEDS; ++i) {
            hsvs[i].val = lerp8by8(0, hsvs[i].val, 224);
        }
        hsv2rgb_rainbow(hsvs, leds, NUM_LEDS);
        FastLED.show();
        delay(50);
    }
}

void turnOffLights() {
    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
        hsvs[i].val = 0;
    }
    hsv2rgb_rainbow(hsvs, leds, NUM_LEDS);
    FastLED.show();
}

uint8_t mapByteToRange(
        uint8_t val, boolean positive, uint8_t minVal, uint8_t maxVal) {
    if (positive) {
        return map(val, 0, 256, minVal, maxVal);
    } else {
        return map(val, 0, 256, maxVal, minVal);
    }
}

uint8_t mapBrightness(uint8_t val, boolean positive) {
    return mapByteToRange(val, positive, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
}

uint8_t mapSaturation(uint8_t val, boolean positive) {
    return mapByteToRange(val, positive, MIN_SAT, MAX_SAT);
}

void setupHuesForGroups() {
    uint8_t baseHue = DEFAULT_HUE;
    uint8_t hue;
    for (uint16_t i = 0; i < NUM_LEDS; i += LED_GROUP_SIZE) {
        hue = calcHueForGroup(baseHue, i);
        for (uint8_t j = i; j < i + LED_GROUP_SIZE; ++j) {
            hsvs[j] = CHSV(hue, DEFAULT_SAT, 0);
        }
        if (i != 0 && (i / LED_GROUP_SIZE) % NUM_HUE_DISTANCES == 0) {
            baseHue += 63;
        }
    }
}

void twinkleLoop(uint8_t numLoops) {
    uint8_t wave, brightUp, brightDown;
    setupHuesForGroups();
    for (uint8_t i = 0; i < numLoops; ++i) {
        for (uint16_t fade = 0; fade <= 255; ++fade) {
            wave = sin8(fade);
            brightUp = mapBrightness(wave, true);
            brightDown = mapBrightness(wave, false);
            for (uint16_t dot = 0; dot < NUM_LEDS; ++dot) {
                if (dot % 2 == 0) {
                    hsvs[dot].val = brightDown;
                } else {
                    hsvs[dot].val = brightUp;
                }
                hsvs[dot].sat = mapSaturation(wave, true);
            }
            hsv2rgb_rainbow(hsvs, leds, NUM_LEDS);
            FastLED.show();
            delay(8);
        }
    }
}

void movingLoop(uint8_t numLoops) {
    uint8_t hue;
    setupHuesForGroups();
    for (uint8_t loopNum = 0; loopNum < numLoops; ++loopNum) {
        turnOffLights();
        for (uint8_t i = 0; i < LED_GROUP_SIZE; ++i) {
            for (uint16_t j = 0; j < NUM_LEDS; j += LED_GROUP_SIZE) {
                hsvs[j + i].hue += 16;
                hsvs[j + i].sat = MAX_SAT;
                hsvs[j + i].val = DEFAULT_BRIGHTNESS;
                for (uint8_t k = j; k < j + i; ++k) {
                    hsvs[k].val = lerp8by8(0, hsvs[k].val, 128);
                    hsvs[k].hue += 16;
                }
            }
            hsv2rgb_rainbow(hsvs, leds, NUM_LEDS);
            FastLED.show();
            delay(110);
        }
    }
}

void panningAnimation(uint8_t left, uint8_t right, uint8_t len, boolean out) {
    for (uint8_t i = 0; i < len; ++i) {
        if (i != 0) {
            if (out) {
                --left, ++right;
            } else {
                ++left, --right;
            }
        }
        if (i == len - 1 && !out) {
            continue;
        }
        for (uint16_t j = 0; j < NUM_LEDS; j += LED_GROUP_SIZE) {
            if (out) {
                hsvs[j + left].sat = MAX_SAT;
                hsvs[j + left].val = DEFAULT_BRIGHTNESS;
                hsvs[j + right].sat = MAX_SAT;
                hsvs[j + right].val = DEFAULT_BRIGHTNESS;
            } else {
                hsvs[j + left].val = 0;
                hsvs[j + right].val = 0;
            }
        }
        for (uint8_t j = 0; j < NUM_LEDS; ++j) {
            hsvs[j].hue += 32;
            if (i % 2 == 0) {
                hsvs[j].sat -= 32;
            } else {
                hsvs[j].sat += 32;
            }
        }
        hsv2rgb_rainbow(hsvs, leds, NUM_LEDS);
        FastLED.show();
        delay(400);
    }
}

void panningLoop(uint8_t numLoops) {
    setupHuesForGroups();
    for (uint8_t i = 0; i < numLoops; ++i) {
        panningAnimation(LEFT_CENTER, RIGHT_CENTER, DIST_CENTER, true);
        panningAnimation(0, LED_GROUP_SIZE - 1, DIST_CENTER, false);
    }
    panningAnimation(LEFT_CENTER, RIGHT_CENTER, DIST_CENTER, true);
}

void pingPongSide(uint8_t lower, uint8_t upper, int8_t inc) {
    uint8_t dir = lower + inc;
    boolean out = true;

    while (dir != lower) {
        for (uint16_t j = 0; j < NUM_LEDS; j += LED_GROUP_SIZE) {
            if (out) {
                hsvs[j + dir].sat = MAX_SAT;
                hsvs[j + dir].val = DEFAULT_BRIGHTNESS;
            } else {
                hsvs[j + dir].val = 0;
            }
        }
        if (dir == upper) {
            if (out) {
                out = false;
            } else {
                dir -= inc;
            }
        } else {
            if (out) {
                dir += inc;
            } else {
                dir -= inc;
            }
        }
        for (uint16_t i = 0; i < NUM_LEDS; ++i) {
            hsvs[i].hue += 32;
        }
        hsv2rgb_rainbow(hsvs, leds, NUM_LEDS);
        FastLED.show();
        delay(400);
    }
}

void pingPongAnimation(uint8_t left, uint8_t right) {
    for (uint16_t j = 0; j < NUM_LEDS; j += LED_GROUP_SIZE) {
        CHSV curHsv = CHSV(
            calcHueForGroup(DEFAULT_HUE, j),
            MAX_SAT, DEFAULT_BRIGHTNESS);
        hsvs[j + left].sat = hsvs[j + right].sat = MAX_SAT;
        hsvs[j + left].val = hsvs[j + right].val = DEFAULT_BRIGHTNESS;
    }
    hsv2rgb_rainbow(hsvs, leds, NUM_LEDS);
    FastLED.show();
    // Right
    pingPongSide(right, LED_GROUP_SIZE - 1, 1);
    // Left
    pingPongSide(left, 0, -1);
}

void pingPongLoop(uint8_t numLoops) {
    setupHuesForGroups();
    for (uint8_t i = 0; i < numLoops; ++i) {
        pingPongAnimation(LEFT_CENTER, RIGHT_CENTER);
    }
}

void setup() {
    Serial.begin(9600);
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN>(leds, NUM_LEDS);
}

void loop() {
    twinkleLoop(8);
    pingPongLoop(6);
    movingLoop(20);
    panningLoop(6);
}
