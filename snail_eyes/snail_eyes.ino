#include <FastLED.h>

// =======================
// Arduino Pin Definitions
// =======================

// Dotstar LED Pins
#define CLOCK_PIN 11  // Pin for LED Clock (Dotstar)
#define CLOCK_PIN2 11
#define DATA_PIN 13   // Pin For LED Data (Dotstar)
#define DATA_PIN2 9

// Number of LED's On Strip (NUM_LEDS % 12 should == 0)
#define NUM_LEDS 192
//#define NUM_LEDS (4*12)
#define NUM_STRIPS 2

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

// Size of a "group" for certain animations.
#define LED_GROUP_SIZE (NUM_LEDS / 16)

// ========================
// Time Syncing Definitions
// ========================

#define BPM 122             // Beat Synced BPM
#define DEBOUNCE_DELAY 200  // Amount of time to wait for debouncing checks
#define LED_OFF_DELAY 750   // Amount of time to wait before turning off
                            // onboard LED's.
#define FPS 120             // Frames Per Second

// Amount of time to show each pattern
#define PATTERN_SECS 16

// Amount of time to show each palette
#define PALETTE_SECS (PATTERN_SECS / 4)

// Amount of time between each hue in rainbow
#define RAINBOW_MILLIS 20

// The LED's, man!
CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS];

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

uint64_t lastDebounceTime = 0;      // Time used for debouncing mode button
uint64_t lastPatternSwitchTime = 0;       // Time used for Pattern Switching LED
uint64_t lastModeSwitchTime = 0;    // Time used for Mode Switching LED


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
 * Pattern that lights up whole octacat with a gradient of a single color, and slowly rotates that gradient
 */
void slowRotatePattern() {
    static uint8_t head = 0;
    CHSV hsv = CHSV(rainbowHue, DEFAULT_SAT, MAX_BRIGHTNESS);
    CHSV hsv2 = CHSV(255 - rainbowHue, DEFAULT_SAT, MAX_BRIGHTNESS);
    CHSV hsv3 = CHSV((rainbowHue + 125) % 255, DEFAULT_SAT, MAX_BRIGHTNESS);
    CHSV hsv4 = CHSV(255 - ((rainbowHue + 125) % 255), DEFAULT_SAT, MAX_BRIGHTNESS);
    
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      uint8_t curr = (head + i) % NUM_LEDS;
      CHSV color = (i < NUM_LEDS/2) ? hsv : hsv2;
      CHSV color2 = (i < NUM_LEDS/2) ? hsv3 : hsv4;
      color.val = i % (NUM_LEDS/2) + 155;
      color2.val = i % (NUM_LEDS/2) + 155;
      leds[curr] = color;
      leds2[curr] = color2;
    }
    delay(10);

    head = scale8(beat8(BPM*12), NUM_LEDS);

    uint8_t bright = beatsin8(BPM/2, MIN_BRIGHTNESS * 3 / 4, MAX_BRIGHTNESS);
    FastLED.setBrightness( bright );
//
//    Serial.print("current brightness is: ");
//    Serial.println(bright);

}

/*
 * Pattern which has multiple trails of LED "comets" moving along the strip.
 */
void cometPattern(boolean hsvColors, boolean flashWholeBox) {
    static uint8_t offset = 0;
    static uint8_t prevPalette = currentRGBPalette;
    static uint8_t flashHue = random(256);
    if(prevPalette != currentRGBPalette) {
      prevPalette = currentRGBPalette;
      flashHue = random(256);
    }
 
    fadeToBlackBy(leds, NUM_LEDS, 127);
    fadeToBlackBy(leds2, NUM_LEDS, 127);

    // flash everything on the beat, all leds lit a varying shade of brightness based on the offset
    if(flashWholeBox) {
      if(offset < LED_GROUP_SIZE * 1 / 4 || offset > LED_GROUP_SIZE * 3 / 4) {
        for (uint16_t i = 0; i < NUM_LEDS; i++) {
          uint8_t brightness  = beatsin8(BPM*2, DEFAULT_BRIGHTNESS, DEFAULT_BRIGHTNESS);
          CHSV backgroundColor = CHSV(flashHue, MAX_SAT, brightness);
          leds[i] = backgroundColor;
          leds2[i] = backgroundColor;
        }
      }
    }

    // otherwise draw the head of the comet in each group (and the above fadeToBlackBy() calls will fade the previous heads)
    for (uint16_t i = offset; i < NUM_LEDS; i += LED_GROUP_SIZE) {
        if (hsvColors) {
            CHSV hsv = ColorFromPalette(
                hsvPalettes[currentHSVPalette],
                getGradientHue(i), MAX_BRIGHTNESS);
            hsv.hue += 16 * offset;
            leds[i] = hsv;
            leds2[i] = hsv;
        } else {
            leds[i] = ColorFromPalette(
                rgbPalettes[currentRGBPalette], getGradientHue(i),
                MAX_BRIGHTNESS);
            leds2[i] = ColorFromPalette(
                rgbPalettes[currentRGBPalette], getGradientHue(i),
                MAX_BRIGHTNESS);
        }
    }

    uint8_t rate = BPM;
    if(flashWholeBox) {
      rate = BPM/2; // do half beat to be less distracting
    }
    // beat8() gives us a sawtooth wave from 0-255 in our BPM, and we scale that to 0-16,
    // so the offset will reset to the beginning of an edge of the octocat on the beat (122 bmp)
    offset = scale8(beat8(rate), LED_GROUP_SIZE);

    delay(10);
}


/*
 * See cometPattern, uses RGB Palettes.
 */
void cometPatternRGB() {
    cometPattern(false, false);
}


/*
 * See cometPattern, uses HSV Palettes.
 */
void cometPatternHSV() {
    cometPattern(true, false);
}

void cometFlashHSV() {
  cometPattern(false, true);
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
                    leds2[j] = hsv;
                } else {
                    leds[j] = ColorFromPalette(
                        rgbPalettes[currentRGBPalette], getGradientHue(j),
                        DEFAULT_BRIGHTNESS);
                    leds2[j] = ColorFromPalette(
                        rgbPalettes[currentRGBPalette], getGradientHue(j),
                        DEFAULT_BRIGHTNESS);
                }
            } else {
                leds[j] = CRGB::Black;
                leds2[j] = CRGB::Black;
            }
        }
    }

    dist = (++dist) % (maxDist + 1);
    if (dist == 0) {
        goingOut = !goingOut;
    }
    delay(65);
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

void rainbowMe() {
  // FastLED's built-in rainbow generator
  uint8_t ledStart = beatsin8(BPM / 2, 0, LED_GROUP_SIZE);
  fill_rainbow( &leds[ledStart], NUM_LEDS, getGroupHue(0), 10);
  fill_rainbow( &leds2[ledStart], NUM_LEDS, 255 - getGroupHue(0), 10);
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
    slowRotatePattern,
    rainbowMe,
    cometPatternHSV,
    cometFlashHSV,
    convergePatternRGB,
    cometPatternRGB
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
    lastPatternSwitchTime = millis();
}

void turnOffLights() {
  // Turn Off The Lights
  FastLED.clear();
  FastLED.show();
}


void countUp(int n, int color) {
  turnOffLights();
  CHSV randomColor = CHSV(random(256), random(MIN_SAT, MAX_SAT), DEFAULT_BRIGHTNESS);
  for (int i = 0; i < n; i++) {
    leds[i] = color;
    if(NUM_STRIPS > 1) {
      leds2[i] = randomColor;
    }
    FastLED.show();
    delay(10);
  }
}


// ======================
// Main Arduino Functions
// ======================

void setup() {
    // Initialize the LED Strips.
    FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR, DATA_RATE_MHZ(7)>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
    if(NUM_STRIPS > 1) {
      FastLED.addLeds<APA102, DATA_PIN2, CLOCK_PIN2, BGR, DATA_RATE_MHZ(7)>(leds2, NUM_LEDS).setCorrection(TypicalSMD5050);
    }

    // Set the color temperature
    FastLED.setTemperature(CarbonArc);

    // Set the global brightness
    FastLED.setBrightness(DEFAULT_BRIGHTNESS);

    Serial.begin(9600);
}


void loop() {
    // Do an action based on the current mode
    
    patterns[currentPattern]();

    FastLED.show(); // Show the LED's
    FastLED.delay(1000 / FPS);  // Add a global delay at the frame rate.

    // Switch patterns every PATTERN_SECS
    EVERY_N_SECONDS(PATTERN_SECS) {
      Serial.print("running pattern: ");
      Serial.println(currentPattern); 
      nextPattern(); 
    }

    // Switch palettes every PALETTE_SECS
    EVERY_N_SECONDS(PALETTE_SECS) { nextPalette(); }

    // Increment the "rainbow hue" every RAINBOW_MILLIS milli's.
    EVERY_N_MILLISECONDS(RAINBOW_MILLIS) { 
      ++rainbowHue;
      rainbowHue %= 255;
    }
}
