#include <FastLED.h>

// =======================
// Arduino Pin Definitions
// =======================

// Digital Clock Pin For APA102 (Dotstar) Timing
#define CLOCK_PIN 23  // SPI Clock Pin (Arduino Zero)

// Digital Data Pin For APA102 (Dotstar) Data
#define DATA_PIN 24  //SPI Data Pin (Arduino Zero)

// Analog Pin used to add entropy to random seed.
// NOTE: Nothing should be plugged into this pin.
#define RANDOM_ANALOG_PIN 0


// ================
// APA102/Dotstar Definitions
// ================

// RGB Order for LED strips
#define RGB_ORDER BGR


// ===============
// HSV Definitions
// ===============

// Brightness/Value
#define DEFAULT_BRIGHTNESS 191
#define MIN_BRIGHTNESS 63
#define MAX_BRIGHTNESS 255

// Saturation
#define DEFAULT_SAT 223
#define MIN_SAT 191
#define MAX_SAT 255


// ==========================
// LED Strip Size Definitions
// ==========================

// Number of LED's On Strip
#define NUM_LEDS 172

// Number of Color Groups throughout LED Strip
// Works best if it cleanly divides into NUM_LEDS or has a small remainder
#define NUM_GROUPS 19

// Size of a color group for certain animations.
#define LED_GROUP_SIZE ((NUM_LEDS) / (NUM_GROUPS))

// Number of LED's per orb size.
#define XL_ORB_LEDS 11
#define L_ORB_LEDS 7
#define M_ORB_LEDS 5
#define S_ORB_LEDS 4

// Number of Sizes
#define ORB_SIZES 4

// Number of each size of orb.
#define NUM_XL_ORBS 4
#define NUM_L_ORBS 8
#define NUM_M_ORBS 8
#define NUM_S_ORBS 8

// Total number of orbs.
#define NUM_ORBS ((NUM_XL_ORBS) + (NUM_L_ORBS) + (NUM_M_ORBS) + (NUM_S_ORBS))

// Number of rings.
#define NUM_RINGS 4

// TODO: Add more color palettes

// Starting indexes of each of the orbs.
uint16_t orbIndexes[NUM_ORBS];

// Number of LED's for each of the orbs
uint16_t orbSizes[NUM_ORBS];

/*
 * LED Ring Sizes
 *
 * Ring 1: XL Ring (0 - 11)
 * Ring 2: L Ring (12 - 21)
 * Ring 3: M Ring (22 - 26)
 * Ring 4: Center (27)
 */

#define RING_ONE 0
#define RING_TWO 12
#define RING_THREE 22
#define RING_FOUR 27

// Starting indexes of each of the rings.
const uint8_t ringIndexes[] = {
    RING_ONE, RING_TWO, RING_THREE, RING_FOUR
};

// Sizes of each of the rings
const uint8_t ringSizes[] = {
    RING_TWO - RING_ONE,
    RING_THREE - RING_TWO,
    RING_FOUR - RING_THREE,
    NUM_ORBS - RING_FOUR
};


// ========================
// Time Syncing Definitions
// ========================

// Frames Per Second
#define FPS 120

// Amount of time to show each pattern
#define PATTERN_SECS 24

// Amount of time to show each palette
#define PALETTE_SECS ((PATTERN_SECS) / 4)

// Amount of time between each hue in rainbow
#define RAINBOW_MILLIS 20


// ========
// Typedefs
// ========

// Typedef to make function pointer array easier to type.
typedef void (*PatternArray[])();

// Typedef for defining the animation type for a given pattern function.
// Generally used in a switch statement to add variety to existing animations.
typedef enum {
    RAINBOW_ANIM, GROUP_ANIM, RGB_PALETTE_ANIM, HSV_PALETTE_ANIM
} AnimationType;


// ================
// Global Variables
// ================

// The LED's, man!
CRGB leds[NUM_LEDS];

// Designates whether the group size is even or odd.
const boolean IS_GROUP_SIZE_EVEN = (LED_GROUP_SIZE) % 2 == 0;

// Designates the "center" index of a given group.
// For even groups sizes, this is "to the right" of where the center would be.
const uint16_t GROUP_CENTER = (LED_GROUP_SIZE) / 2;

uint8_t currentPattern;     // Index of currently selected pattern
uint8_t currentHSVPalette;  // Index of currently selected HSV Palette
uint8_t currentRGBPalette;  // Index of currently selected RGB Palette
uint8_t rainbowHue = 0;     // Global value for cycling through hues


// TODO: Add more color palettes
// ============
// HSV Palettes
// ============

/*
 * Chez Cargot Colors
 * A variety of dynamic colors that move across the color hue wheel in 1/6ths
 * with a few colors wrapping around, having a mixed sorting order that
 * encourages similar colors to be spread out.
 */
const CHSVPalette16 ChezCargotColorsPalette_p(
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

const CRGBPalette16 LavaNoBlackColorsPalette_p(
    CRGB::Crimson,
    CRGB::Maroon,
    CRGB::Red,
    CRGB::Maroon,

    CRGB::DarkRed,
    CRGB::Maroon,
    CRGB::DarkRed,
    CRGB::Orange,

    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::Red,
    CRGB::Orange,

    CRGB::White,
    CRGB::Orange,
    CRGB::Red,
    CRGB::DarkRed
);

CHSVPalette16 hsvPalettes[] = {
    ChezCargotColorsPalette_p, ChezCargotColorsPalette_p
};

// TODO: Add more color palettes
CRGBPalette16 rgbPalettes[] = {
    ChezCargotColorsPalette_p, LavaNoBlackColorsPalette_p, PartyColors_p,
    OceanColors_p
};


// Length of hsvPalettes[]
const uint16_t hsvPalettesLength = (
    sizeof(hsvPalettes) / sizeof(hsvPalettes[0]));

// Length of rgbPalettes[]
const uint16_t rgbPalettesLength = (
    sizeof(rgbPalettes) / sizeof(rgbPalettes[0]));


// =================
// Utility Functions
// =================


uint16_t generate_entropy() {
    return (analogRead(RANDOM_ANALOG_PIN) * 65535) / 1023;
}


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
 * There are NUM_GROUPS groups, and each group number corresponds to a value
 * over [0, 255]; this function returns the discrete value that corresponds
 * with a given LED's group. For example, if LED_GROUP_SIZE == 15, led
 * indexes [0, 14] would all return the same value, and the next hue starts at
 * led index 15.
 *
 * Pre: 0 <= led < NUM_LEDS
 * Returns: 0 <= groupHue <= 255
 */
uint8_t getGroupHue(uint16_t led) {
    return ((led / LED_GROUP_SIZE) * 255) / (NUM_GROUPS - 1);
}


void setupOrbIndexes() {
    uint8_t orbSizeTypes[] = {XL_ORB_LEDS, L_ORB_LEDS, M_ORB_LEDS, S_ORB_LEDS};

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

    uint8_t currentLED = 0;
    uint8_t currentOffset = 0;
    uint8_t increment;
    for (uint8_t i = 0; i < NUM_ORBS; ++i) {
        increment = orbSizeTypes[currentOffset];
        orbIndexes[i] = currentLED;
        orbSizes[i] = increment;
        currentLED += increment;
        if (currentLED >= orbOffsets[currentOffset]) {
            ++currentOffset;
        }
    }
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
    static const uint64_t animationTimeMillis = 50;
    static uint64_t lastTime = 0;
    static uint16_t offset = 0;

    uint64_t currentTime = millis();

    fadeToBlackBy(leds, NUM_LEDS, 31);

    if (lastTime + animationTimeMillis <= currentTime) {
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
        lastTime = currentTime;
    }
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
    static const uint64_t animationTimeMillis = 75;
    static const uint16_t maxDist = (
        IS_GROUP_SIZE_EVEN ? GROUP_CENTER - 1: GROUP_CENTER);

    static uint64_t lastTime = 0;
    static uint16_t dist = 0;
    static boolean goingOut = true;

    uint64_t currentTime = millis();
    uint16_t start = maxDist - dist;

    if (lastTime + animationTimeMillis <= currentTime) {
        for (uint16_t i = 0; i < NUM_LEDS; i += LED_GROUP_SIZE) {
            for (uint16_t j = i + start;
                    j <= i + GROUP_CENTER + dist && j < NUM_LEDS; ++j) {
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
                                rgbPalettes[currentRGBPalette],
                                getGradientHue(j), DEFAULT_BRIGHTNESS);
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
        lastTime = currentTime;
    }
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


void flowerOrbPattern(AnimationType animType) {
    static const uint64_t animationTimeMillis = 150;
    static uint64_t lastTime = 0;
    static uint8_t ringNum = NUM_RINGS - 1;

    uint64_t currentTime = millis();

    fadeToBlackBy(leds, NUM_LEDS, 15);

    if (lastTime + animationTimeMillis <= currentTime) {
        uint8_t startOrb = ringIndexes[ringNum];
        uint8_t endOrb = startOrb + ringSizes[ringNum];

        for (uint8_t i = startOrb; i < endOrb; ++i) {
            uint16_t startLED = orbIndexes[i];
            uint16_t endLED = startLED + orbSizes[i];
            for (uint8_t j = startLED; j < endLED; ++j) {
                switch(animType) {
                    case GROUP_ANIM:
                        leds[j] = ColorFromPalette(
                            rgbPalettes[currentRGBPalette],
                            getGroupHue(ringNum * LED_GROUP_SIZE),
                            MAX_BRIGHTNESS);
                        break;
                    case RAINBOW_ANIM:
                        leds[j] = ColorFromPalette(
                            rgbPalettes[currentRGBPalette], rainbowHue,
                            MAX_BRIGHTNESS);
                        break;
                    case RGB_PALETTE_ANIM:
                    default:
                        leds[j] = ColorFromPalette(
                            rgbPalettes[currentRGBPalette], getGradientHue(j),
                            MAX_BRIGHTNESS);
                    break;
                }
            }
        }

        --ringNum;
        if (startOrb == 0) {
            ringNum = NUM_RINGS - 1;
        }
        lastTime = currentTime;
    }
}


void flowerOrbRGBPattern() {
    flowerOrbPattern(RGB_PALETTE_ANIM);
}

void flowerOrbGroupPattern() {
    flowerOrbPattern(GROUP_ANIM);
}

void flowerOrbRainbowPattern() {
    flowerOrbPattern(RAINBOW_ANIM);
}


/*
 * Pattern that goes through ChezCargot Colors with glittering lights and
 * "morphing" like quality.
 */
void glitterPattern() {
    uint8_t bpm = 30;
    uint8_t beat = beatsin8(bpm, 127, MAX_BRIGHTNESS);
    CRGBPalette16 palette = ChezCargotColorsPalette_p;

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

    CRGBPalette16 palette = ChezCargotColorsPalette_p;

    for(uint16_t i = 0; i < NUM_LEDS; ++i) {
        uint8_t brightness = beat - (rainbowHue + (i * 8));
        leds[i] = ColorFromPalette(
            palette, slowBeat - (rainbowHue + (i * 2)), brightness);
    }
}


void randomOrbsPattern(AnimationType animType) {
    static const uint64_t animationTimeMillis = 175;
    static uint64_t lastTime;
    static uint8_t lastOrb = 0;

    uint64_t currentTime = millis();

    fadeToBlackBy(leds, NUM_LEDS, 3);

    if (lastTime + animationTimeMillis <= currentTime) {
        uint8_t orb = random8(NUM_ORBS);
        while (orb == lastOrb) {
            orb = random8(NUM_ORBS);
        }
        uint16_t startLED = orbIndexes[orb];
        uint16_t endLED = startLED + orbSizes[orb];

        for (uint16_t i = startLED; i < endLED; ++i) {
            switch (animType) {
                case RAINBOW_ANIM:
                    leds[i] += CHSV(
                        rainbowHue + random8(64), DEFAULT_SAT,
                        MAX_BRIGHTNESS);
                    break;
                case GROUP_ANIM:
                    leds[i] += CHSV(
                        getGroupHue(startLED) + random8(64), DEFAULT_SAT,
                        MAX_BRIGHTNESS);
                    break;
                case RGB_PALETTE_ANIM:
                default:
                    leds[i] = ColorFromPalette(
                        rgbPalettes[currentRGBPalette], getGradientHue(i),
                        MAX_BRIGHTNESS);
                    break;
            }
        }
        lastOrb = orb;
        lastTime = currentTime;
    }
}

void randomOrbsRainbowPattern() {
    randomOrbsPattern(RAINBOW_ANIM);
}

void randomOrbsGroupPattern() {
    randomOrbsPattern(GROUP_ANIM);
}

void randomOrbsRGBPalettePattern() {
    randomOrbsPattern(RGB_PALETTE_ANIM);
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
    randomSparklesPattern(RAINBOW_ANIM);
}


void ringsPattern(AnimationType animType) {
    uint8_t phaseOffset;

    for (uint8_t ringNum = 0; ringNum < NUM_RINGS; ++ringNum) {
        uint8_t startOrb = ringIndexes[ringNum];
        uint8_t endOrb = startOrb + ringSizes[ringNum];
        for (uint8_t i = startOrb; i < endOrb; ++i) {
            uint16_t startLED = orbIndexes[i];
            uint16_t endLED = startLED + orbSizes[i];
            for (uint8_t j = startLED; j < endLED; ++j) {
                phaseOffset = ringNum * 32;
                switch(animType) {
                    case RGB_PALETTE_ANIM:
                        leds[j] = ColorFromPalette(
                            rgbPalettes[currentRGBPalette],
                            rainbowHue + (ringNum * 16),
                            beatsin8(60, 0, MAX_BRIGHTNESS, 0, phaseOffset));
                        break;
                    case RAINBOW_ANIM:
                    default:
                        leds[j] = ColorFromPalette(
                            rgbPalettes[currentRGBPalette],
                            rainbowHue,
                            beatsin8(60, 0, MAX_BRIGHTNESS, 0, phaseOffset));
                        break;
                }
            }
        }
    }
}

void ringsRainbowPattern() {
    ringsPattern(RAINBOW_ANIM);
}


void ringsRGBPattern() {
    ringsPattern(RGB_PALETTE_ANIM);
}


void spiralPattern(boolean dynamicDelay, boolean inAndOut) {
    static uint8_t orbNum = NUM_ORBS - 1;
    static const uint64_t steadyAnimTime = 100;
    static uint64_t animationTimeMillis = steadyAnimTime;
    static uint64_t lastTime = 0;
    static boolean inwards = false;

    if (dynamicDelay) {
        animationTimeMillis = beatsin8(10, 50, 125);
    } else {
        animationTimeMillis = steadyAnimTime;
    }

    uint64_t currentTime = millis();

    fadeToBlackBy(leds, NUM_LEDS, 3);

    if (lastTime + animationTimeMillis <= currentTime) {
        uint16_t startLED = orbIndexes[orbNum];
        uint16_t endLED = startLED + orbSizes[orbNum];
        for (uint16_t i = startLED; i < endLED; ++i) {
            leds[i] = ColorFromPalette(
                rgbPalettes[currentRGBPalette], getGradientHue(i),
                DEFAULT_BRIGHTNESS);
        }
        if (inwards) {
            ++orbNum;
            if (endLED == NUM_LEDS) {
                if (inAndOut) {
                    // We flip direction and start going outward (no repeat on
                    // last orb)
                    inwards = false;
                    orbNum = NUM_ORBS - 2;
                } else {
                    // We start from the beginning
                    orbNum = 0;
                }
            }

        } else {
            --orbNum;
            if (startLED == 0) {
                if (inAndOut) {
                    // We flip direction and start heading inward
                    inwards = true;
                    orbNum = 1;
                } else {
                    // We start from the end (center)
                    orbNum = NUM_ORBS - 1;
                }
            }
        }
        lastTime = currentTime;
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
 *
 * Each should be the name of a function that has return type void and takes no
 * parameters. Some patterns are repeated in order to increase their chances of
 * showing up when the next pattern is randomly chosen.
 */
PatternArray patterns = {
    beatSyncMultiplesPattern,
    beatSyncMultiplesPattern,
    cometPatternHSV,
    cometPatternRGB,
    convergePatternHSV,
    convergePatternRGB,
    flowerOrbGroupPattern,
    flowerOrbRGBPattern,
    flowerOrbRainbowPattern,
    glitterPattern,
    glitterPattern,
    pulsingPattern,
    pulsingPattern,
    randomOrbsGroupPattern,
    randomOrbsRGBPalettePattern,
    randomOrbsRainbowPattern,
    randomSparklesGroupPattern,
    randomSparklesRainbowPattern,
    ringsRGBPattern,
    ringsRainbowPattern,
    spiralDynamicInAndOutPattern,
    spiralPatternDynamic,
    spiralPatternSteady,
    spiralSteadyInAndOutPattern,
    twinklePattern,
    twinklePattern
};


// Length of patterns[]
const uint16_t patternsLength = sizeof(patterns) / sizeof(patterns[0]);


// ========================
// State Changing Functions
// ========================

/*
 * Randomly changes the index on both the HSV and RGB palette arrays. This
 * should be called in a EVERY_N_SECONDS macro provided by FastLED in order
 * to dynamically change color palettes in patterns.
 */
void nextPalette() {
    uint8_t newRGBPalette = random8(rgbPalettesLength);
    while (newRGBPalette == currentRGBPalette) {
        newRGBPalette = random8(rgbPalettesLength);
    }
    currentRGBPalette = newRGBPalette;

    uint8_t newHSVPalette = random8(hsvPalettesLength);
    while (newHSVPalette == currentHSVPalette) {
        newHSVPalette = random8(hsvPalettesLength);
    }
    currentHSVPalette = newHSVPalette;
}


/*
 * Randomly changes the index of the patterns array in order to switch to the
 * next pattern. This should be called in an EVERY_N_SECONDS macro provided by
 * FastLED in order to dynamically change patterns.
 */
void nextPattern() {
    uint8_t newPattern = random8(patternsLength);
    while (newPattern == currentPattern) {
        newPattern = random8(patternsLength);
    }
    currentPattern = newPattern;
}


// ======================
// Main Arduino Functions
// ======================


void setup() {
    // Add some entropy to initial random FastLED seed
    random16_add_entropy(generate_entropy());

    // Initialize orb indexes
    setupOrbIndexes();

    // Initialize starting patterns and palettes
    currentPattern = random8(patternsLength);
    currentHSVPalette = random8(hsvPalettesLength);
    currentRGBPalette = random8(rgbPalettesLength);

    // Initialize the LED Strip.
    // Uses Implicit SPI Pins From FastLED
    FastLED.addLeds<APA102, RGB_ORDER>(
        leds, NUM_LEDS).setCorrection(TypicalSMD5050);

    // For Use With Explicit Clock and Data
    /* FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB_ORDER>( */
    /*     leds, NUM_LEDS).setCorrection(TypicalSMD5050); */

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
