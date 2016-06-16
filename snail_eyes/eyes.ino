#include <FastLED.h>

const uint8_t CLOCK_PIN1 = 13;
const uint8_t CLOCK_PIN2 = 10;
const uint8_t CLOCK_PIN3 = 8;
const uint8_t DATA_PIN1 = 11;
const uint8_t DATA_PIN2 = 9;
const uint8_t DATA_PIN3 = 7;
const uint8_t MOTION_SENSOR_PIN = 3;

const uint16_t NUM_LEDS = (8 * 30 + 4);
//const uint16_t NUM_LEDS = 153;
//const uint16_t NUM_LEDS = 15;

const uint8_t DEFAULT_HUE = 5;
const uint8_t DEFAULT_BRIGHTNESS = 127;
const uint8_t DEFAULT_SAT = 191;
const uint8_t LED_GROUP_SIZE = 10;
const uint8_t MAX_BRIGHTNESS = 223;
const uint8_t MAX_SAT = 255;
const uint8_t MIN_BRIGHTNESS = 63;
const uint8_t MIN_SAT = 191;
const uint8_t NUM_HUE_DISTANCES = 6;
const uint8_t PULSE_LENGTH = 9;
const uint8_t ZIGZAG_LENGTH = 16;
const uint8_t SEGMENT_LENGTH = 40;
const uint8_t BLINK_LENGTH = 40;

const uint8_t LEFT_CENTER = (
    LED_GROUP_SIZE % 2 == 0 ? (LED_GROUP_SIZE / 2) - 1 : LED_GROUP_SIZE / 2);
const uint8_t RIGHT_CENTER = LED_GROUP_SIZE / 2;
const uint8_t DIST_CENTER = (
    LED_GROUP_SIZE % 2 == 0 ? LED_GROUP_SIZE / 2 : (LED_GROUP_SIZE / 2) + 1);

const uint8_t HUE_DISTANCES[NUM_HUE_DISTANCES] = {0, 127, 42, 170, 234, 191};

CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS];
CRGB leds3[NUM_LEDS];
CHSV hsvs[NUM_LEDS];
CHSV hsvs2[NUM_LEDS];
CHSV hsvs3[NUM_LEDS];
byte temp[NUM_LEDS];

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
    // Turn Off The Lights
    for (uint16_t i = 0; i < NUM_LEDS; ++i) {
      leds[i] = 0;
      leds2[i] = 0;
      leds3[i] = 0;
    }
    FastLED.show();
}

void hsv2rgbAll() {
  hsv2rgb_rainbow(hsvs, leds, NUM_LEDS);
  hsv2rgb_rainbow(hsvs2, leds2, NUM_LEDS);
  hsv2rgb_rainbow(hsvs3, leds3, NUM_LEDS);
}

void setHSVAllStrips(uint16_t pixel, CHSV color) {
  hsvs[pixel] = color;
  hsvs2[pixel] = color;
  hsvs3[pixel] = color;
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
            setHSVAllStrips(j, CHSV(hue, DEFAULT_SAT, 0));
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
            hsv2rgbAll();
            FastLED.show();
            delay(5);
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
                hsvs[j + i].sat = MAX_SAT;
                hsvs[j + i].val = DEFAULT_BRIGHTNESS;
                for (uint8_t k = j; k < j + i; ++k) {
                    hsvs[k].val = lerp8by8(0, hsvs[k].val, 128);
                }
            }
            hsv2rgbAll();
            FastLED.show();
            delay(50);
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
        hsv2rgbAll();
        FastLED.show();
        delay(100);
    }
}

// TODO: Add modulation to H, S, or V
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
        hsv2rgbAll();
        FastLED.show();
        delay(100);
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
    hsv2rgbAll();
    FastLED.show();
    // Right
    pingPongSide(right, LED_GROUP_SIZE - 1, 1);
    // Left
    pingPongSide(left, 0, -1);
}

// TODO: Add modulation to H, S, or V
void pingPongLoop(uint8_t numLoops) {
    setupHuesForGroups();
    for (uint8_t i = 0; i < numLoops; ++i) {
        pingPongAnimation(LEFT_CENTER, RIGHT_CENTER);
    }
}

void rainbowRace() {
  CHSV pulse_color = CHSV(random(256), random(MIN_SAT, MAX_SAT), DEFAULT_BRIGHTNESS);
  CHSV base_color = CHSV(pulse_color.hue + 128 % 256, pulse_color.sat, DEFAULT_BRIGHTNESS);
  CHSV off = CHSV(0, 0, 0);
  uint8_t hue;
  uint16_t led;
  uint8_t brightness = MIN_BRIGHTNESS;
  uint8_t NUM_LOOPS = 14;
  uint8_t dx = (MAX_BRIGHTNESS - MIN_BRIGHTNESS) * 2 / SEGMENT_LENGTH / NUM_LOOPS;
  for (uint8_t loop = 0; loop < NUM_LOOPS; loop++) {
    for (uint16_t i = 0; i < SEGMENT_LENGTH; ++i) {
      brightness += dx;
      if(brightness > MAX_BRIGHTNESS || brightness < MIN_BRIGHTNESS) {
        dx = -dx;
      }
      for (uint16_t head = i; head < NUM_LEDS + SEGMENT_LENGTH; head += SEGMENT_LENGTH) {
        for(uint16_t k = 0; k < SEGMENT_LENGTH; ++k) {
          // Strip 1 and 3 have their head at 'head', and strip 2's head is head + PULSE_LENGTH
          led = head - k;
          if(led > 0 && led < NUM_LEDS) {
            bool within_pulse = k < PULSE_LENGTH;
            if(within_pulse) {
              hsvs[led] = base_color;
              hsvs2[(led + SEGMENT_LENGTH / 2) % NUM_LEDS] = pulse_color;
              hsvs3[led] = base_color;
            } else {
              hsvs[led] = off;
              hsvs2[(led + SEGMENT_LENGTH / 2) % NUM_LEDS] = off;
              hsvs3[led] = off;
            } 
          }
        }
      }
      hsv2rgbAll();
      FastLED.show();
      delay(1);
    }
  }
}

void growing_blinks(uint8_t color, uint8_t initial_block_size) {
  uint8_t block_size = initial_block_size;
//  
//    uint8_t pulse_color = base_color + 128 % 256;
//    uint8_t hue;
//    uint16_t led;
//    uint8_t brightness = MIN_BRIGHTNESS;
    uint8_t numloops = 4;
//    uint8_t dx = (MAX_BRIGHTNESS - MIN_BRIGHTNESS) * 2 / SEGMENT_LENGTH / NUM_LOOPS;

    for(uint8_t block_size = initial_block_size; block_size < BLINK_LENGTH*2; block_size*=2){
      for(uint16_t flash = 0; flash < numloops; ++flash) {
        for (uint16_t mid = BLINK_LENGTH/2; mid < NUM_LEDS + BLINK_LENGTH; mid += BLINK_LENGTH) {
          for(int k = mid-block_size/2; k < mid+block_size/2; ++k) {
            if(k >= 0 && k <= NUM_LEDS){
              hsvs[k] = CHSV(color, DEFAULT_SAT, MAX_BRIGHTNESS);
            }
          }
        }
        hsv2rgbAll();
        FastLED.show();
        delay(100);
        turnOffLights();
        delay(100);
    }
    numloops--;
  }
    turnOffLights();
//    delay(300);
}

void blinks(CHSV color, uint8_t block_size) {
//  
//    uint8_t pulse_color = base_color + 128 % 256;
//    uint8_t hue;
//    uint16_t led;
//    uint8_t brightness = MIN_BRIGHTNESS;
//    uint8_t dx = (MAX_BRIGHTNESS - MIN_BRIGHTNESS) * 2 / SEGMENT_LENGTH / NUM_LOOPS;

  uint8_t numloops = 2;
      for(uint16_t flash = 0; flash < numloops; ++flash) {
        turnOffLights();
        delay(100);
        for (uint16_t head = 0; head < NUM_LEDS; head += BLINK_LENGTH) {
          for(int k = head; k > head-block_size; --k) {
            if(k >= 0 && k <= NUM_LEDS){
              hsvs[k] = color;
            }
          }
        }
        hsv2rgbAll();
        FastLED.show();
        delay(100);
  }
//    turnOffLights();
    delay(300);
}

void rotate(){
  hsvs[0] = hsvs[NUM_LEDS-1];
  for(uint16_t i = NUM_LEDS-1; i > 0; i--){
    hsvs[i] = hsvs[i-1];
  }  
}

void inverseRotate(){
  for(uint16_t i = 0; i < NUM_LEDS; i++){
    hsvs[i] = hsvs[i+1];
  } 
  hsvs[NUM_LEDS-1] = hsvs[0];
}

uint16_t repeatingPattern(uint16_t hues[], uint8_t lengths[], uint8_t numHues){
  uint16_t totalLength = 0;
  for(uint16_t i = 0; i < numHues; i++){
    totalLength += lengths[i]; 
  }
  for(uint16_t i = 0; i < NUM_LEDS; i++){
    uint16_t moddedValue = i%(totalLength);
    uint16_t sum = 0;
    uint16_t hueIndex = 0;
    for(uint16_t i = 0; i < numHues; i++){
      if(moddedValue >= sum) {    
        hueIndex = i;
      } else {
        break;
      }
      sum += lengths[i];   
    }
    hsvs[i] = CHSV(hues[hueIndex], MAX_SAT, DEFAULT_BRIGHTNESS);  
  }
 return totalLength;
}

void bounce(uint16_t hues[], uint8_t lengths[], bool shouldBounce, uint8_t numHues){ 
  uint16_t totalLength = repeatingPattern(hues, lengths, numHues);
  for( uint16_t i = 0; i < totalLength; i++){
    rotate();
    hsv2rgbAll();
    FastLED.show(); 
    delay(400);
  }
  if(shouldBounce){
    for( uint16_t i = 0; i < totalLength; i++){
      inverseRotate();
      hsv2rgbAll();
      FastLED.show(); 
      delay(400);
    }
  }
}

void bounceColorChange(uint16_t hues[], uint8_t lengths[], bool shouldBounce, uint8_t numHues){
  for(uint8_t colorOffset = 0; colorOffset < 255; colorOffset++){
    for(uint8_t hue = 0; hue < numHues; hue++){
      hues[hue] = hues[hue] + colorOffset % 255;
    }
    bounce(hues, lengths, shouldBounce, numHues);
    if(colorOffset == 255) { colorOffset = 0; }
  } 
}


void rainbowScroll(uint16_t hueLength){
  uint8_t hues[hueLength];
  uint8_t NUM_LOOPS = 10;
  for(uint16_t i = 0; i < hueLength; i++){
    hues[i] = 255/hueLength * i;
  }
  for(uint8_t loopNum = 0; loopNum < NUM_LOOPS; loopNum++) {
    for(uint8_t offset = 0; offset < hueLength; offset++){
      for(uint16_t led = 0; led < NUM_LEDS; ++led){  
        hsvs[led] = CHSV(hues[(led - offset)%hueLength], MAX_SAT, MAX_BRIGHTNESS);  
        hsvs2[led] = CHSV(hues[(led - offset)%hueLength], MAX_SAT, MAX_BRIGHTNESS);  
        hsvs3[led] = CHSV(hues[(led - offset)%hueLength], MAX_SAT, MAX_BRIGHTNESS);  
      }
      hsv2rgbAll();
      FastLED.show(); 
      delay(50);
    }
  }
}

void zigzag() {
  CHSV pulse_color = CHSV(random(256), random(MIN_SAT, MAX_SAT), DEFAULT_BRIGHTNESS);
  CHSV base_color = CHSV(pulse_color.hue + 128 % 256, pulse_color.sat, DEFAULT_BRIGHTNESS);
  CHSV off = CHSV(0, 0, 0);
  uint8_t hue;
  uint16_t led;
  uint8_t NUM_LOOPS = 14;
  uint8_t numOnStrip1 = ZIGZAG_LENGTH;
  uint8_t dx = 1;
  for (uint8_t loop = 0; loop < NUM_LOOPS; loop++) {
    for (uint16_t i = 0; i < SEGMENT_LENGTH; ++i) {
      for (uint16_t head = i; head < NUM_LEDS + SEGMENT_LENGTH; head += SEGMENT_LENGTH) {
        for(uint16_t k = 0; k < SEGMENT_LENGTH; ++k) {
          led = head - k;
          if(led > 0 && led < NUM_LEDS) {
            bool within_pulse = k < PULSE_LENGTH;
            if(within_pulse) {
              if(k < PULSE_LENGTH / 3) {
                hsvs[led] = pulse_color;
                hsvs2[led] = off;
                hsvs3[led] = off;
              } else if(k < PULSE_LENGTH * 2/ 3) {
                hsvs[led] = off;
                hsvs2[led] = pulse_color;
                hsvs3[led] = off;
              } else {
                hsvs[led] = off;
                hsvs2[led] = off;
                hsvs3[led] = pulse_color;
              }
            } else {
              hsvs[led] = off;
              hsvs2[led] = off;
              hsvs3[led] = off;
            } 
          }
        }
      }
      hsv2rgbAll();
      FastLED.show();
      delay(1);
    }
  }
}



void lightN(int n) {
  turnOffLights();
  for(int i = 0; i < n; i++) {
    leds[i] = (255, 255, 255);
  }
  FastLED.show(); 
}

void setup() {
    Serial.begin(9600);
    FastLED.addLeds<APA102, DATA_PIN1, CLOCK_PIN1>(leds, NUM_LEDS);
    FastLED.addLeds<APA102, DATA_PIN2, CLOCK_PIN2>(leds2, NUM_LEDS);
    FastLED.addLeds<APA102, DATA_PIN3, CLOCK_PIN3>(leds3, NUM_LEDS);
    setup_hue_array();
    pinMode(MOTION_SENSOR_PIN, INPUT);
}


byte wave;
int wave_scale = 20;

void wave_fx(){
  for(uint16_t tick = 0; tick < 2000; tick++) {
    //define the middle of the strip
    static byte middle = NUM_LEDS/2;
    
    for (byte i = 0; i < middle; i++){
      //this is the function that creates the wave effect
      //it may take time to fully understand what is happening here
      wave = sin8((millis() * 2 / wave_scale) * i);
      
      //populate the leds[x] with the temp[x] array hue's
      leds[i] = CHSV(temp[i], DEFAULT_SAT, wave);
      leds2[i] = CHSV(temp[i], DEFAULT_SAT, wave);
      leds3[i] = CHSV(temp[i], DEFAULT_SAT, wave);
    }
    //now mirror the data from leds #0 thru #middle
    //to leds #middle thru #end of string
    for(byte i = middle; i < NUM_LEDS; i++){
      leds[i] = leds[NUM_LEDS - i];
      leds2[i] = leds[NUM_LEDS - i];
      leds3[i] = leds[NUM_LEDS - i];
    }
    FastLED.show();
    delay(1);
  }
}

void setup_hue_array(){
  static byte x, hue = 0;
  
  //only assign data for half the led strip
  for(byte i = 0; i < ((NUM_LEDS/2)+ 1); i++){
    
    //for every 10 leds, increase the hue by 30
    if(x++ == 10){
      hue += 30;
      x = 0;   
    }
    
    //assign the hue for later use in the temp[] array
    temp[i] = hue;
  }
}

int prevState = LOW;
uint8_t sketch = 0;
uint8_t NUM_SKETCHES = 4;

void doNextSketch() {
  if(sketch == 0) {
    wave_fx();
  } else if(sketch == 1) {
    zigzag();
  } else if(sketch == 2) {
    rainbowRace();
  } else if(sketch == 3) {
    rainbowScroll(7);
//      bounceColorChange(hues, lengths, true, numHues);
  }
  sketch++;
  sketch %= NUM_SKETCHES;
  delay(3);
  turnOffLights();
  delay(3);
}


void loop() {
//  uint8_t NUM_HUES = 4;
//  uint16_t hues[] =  {40, 120, 160, 210};
//  uint8_t lengths[] =  {5, 3, 5, 3};
//  lightN(NUM_LEDS);
//  delay(5000);
//  turnOffLights();
//  delay(5000);
  doNextSketch();
//  int val = digitalRead(MOTION_SENSOR_PIN);
//  if(val == HIGH && prevState == LOW) {
//    prevState = HIGH;
//    doNextSketch();
//  } else if(val != HIGH) {
//    prevState = LOW;
//  }
}

