#include "src/AN_LCDRow.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include <Wire.h>
#include <MD_DS3231.h>

#define DEBOUNCE      10   // button debounce in ms
#define CLOCKUPDATE   200 // check to update clock in ms
#define ALARM_FLASH   2   // flash interval of alarm display in units of CLOCKUPDATE

// everything buttons
byte buttons[] = {0,1,2,3,4,5,8,13};
#define NUMBUTTONS              sizeof(buttons)
#define BUZZER_LIGHT            6                     //!< buzzer light output (PWM)
#define BUZZER_OUTPUT           8                     //!< piezo buzzer output
volatile byte pressed[NUMBUTTONS], justreleased[NUMBUTTONS];
bool anyButtonPressed = false;

//
//  Program state
//
typedef enum {
  PROGRAM_STATE_NORMAL,     // normal operaiton of program. The time is shown
  PROGRAM_STATE_ALARM,      // alarm state of the program. An alarm is shown
  PROGRAM_STATE_SNOOZING,   // snoozing
  PROGRAM_STATE_SETTINGS,    // in settings state
} programStateTypeDef;

programStateTypeDef currentProgramState = PROGRAM_STATE_NORMAL;

//
//  alarm indicator helpers
//
int alarmFlashCounter = 0;
bool alarmFlashPolarity = true;

/* button map
 *  7: snooze (big red button)
 *  4: blue down
 *  0: blue up
 *  1: green center
 *  5: gray left
 *  3: gray right
 *  2: red back 
 *  
 *  6: led for button
 *  8: buzzer piezo output
 */
#define BUTTON_SNOOZE     7
#define BUTTON_BLUE_DOWN  4
#define BUTTON_BLUE_UP    0
#define BUTTON_GREEN      1
#define BUTTON_GRAY_RIGHT 3
#define BUTTON_GRAY_LEFT  5
#define BUTTON_RED        2
 /*
  * Timer uses
  * 
  * timer0: delay(), millis() and micros() provided by Arduino. Also analogWrite on pin 6
  * timer1: potentially not used
  * timer2: tone() provided by Arduino
  */

// luminosity sensor
Adafruit_TSL2591 lumSens = Adafruit_TSL2591(2591);

// lcd display row
AN_LCDRow lcdRow = AN_LCDRow();

int currentIntensity = 8;

void setup() {
  Serial.begin(9600);

  lcdRow.initialise();

  // initialise the buttons
  for (int i=0; i<NUMBUTTONS; i++) {
    pinMode(buttons[i], INPUT_PULLUP);
  }

  // initialise PWM output for main push button
  pinMode(BUZZER_LIGHT, OUTPUT);
  analogWrite(BUZZER_LIGHT, 255); 

  // initialise the speaker output
  pinMode(BUZZER_OUTPUT, OUTPUT);
  tone(BUZZER_OUTPUT, 2000, 1000); // output @ 1kHz

  // create interrupt on timer1 every 10ms
  // enable the timer, 16MHz no prescaler
  TCCR1A = 0;                             // disable output compare mode
  TCCR1B = 0<<CS12 | 0<<CS11 | 1<<CS10;   // disable prescaler
  // program the counter
  // counter value is 155 for 10ms
  TCNT1L = 155;    // low byte 
  TCNT1H = 0;    // high byte

  // Timer1 Overflow interrupt enable
  TIMSK1 |= 1<<TOIE1;
  
//  // set up the luminosity sensor
//  if(lumSens.begin()) {
//    lumSens.setGain(TSL2591_GAIN_MED);
//    lumSens.setTiming(TSL2591_INTEGRATIONTIME_100MS);
//    Serial.println("Found luminosity sensor");
//  } else {
//    Serial.println("Luminosity sensor not found");
//  }

  RTC.readTime();
  lcdRow.showTime(RTC.h, RTC.m, rollingVertical);
}

/*
 * Timer1 overlfow interrupt service routine
 */
SIGNAL(TIMER1_OVF_vect) {
  check_switches();
}

// timer for clock update
int lastClockCheck = 0;
void loop() {

  // handle button presses
  if (anyButtonPressed) {
    for (int i=0; i<NUMBUTTONS; i++) {
      if (justreleased[i]) {
        Serial.print("Released button "); Serial.println(i, DEC);

        // for debug
        if (i == BUTTON_GREEN && currentProgramState == PROGRAM_STATE_NORMAL) {
          lcdRow.clearLCDs();
          Serial.println("Entering alarm state");
          currentProgramState = PROGRAM_STATE_ALARM;
        } else if (currentProgramState == PROGRAM_STATE_ALARM) {
          if (i == BUTTON_SNOOZE) {
            showSnooze();
            lcdRow.setSnoozeState(true);
            // enter snoozing state
            currentProgramState = PROGRAM_STATE_SNOOZING;
            noTone(BUZZER_OUTPUT);
          } else if (i == BUTTON_RED) {
            disableAlarm();
          }
        } else if (i == BUTTON_RED && currentProgramState == PROGRAM_STATE_SNOOZING) {
          disableAlarm();
        }
      }
    }
    clearSwitches();
  }

  // prevent wrap around of millis to mess with the update
  if (lastClockCheck > millis()) {
    lastClockCheck = millis();
  }

  // main logic event loop
  eventLoop();
}

//
//  Check if any of the buttons are pressed.
//  
void check_switches()
{  
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  static long lasttime;
  byte index;

  if (millis() < lasttime) { // we wrapped around, lets just try again
     lasttime = millis();
  }
  
  if ((lasttime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    return; 
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();
  
  for (index = 0; index<sizeof(buttons); index++) {
    currentstate[index] = digitalRead(buttons[index]);   // read the button

    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
          // just pressed
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
          // just released
          justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
    }
    //Serial.println(pressed[index], DEC);
    previousstate[index] = currentstate[index];   // keep a running tally of the buttons
    // set the general flag high
    anyButtonPressed = true;
  }
}

//
//  Clear buffer for pressed buttons
//
void clearSwitches() {
  for (int i=0; i<NUMBUTTONS; i++) {
    pressed[i] = 0;
    justreleased[i] = 0;
  }
  anyButtonPressed = false;
}

//
//  Handle clock update
//
void handleClock(void)
{
  if ((lastClockCheck - millis()) > CLOCKUPDATE) {
    RTC.readTime();
    lcdRow.showTime(RTC.h, RTC.m, rollingVertical);
    delay(200);
  
    // adjust intensity of diodes dependent on time of the day (luminsity sensor not working yet)
    if ((RTC.h < 8 || RTC.h > 22)  && currentIntensity != 0) {
      lcdRow.setIntens(0);
      currentIntensity = 0;
    }
  
    if ((RTC.h >8 && RTC.h < 22) && currentIntensity != 8) {
      lcdRow.setIntens(8);
      currentIntensity = 8;
    }
    lastClockCheck = millis();
  }
}

//
//  Handle alarm display
//
void handleAlarm(void)
{
  if ((lastClockCheck - millis()) > CLOCKUPDATE) {
    lcdRow.showAlarm(1, alarmFlashPolarity);
    alarmFlashCounter++;
    if (alarmFlashCounter > ALARM_FLASH) {
      alarmFlashCounter = 0;
      alarmFlashPolarity = !alarmFlashPolarity;
      if (alarmFlashPolarity) {
        tone(BUZZER_OUTPUT, 2000, 500); // output @ 2kHz
      } else {
        noTone(BUZZER_OUTPUT);
      }
    }
    lastClockCheck == millis();
  }
}

//
//  Set the snooze
//
void showSnooze(void)
{
  char str[] = " snooze";
  lcdRow.showString(str, sizeof(str)-1, 5000, nothing);
}

//
//  Disable the alarm
//
void disableAlarm(void)
{
  noTone(BUZZER_OUTPUT);
  lcdRow.setSnoozeState(false);
  currentProgramState = PROGRAM_STATE_NORMAL;
  lcdRow.clearLCDs();
  char str[] = "alm off";
  lcdRow.showString(str, sizeof(str)-1, 5000, nothing);
}

//
//  Main program event and state loop
//
void eventLoop(void)
{
  if (currentProgramState == PROGRAM_STATE_NORMAL) {
    // show the normal clock face.
    handleClock();
  } else if(currentProgramState == PROGRAM_STATE_ALARM) {
    handleAlarm();
  } else if (currentProgramState == PROGRAM_STATE_SNOOZING) {
    // we are snoozing. Handle clock normaly
    handleClock();
  } else if(currentProgramState == PROGRAM_STATE_SETTINGS) {

  }
}
