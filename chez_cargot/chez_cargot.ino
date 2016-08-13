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

// LED Ball Sizes
#define XL_BALL_LEDS 11
#define L_BALL_LEDS 7
#define M_BALL_LEDS 5
#define S_BALL_LEDS 4
#define BALL_SIZES 4

#define NUM_XL_BALLS 4
#define NUM_L_BALLS 8
#define NUM_M_BALLS 8
#define NUM_S_BALLS 8

uint8_t ballSizes[] = {XL_BALL_LEDS, L_BALL_LEDS, M_BALL_LEDS, S_BALL_LEDS};

// First offset is at end of XL balls
// Second offset is at end of L balls (after XL's)
// Third offset is at end of M balls (right before S's at the end)
// Fourth offset is the end of the strip (end of S's)
uint16_t ballOffsets[] = {
    (XL_BALL_LEDS * NUM_XL_BALLS),
    (XL_BALL_LEDS * NUM_XL_BALLS) + (L_BALL_LEDS * NUM_L_BALLS),
    NUM_LEDS - (S_BALL_LEDS * NUM_S_BALLS),
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

// Represents the current animation mode
typedef enum {NORMAL_MODE, BEAT_MODE, FLASHING_RED_MODE} AnimationMode;
AnimationMode currentMode = NORMAL_MODE;


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
    for(uint16_t i = 0; i < 16; i++) {
        uint16_t index = beatsin16(i * 2, 0, NUM_LEDS);
        leds[index] |= ColorFromPalette(palette, i * 16, MAX_BRIGHTNESS);
    }
}


/*
 * Pattern which has multiple trails of LED "comets" moving along the strip.
 */
void cometPattern(boolean hsvColors) {
    static uint16_t offset = 0;

    fadeToBlackBy(leds, NUM_LEDS, 127);
    for (uint16_t i = offset; i < NUM_LEDS; i += LED_GROUP_SIZE) {
        if (hsvColors) {
            CHSV hsv = ColorFromPalette(
                hsvPalettes[currentHSVPalette], getGradientHue(i),
                MAX_BRIGHTNESS);
            hsv.hue += 16 * offset;
            leds[i] = hsv;
        } else {
            leds[i] = ColorFromPalette(
                rgbPalettes[currentRGBPalette], getGradientHue(i),
                MAX_BRIGHTNESS);
        }
    }
    offset = (++offset) % LED_GROUP_SIZE;
    delay(50);
}


/*
 * See cometPattern, uses RGB Palettes.
 */
void cometPatternRGB() {
    cometPattern(false);
}


/*
 * See cometPattern, uses HSV Palettes.
 */
void cometPatternHSV() {
    cometPattern(true);
}


/*
 * Pattern that starts from individual points and converges with others
 */
void convergePattern(boolean hsvColors) {
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
                if (hsvColors) {
                    CHSV hsv = ColorFromPalette(
                        hsvPalettes[currentHSVPalette],
                        getGradientHue(i), DEFAULT_BRIGHTNESS);
                    hsv.sat = beatsin8(30, MIN_SAT, MAX_SAT);
                    hsv.hue += 48 * dist;
                    leds[j] = hsv;
                } else {
                    leds[j] = ColorFromPalette(
                        rgbPalettes[currentRGBPalette], getGradientHue(j),
                        DEFAULT_BRIGHTNESS);
                }
            } else {
                leds[j] = CRGB::Black;
            }
        }
    }

    dist = (++dist) % (maxDist + 1);
    if (dist == 0) {
        goingOut = !goingOut;
    }
    delay(75);
}


/*
 * See convergePattern, uses RGB Palettes.
 */
void convergePatternRGB() {
    convergePattern(false);
}


/*
 * See convergePattern, uses RGB Palettes.
 */
void convergePatternHSV() {
    convergePattern(true);
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
    uint8_t beat = beatsin8(bpm, 64, 255);
    uint8_t slowBeat = beatsin8(bpm / 10, 64, 255);

    CRGBPalette16 palette = OctocatColorsPalette_p;

    for(uint16_t i = 0; i < NUM_LEDS; i++) {
        uint8_t brightness = beat - (rainbowHue + (i * 8));
        leds[i] = ColorFromPalette(
            palette, slowBeat - (rainbowHue + (i * 2)), brightness);
    }
}


/*
 * Turns on LED's at random, creating a sparkling pattern where LED's slowly
 * fade to black.
 */
void randomSparklesPattern(boolean groupHues) {
    uint16_t pos = random16(NUM_LEDS);
    uint8_t hue;

    fadeToBlackBy(leds, NUM_LEDS, 10);
    if (groupHues) {
        hue = getGroupHue(pos);
    } else {
        hue = rainbowHue;
    }
    leds[pos] += CHSV(hue + random8(64), DEFAULT_SAT, MAX_BRIGHTNESS);
}


/*
 * See randomSparklesPattern using group index hues.
 */
void randomSparklesGroupPattern() {
    randomSparklesPattern(true);
}


/*
 * See randomSparklesPattern using rainbow hues.
 */
void randomSparklesRainbowPattern() {
    randomSparklesPattern(false);
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
    randomSparklesGroupPattern,
    randomSparklesRainbowPattern,
    cometPatternHSV,
    cometPatternRGB,
    convergePatternRGB,
    convergePatternHSV,
    pulsingPattern,
    twinklePattern,
    glitterPattern,
    beatSyncMultiplesPattern,
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
    currentPattern = (currentPattern + 1) % patternsLength;
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
