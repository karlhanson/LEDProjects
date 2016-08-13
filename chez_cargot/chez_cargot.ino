#include <FastLED.h>

// =======================
// Arduino Pin Definitions
// =======================

// Dotstar LED Pins
/* #define CLOCK_PIN 4  // Pin for LED Clock (Dotstar) */
/* #define DATA_PIN 5   // Pin For LED Data (Dotstar) */

// ===============
// HSV Definitions
// ===============

// Brightness/Value
#define DEFAULT_BRIGHTNESS 191
#define MIN_BRIGHTNESS 63
#define MAX_BRIGHTNESS 223

// Saturation
#define DEFAULT_SAT 223
#define MIN_SAT 191
#define MAX_SAT 255

// ==========================
// LED Strip Size Definitions
// ==========================

// Number of LED's On Strip (NUM_LEDS % 12 should == 0)
#define NUM_LEDS 172

// Size of a "group" for certain animations.
#define LED_GROUP_SIZE (NUM_LEDS / 12)

// LED Orb Sizes
#define XL_ORB_LEDS 11
#define L_ORB_LEDS 7
#define M_ORB_LEDS 5
#define S_ORB_LEDS 4
#define ORB_SIZES 4

#define NUM_XL_ORBS 4
#define NUM_L_ORBS 8
#define NUM_M_ORBS 8
#define NUM_S_ORBS 8
#define NUM_ORBS

uint8_t orbSizes[] = {XL_ORB_LEDS, L_ORB_LEDS, M_ORB_LEDS, S_ORB_LEDS};

// First offset is at end of XL orbs
// Second offset is at end of L orbs (after XL's)
// Third offset is at end of M orbs (right before S's at the end)
// Fourth offset is the end of the strip (end of S's)
uint16_t orbOffsets[] = {
    (XL_ORB_LEDS * NUM_XL_ORBS),
    (XL_ORB_LEDS * NUM_XL_ORBS) + (L_ORB_LEDS * NUM_L_ORBS),
    NUM_LEDS - (S_ORB_LEDS * NUM_S_ORBS),
    NUM_LEDS
};

// ========================
// Time Syncing Definitions
// ========================

#define FPS 120             // Frames Per Second

// Amount of time to show each pattern
#define PATTERN_SECS 20

// Amount of time to show each palette
#define PALETTE_SECS (PATTERN_SECS / 4)

// Amount of time between each hue in rainbow
#define RAINBOW_MILLIS 20

// The LED's, man!
CRGB leds[NUM_LEDS];

// Designates whether the group size is even or odd.
const boolean IS_GROUP_SIZE_EVEN = LED_GROUP_SIZE % 2 == 0;

// Designates the "center" index of a given group.
// For even groups sizes, this is "to the right" of where the center would be.
const uint16_t GROUP_CENTER = LED_GROUP_SIZE / 2;

// Typedef to make function pointer array easier to type.
typedef void (*PatternArray[])();

typedef enum {
    RAINBOW_ANIM, GROUP_ANIM, RGB_PALETTE_ANIM, HSV_PALETTE_ANIM
} AnimationType;

const CHSVPalette16 OctocatColorsPalette_p(
    CHSV(0, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(127, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(42, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(170, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(234, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(191, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(64, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(191, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(106, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(234, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(42, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(255, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(128, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(255, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(170, DEFAULT_SAT, MAX_BRIGHTNESS),
    CHSV(42, DEFAULT_SAT, MAX_BRIGHTNESS)
);

CHSVPalette16 hsvPalettes[] = {
    OctocatColorsPalette_p
};

// TODO: Add more color palettes
CRGBPalette16 rgbPalettes[] = {
    OctocatColorsPalette_p, LavaColors_p, PartyColors_p, OceanColors_p
};


// Length of hsvPalettes[]
const uint16_t hsvPalettesLength = (
    sizeof(hsvPalettes) / sizeof(hsvPalettes[0]));

// Length of rgbPalettes[]
const uint16_t rgbPalettesLength = (
    sizeof(rgbPalettes) / sizeof(rgbPalettes[0]));


uint8_t currentPattern = 0;     // Index of currently selected pattern
uint8_t currentHSVPalette = 0;  // Index of currently selected HSV Palette
uint8_t currentRGBPalette = 0;  // Index of currently selected RGB Palette
uint8_t rainbowHue = 0;         // Global value for cycling through hues


// =================
// Utility Functions
// =================


/*
 * Returns a hue index that corresponds with the given LED index.
 *
 * This function smoothly interpolates over the range of [0, 255].
 *
 * Pre: 0 <= led < NUM_LEDS
 * Returns: 0 <= gradientHue <= 255
 */
uint8_t getGradientHue(uint16_t led) {
    return (led * 255) / (NUM_LEDS - 1);
}


/*
 * Returns a hue index that corresponds to the group index of the given LED.
 *
 * There are always 12 groups, and each group number corresponds to a value
 * over [0, 255]; this function returns the discrete value that corresponds
 * with a given LED's group. For example, if LED_GROUP_SIZE == 15, led
 * indexes [0, 14] would all return the same value, and the next hue starts at
 * led index 15.
 *
 * Pre: 0 <= led < NUM_LEDS
 * Returns: 0 <= groupHue <= 255
 */
uint8_t getGroupHue(uint16_t led) {
    return ((led / LED_GROUP_SIZE) * 255) / 11;
}


// ============
// LED Patterns
// ============


/*
 * Notes For LED Patterns
 *
 * Each LED Pattern should be defined as one iteration of the loop, preserving
 * any state as necessary between subsequent calls (since each pattern function
 * is called in every iteration of the Arduino hardware's loop() function).
 *
 * General Steps For Creating A New Pattern:
 * - Create a function with no parameters, ideally in the format of *Pattern.
 * - Add the function to the global PatternArray patterns (which essentially is
 *   an array of function pointers that change automatically).
 *
 * Other Tips:
 * - Use rgbPalettes with currentRGBPalette, and hsvPalettes with
 *   currentHSVPalette to easily add dynamic color changes to your pattern
 *   using FastLED's ColorFromPalette.
 * - If you want to make variations of a pattern, create master functinos that
 *   take parameters, and add small parameter-less functions that call your
 *   master pattern function in different states.
 */


/*
 * Pattern that goes the entire strip in 16 off beat sin waves.
 */
void beatSyncMultiplesPattern() {
    fadeToBlackBy(leds, NUM_LEDS, 20);
    CRGBPalette16 palette = rgbPalettes[currentRGBPalette];
    for(uint16_t i = 0; i < 16; ++i) {
        uint16_t index = beatsin16(i * 2, 0, NUM_LEDS);
        leds[index] |= ColorFromPalette(palette, i * 16, MAX_BRIGHTNESS);
    }
}


/*
 * Pattern which has multiple trails of LED "comets" moving along the strip.
 */
void cometPattern(AnimationType animType) {
    static uint16_t offset = 0;

    fadeToBlackBy(leds, NUM_LEDS, 127);
    for (uint16_t i = offset; i < NUM_LEDS; i += LED_GROUP_SIZE) {
        switch (animType) {
            case HSV_PALETTE_ANIM:
                {
                    CHSV hsv = ColorFromPalette(
                        hsvPalettes[currentHSVPalette], getGradientHue(i),
                        MAX_BRIGHTNESS);
                    hsv.hue += 16 * offset;
                    leds[i] = hsv;
                }
                break;
            case RGB_PALETTE_ANIM:
            default:
                leds[i] = ColorFromPalette(
                    rgbPalettes[currentRGBPalette], getGradientHue(i),
                    MAX_BRIGHTNESS);
                break;
        }
    }
    offset = (offset + 1) % LED_GROUP_SIZE;
    delay(50);
}


/*
 * See cometPattern, uses RGB Palettes.
 */
void cometPatternRGB() {
    cometPattern(RGB_PALETTE_ANIM);
}


/*
 * See cometPattern, uses HSV Palettes.
 */
void cometPatternHSV() {
    cometPattern(HSV_PALETTE_ANIM);
}


/*
 * Pattern that starts from individual points and converges with others
 */
void convergePattern(AnimationType animType) {
    static uint8_t dist = 0;
    static boolean goingOut = true;

    uint16_t maxDist = GROUP_CENTER;
    uint16_t start = GROUP_CENTER - dist;
    if (IS_GROUP_SIZE_EVEN) {
        --start;
        --maxDist;
    }
    for (uint16_t i = 0; i < NUM_LEDS; i += LED_GROUP_SIZE) {
        for (uint16_t j = i + start; j <= i + GROUP_CENTER + dist; ++j) {
            if (goingOut) {
                switch (animType) {
                    case HSV_PALETTE_ANIM:
                        {
                            CHSV hsv = ColorFromPalette(
                                hsvPalettes[currentHSVPalette],
                                getGradientHue(j), DEFAULT_BRIGHTNESS);
                            hsv.sat = beatsin8(30, MIN_SAT, MAX_SAT);
                            hsv.hue += 48 * dist;
                            leds[j] = hsv;
                        }
                        break;
                    case RGB_PALETTE_ANIM:
                    default:
                        leds[j] = ColorFromPalette(
                            rgbPalettes[currentRGBPalette], getGradientHue(j),
                            DEFAULT_BRIGHTNESS);
                        break;
                }
            } else {
                leds[j] = CRGB::Black;
            }
        }
    }

    dist = (dist + 1) % (maxDist + 1);
    if (dist == 0) {
        goingOut = !goingOut;
    }
    delay(75);
}


/*
 * See convergePattern, uses RGB Palettes.
 */
void convergePatternRGB() {
    convergePattern(RGB_PALETTE_ANIM);
}


/*
 * See convergePattern, uses RGB Palettes.
 */
void convergePatternHSV() {
    convergePattern(HSV_PALETTE_ANIM);
}


/*
 * Pattern that goes through Octocat Colors with glittering lights and
 * "morphing" like quality.
 */
void glitterPattern() {
    uint8_t bpm = 30;
    uint8_t beat = beatsin8(bpm, 127, MAX_BRIGHTNESS);
    CRGBPalette16 palette = OctocatColorsPalette_p;

    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
        leds[i] = ColorFromPalette(palette, rainbowHue + (i * 3), beat);
    }
    if(random8() < 64) {
        leds[random16(NUM_LEDS)] += CRGB::White;
    }
}


/*
 * Beat syncing pattern that pulses in both directions with a varying hue and
 * brightness factor.
 */
void pulsingPattern() {
    uint8_t bpm = 30;
    uint8_t beat = beatsin8(bpm, 64, MAX_BRIGHTNESS);
    uint8_t slowBeat = beatsin8(bpm / 10, 64, 255);

    CRGBPalette16 palette = OctocatColorsPalette_p;

    for(uint16_t i = 0; i < NUM_LEDS; ++i) {
        uint8_t brightness = beat - (rainbowHue + (i * 8));
        leds[i] = ColorFromPalette(
            palette, slowBeat - (rainbowHue + (i * 2)), brightness);
    }
}


/*
 * Turns on LED's at random, creating a sparkling pattern where LED's slowly
 * fade to black.
 */
void randomSparklesPattern(AnimationType animType) {
    uint16_t pos = random16(NUM_LEDS);
    uint8_t hue;

    fadeToBlackBy(leds, NUM_LEDS, 10);
    switch(animType) {
        case GROUP_ANIM:
            hue = getGroupHue(pos);
            break;
        case RAINBOW_ANIM:
        default:
            hue = rainbowHue;
            break;
    }
    leds[pos] += CHSV(hue + random8(64), DEFAULT_SAT, MAX_BRIGHTNESS);
}


/*
 * See randomSparklesPattern using group index hues.
 */
void randomSparklesGroupPattern() {
    randomSparklesPattern(GROUP_ANIM);
}


/*
 * See randomSparklesPattern using rainbow hues.
 */
void randomSparklesRainbowPattern() {
    randomSparklesPattern(false);
}


void spiralPattern(boolean dynamicDelay, boolean inAndOut) {
    static uint16_t ledOffset = 0;
    static uint16_t orbSizeOffset = 0;
    static uint8_t lightsOn = 0;
    static boolean inwards = true;

    uint16_t orbSize = orbSizes[orbSizeOffset];
    uint16_t endOfOrb;

    fadeToBlackBy(leds, NUM_LEDS, 31);

    if (lightsOn == 0) {
        if (inwards) {
            endOfOrb = ledOffset + orbSize;
            for (uint16_t i = ledOffset; i < endOfOrb; ++i) {
                leds[i] = ColorFromPalette(
                    rgbPalettes[currentRGBPalette], getGradientHue(i),
                    MAX_BRIGHTNESS);
            }

            ledOffset = endOfOrb;
            if (endOfOrb >= orbOffsets[orbSizeOffset]) {
                orbSizeOffset = (++orbSizeOffset) % ORB_SIZES;
            }

            if (endOfOrb == NUM_LEDS) {
                if (inAndOut) {
                    // We flip direction and start going outward
                    inwards = false;
                    orbSizeOffset = ORB_SIZES - 1;
                    ledOffset = NUM_LEDS - orbSizes[orbSizeOffset];
                } else {
                    // We start from the beginning
                    ledOffset = 0;
                }
            }

        } else {
            endOfOrb = ledOffset - orbSize;
            for (uint16_t i = ledOffset; i - 1 >= endOfOrb; --i) {
                leds[i - 1] = ColorFromPalette(
                    rgbPalettes[currentRGBPalette], getGradientHue(i),
                    MAX_BRIGHTNESS);
            }

            ledOffset = endOfOrb;
            if (orbSizeOffset > 0 && endOfOrb <= orbOffsets[orbSizeOffset - 1]) {
                orbSizeOffset = (--orbSizeOffset);
            }

            if (endOfOrb == 0) {
                if (inAndOut) {
                    // We flip direction and start heading inward
                    inwards = true;
                    orbSizeOffset = 0;
                    ledOffset = orbSizes[orbSizeOffset];
                } else {
                    // We start from the end (center)
                    orbSizeOffset = ORB_SIZES - 1;
                    ledOffset = NUM_LEDS;
                }
            }
        }
    }
    lightsOn = (lightsOn + 1) % 4;

    if (dynamicDelay) {
        delay(beatsin8(15, 15, 30));
    } else {
        delay(20);
    }
}


void spiralPatternSteady() {
    spiralPattern(false, false);
}


void spiralSteadyInAndOutPattern() {
    spiralPattern(false, true);
}


void spiralPatternDynamic() {
    spiralPattern(true, false);
}


void spiralDynamicInAndOutPattern() {
    spiralPattern(true, true);
}


/*
 * Pattern that twinkles on even and odd lights with changing saturation.
 */
void twinklePattern() {
    uint8_t bpm = 30;
    uint8_t beat = beatsin8(bpm, 0, 255);
    CHSVPalette16 palette = hsvPalettes[currentHSVPalette];
    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
        CHSV hsv = ColorFromPalette(
            palette, getGroupHue(i), DEFAULT_BRIGHTNESS);
        hsv.sat = lerp8by8(MIN_SAT, MAX_SAT, beat);
        if (i % 2 == 0) {
            hsv.val = lerp8by8(MAX_BRIGHTNESS, MIN_BRIGHTNESS, beat);
        } else {
            hsv.val = lerp8by8(MIN_BRIGHTNESS, MAX_BRIGHTNESS, beat);
        }
        leds[i] = hsv;
    }
}


// =======================
// Patterns Initialization
// =======================


/*
 * Global list of pattern functions.
 * Each should be the name of a function that has return type void and takes no
 * parameters.
 */
PatternArray patterns = {
    beatSyncMultiplesPattern,
    cometPatternHSV,
    cometPatternRGB,
    convergePatternHSV,
    convergePatternRGB,
    glitterPattern,
    pulsingPattern,
    randomSparklesGroupPattern,
    randomSparklesRainbowPattern,
    spiralDynamicInAndOutPattern,
    spiralPatternDynamic,
    spiralPatternSteady,
    spiralSteadyInAndOutPattern,
    twinklePattern
};


// Length of patterns[]
const uint16_t patternsLength = sizeof(patterns) / sizeof(patterns[0]);


// ========================
// State Changing Functions
// ========================

/*
 * Increments the index on both the HSV and RGB palette arrays. This should be
 * called in a EVERY_N_SECONDS macro provided by FastLED in order to
 * dynamically change color palettes in patterns.
 */
void nextPalette() {
    currentRGBPalette = (currentRGBPalette + 1) % rgbPalettesLength;
    currentHSVPalette = (currentHSVPalette + 1) % hsvPalettesLength;
}


/*
 * Increments the index of patterns array in order to switch to the next
 * pattern. This should be called in a EVERY_N_SECONDS macro provided by
 * FastLED in order to dynamically change patterns.
 */
void nextPattern() {
    currentPattern = random8(patternsLength);
}


// ======================
// Main Arduino Functions
// ======================


void setup() {
    // Initialize the LED Strip.
    FastLED.addLeds<APA102, BGR>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);

    // Set the color temperature
    FastLED.setTemperature(CarbonArc);

    // Set the global brightness
    FastLED.setBrightness(DEFAULT_BRIGHTNESS);
}


void loop() {
    patterns[currentPattern]();
    FastLED.show(); // Show the LED's
    FastLED.delay(1000 / FPS);  // Add a global delay at the frame rate.

    // Switch patterns every PATTERN_SECS
    EVERY_N_SECONDS(PATTERN_SECS) { nextPattern(); }

    // Switch palettes every PALETTE_SECS
    EVERY_N_SECONDS(PALETTE_SECS) { nextPalette(); }

    // Increment the "rainbow hue" every RAINBOW_MILLIS milli's.
    EVERY_N_MILLISECONDS(RAINBOW_MILLIS) { ++rainbowHue; }
}
