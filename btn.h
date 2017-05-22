#define debounce 100 
#define holdTime 1000 

typedef void (*Trigger)();

class Btn {
    int buttonVal = 0; // value read from button
    int buttonLast = HIGH; // buffered value of the button's previous state
    long btnDnTime; // time the button was pressed down
    long btnUpTime; // time the button was released
    long lastHeld = 0;
    boolean ignoreUp = false; // whether to ignore the button release because the click+hold was triggered
    int buttonPin;

    Trigger t_pressed;
    Trigger t_held;
    //void (*trigger_held)();

public:
    Btn(int pin) : buttonPin(pin) {
      pinMode(pin, INPUT_PULLUP);
    }

    Btn(int pin, Trigger pressed, Trigger held) : //void(*pressed)(), void(*held)()) : 
        buttonPin(pin),
        t_pressed(pressed),
        t_held(held) {
      pinMode(pin, INPUT_PULLUP);
    }

    long last_action() { 
      return max(btnDnTime, max(btnUpTime, lastHeld));
    }

    // Something invisible happened, so if the button is still being held, pretend it was just pushed down
    void bump() { 
      if (pressed()) { 
        lastHeld = millis();
      }
    }

    bool pressed() {
      return buttonLast == LOW;
    }

    void poll() {
      poll(t_pressed, t_held);
    }
    
    void poll(void (*pressed)(), void (*held)()) {
        // Read the state of the button
        buttonVal = digitalRead(buttonPin);

        // Test for button pressed and store the down time
        if (buttonVal == LOW && buttonLast == HIGH && (millis() - btnUpTime) > long(debounce)) {
          btnDnTime = millis();
        }

        // Test for button release and store the up time
        if (buttonVal == HIGH && buttonLast == LOW && (millis() - btnDnTime) > long(debounce)) {
          if (ignoreUp == false) {
            if (pressed) pressed();
          }
          else ignoreUp = false;
          btnUpTime = millis();
        }

        // Test for button held down for longer than the hold time
        if (buttonVal == LOW && (millis() - btnDnTime) > long(holdTime)) {
          if ((millis() - lastHeld) > 30) {
              if (held) held();
              lastHeld = millis();
          }
          ignoreUp = true;
        }

        buttonLast = buttonVal;
    }  
};


//----------------------------------------------------------------------------------
//                                                                      
//----------------------------------------------------------------------------------



//----------------------------------------------------------------------------------
//                                                                      Sleep / Wake
//----------------------------------------------------------------------------------

void leds_wake_up() { }
void leds_sleep() {
    for (int j = g_brightness; j > 0; j-=2) {
        FastLED.setBrightness(j);
        FastLED.show();
    }
    FastLED.clear();
    pinMode(DATA_PIN, INPUT);
    digitalWrite(LED_EN_PIN, LOW);

    USBCON |= _BV(FRZCLK);  //freeze USB clock
    PLLCSR &= ~_BV(PLLE);   // turn off USB PLL
    USBCON &= ~_BV(USBE);   // disable USB

    delay(500);

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    attachInterrupt(0, leds_wake_up, LOW);
    sleep_mode();

    /* SLEEP */
    sleep_disable();
    detachInterrupt(0);

    /* ON */
    sei();
    USBDevice.attach();
    delay(100);
    FastLED.clear();
    digitalWrite(LED_EN_PIN, HIGH);
    pinMode(DATA_PIN, OUTPUT);
}
