
#include <avr/sleep.h>
#include <RF24.h>
#include <FastLED.h>
#include <EEPROM.h>
#include "timer.h"


//----------------------------------------------------------------------------------
//                                                               General Definitions
//----------------------------------------------------------------------------------

#define NUM_LEDS             99
#define SYMMETRY              2
#define CENTER              NUM_LEDS / 2

#define FPS                  60
#define INITIAL_BRIGHTNESS  200
#define PATTERN_CYCLE_TIME  12000
//#define CHIPSET             APA102
#define CHIPSET             WS2812
#define COLOR_ORDER         GRB

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

//----------------------------------------------------------------------------------
//                                                   Controller Specific Definitions
//----------------------------------------------------------------------------------

// uncomment just one of these
#define LED_KEYCHAIN

#ifdef LED_KEYCHAIN
#define DATA_PIN       11
#define BUTTON1_PIN   3
#define BUTTON2_PIN   2
#define BUTTON3_PIN   0
#define BUTTON4_PIN   1
#define BUTTON_POWER  BUTTON1_PIN
#define BUTTON_DOWN   BUTTON2_PIN
#define BUTTON_UP     BUTTON3_PIN
#define BUTTON_MODE   BUTTON4_PIN
#define LED_EN_PIN 5  // PC6
#define USB_POWERED
#endif



//----------------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------------

uint8_t g_brightness = INITIAL_BRIGHTNESS;
uint32_t g_now = 0;
uint8_t g_hidden = 0;

CRGB leds[NUM_LEDS];
CRGB * leds2 = (leds + (NUM_LEDS/2));

MillisTimer pattern_timer;



//----------------------------------------------------------------------------------
// Patterns
//----------------------------------------------------------------------------------

#include "blinkypants_patterns.h"
#include "fastled_patterns.h"
#include "tinybee_patterns.h"
#include "artemiid_patterns.h"
#include "Fire2012.h"

//----------------------------------------------------------------------------------
// Mode / state
//----------------------------------------------------------------------------------


typedef int (*SimplePatternList[])();
uint8_t g_current_pattern = 0;


SimplePatternList patterns = {
  //fire2012_loop,
  Fire2012WithPalette,
  //collision,
  moving_palette,
  //rgb_calibrate_loop,
  rainbow,
  //sinelon,
  confetti,
  //mode_yalda,
  sinelonN,
  plasma,
  //fader_loop1,
  //fader_loop2,
  //fader_loop3,
};


void next_pattern()
{
    g_current_pattern = (g_current_pattern + 1) % ARRAY_SIZE(patterns);
    write_state();
    FastLED.clear();
}

void enable_autocycle() {
    if (pattern_timer.running()) return;    
    pattern_timer.start(PATTERN_CYCLE_TIME, true);
    next_pattern();  
}

void disable_autocycle() {
    if (!pattern_timer.running()) return;
    pattern_timer.stop();
    write_state();
}

void mode_button() {
    if (pattern_timer.running()) {
      disable_autocycle();
    } else {
      next_pattern();
    }
}

void brightness_up() {
 switch (g_brightness) {
      case 0 ... 15: g_brightness += 1; break;
      case 16 ... 100: g_brightness += 15; break;
      case 101 ... (0xff - 30): g_brightness += 30; break;
      case (0xff - 30 + 1) ... 254: g_brightness = 255; break;
      case 255: break;
  } 
}
void brightness_up_smooth() { if (g_brightness < 255) {g_brightness++;} }

void brightness_down() {
  switch (g_brightness) {
      case 0: break;
      case 1 ... 15: g_brightness -= 1; break;
      case 16 ... 100: g_brightness -= 15; break;
      case 101 ... 255: g_brightness -= 30; break;
  }            
}

void brightness_down_smooth() { if (g_brightness > 0) {g_brightness--;} }

void hidden_up() { g_hidden++; Serial.print("Hidden: "); Serial.println(g_hidden);}
void hidden_down() { g_hidden--; Serial.print("Hidden: "); Serial.println(g_hidden);}

void read_state() {
    uint8_t buffer = 0;
    // autocycle
    EEPROM.get(0, buffer);
    if (buffer) {
        pattern_timer.start(PATTERN_CYCLE_TIME, true);
    }
    
    // current pattern
    EEPROM.get(1, buffer);
    if (buffer == 255) {
      g_current_pattern = 0;
    } else {
      g_current_pattern = buffer % ARRAY_SIZE( patterns);
    }

    // brightness
    EEPROM.get(2, buffer);
    if (buffer == 255) {
      g_brightness = INITIAL_BRIGHTNESS;
    } else {
      g_brightness = buffer;
    }
    
    // hidden
    EEPROM.get(3, buffer);
    if (buffer == 255) {
      g_hidden = 0;
    } else {
      g_hidden = buffer;
    }
    
}

void write_state() {
    EEPROM.update(0, pattern_timer.running());
    EEPROM.update(1, g_current_pattern);
    EEPROM.update(2, g_brightness);
    EEPROM.update(3, g_hidden);
}

//----------------------------------------------------------------------------------
// Setup & loop
//----------------------------------------------------------------------------------


#include "btn.h"
Btn btn_mode(BUTTON_MODE, &mode_button, &enable_autocycle);
Btn btn_up(BUTTON_UP, &brightness_up, &brightness_up_smooth);
Btn btn_down(BUTTON_DOWN, &brightness_down, &brightness_down_smooth);



void setup() {

  Serial.begin(9600);
  
   // Leds
  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  #ifdef USB_POWERED
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000);
  #endif

  // Keychain Setup
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(LED_EN_PIN, OUTPUT);
  digitalWrite(LED_EN_PIN, LOW);
  delay(2000);
  digitalWrite(LED_EN_PIN, HIGH);

  read_state();
}



void loop() {
  g_now = millis();
  random16_add_entropy( random());

  //Serial.print("Time: ");
  //Serial.println(g_now);

  
  // Poll all our Buttons

  // Holding MODE while pressing UP or DOWN adjusts the hidden variable
  bool bumped = false;
  if (btn_mode.pressed()) { 
    btn_up.poll(&hidden_up, &hidden_up);
    btn_down.poll(&hidden_down, &hidden_down);
    if (btn_up.last_action() > btn_mode.last_action()) { btn_mode.bump(); bumped = true;}
    if (btn_down.last_action() > btn_mode.last_action()) { btn_mode.bump(); bumped = true;}
  }

  if (not bumped) {
    btn_mode.poll();
    btn_up.poll();
    btn_down.poll();
  }

    
  // Check for power off
  if (!digitalRead(BUTTON_POWER)) {
     delay(10);
     if (!digitalRead(BUTTON_POWER)) {
       leds_sleep();
     }
  }
  


  


  uint8_t brightness = g_brightness;
  if (pattern_timer.running()) {
    if (pattern_timer.sinceStart() < 300) {
        brightness = scale8(pattern_timer.sinceStart() * 256 / 300, g_brightness);
    }
    if (pattern_timer.untilDone() < 300) {
        brightness = scale8(pattern_timer.untilDone() * 256 / 300, g_brightness);
    }
  }
  FastLED.setBrightness(brightness);



  if (pattern_timer.fired()) {
    next_pattern();
  }

  uint32_t d = (patterns)[g_current_pattern]();


  FastLED.show();
  if (d == 0) d = 1000/FPS;
  FastLED.delay(d);
}
