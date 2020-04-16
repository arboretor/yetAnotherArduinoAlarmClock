#include "LedControl.h"

const int numberOfDisplays = 6; // the number of displays

// all numbers are 5 pixels wide (excluding spaces)
const unsigned char numbers [] PROGMEM = {
  B01110000, // 0
  B10001000,
  B10001000,
  B10001000,
  B10001000,
  B01110000,
  B00000000,

  B00010000, // 1
  B00110000,
  B00010000,
  B00010000,
  B00010000,
  B00111000,
  B00000000,

  B01110000,  // 2
  B10001000,
  B00010000,
  B00100000,
  B01000000,
  B11111000,
  B00000000,

  B01110000, // 3
  B10001000,
  B00001000,
  B00110000,
  B10001000,
  B01110000,
  B00000000,

  B00010000,  // 4
  B00110000,
  B01010000,
  B11111000,
  B00010000,
  B00010000,
  B00000000,

  B11110000, // 5
  B10000000,
  B11100000,
  B00010000,
  B00010000,
  B11100000,
  B00000000,

  B00110000, // 6
  B01000000,
  B11110000,
  B10001000,
  B10001000,
  B01110000,
  B00000000,

  B11111000, // 7
  B10001000,
  B00010000,
  B00100000,
  B01000000,
  B10000000,
  B00000000,

  B01110000, // 8
  B10001000,
  B01110000,
  B10001000,
  B10001000,
  B01110000,
  B00000000,

  B01110000, // 9
  B10001000,
  B10001000,
  B01111000,
  B00001000,
  B01110000,
  B00000000
};

//
//  Special characters
//
const unsigned char specials [] PROGMEM = {
  B00010000, // alarm bell
  B00111000,
  B01111100,
  B01111100,
  B01111100,
  B11111110,
  B00010000
};

// full size characters
const unsigned char characters [] PROGMEM = {
  B000000, // A
  B011000,
  B100100,
  B111100,
  B100100,
  B100100,
  B000000,

  B000000, // B
  B111000,
  B100100,
  B111000,
  B100100,
  B111000,
  B000000,

  B000000, // C
  B011100,
  B100000,
  B100000,
  B100000,
  B011100,
  B000000,

  B000000, // D
  B111000,
  B100100,
  B100100,
  B100100,
  B111000,
  B000000,

  B000000, // E
  B111100,
  B100000,
  B111100,
  B100000,
  B111100,
  B000000,

  B000000, // F
  B111100,
  B100000,
  B111000,
  B100000,
  B100000,
  B000000,

  B00000000, // G
  B011100,
  B100000,
  B101100,
  B100100,
  B011100,
  B000000,

  B000000, // H
  B100100,
  B100100,
  B111100,
  B100100,
  B100100,
  B000000,

  B000000, // I
  B011100,
  B001000,
  B001000,
  B010000,
  B011100,
  B000000,

  B000000, // J
  B011100,
  B001000,
  B001000,
  B001000,
  B011000,
  B000000,

  B000000, // K
  B100100,
  B101000,
  B110000,
  B101000,
  B100100,
  B000000,

  B000000, // L
  B100000,
  B100000,
  B100000,
  B100100,
  B111100,
  B000000,

  B000000, // M
  B100010,
  B110110,
  B101010,
  B100010,
  B100010,
  B000000,

  B000000, // N
  B100100,
  B110100,
  B101100,
  B101100,
  B100100,
  B000000,

  B000000, // O
  B011000,
  B100100,
  B100100,
  B100100,
  B011000,
  B000000,

  B000000, // P
  B111000,
  B101000,
  B111000,
  B100000,
  B100000,
  B000000,

  B000000, // Q
  B001000,
  B010100,
  B100010,
  B010100,
  B001010,
  B000000,

  B000000, // R
  B111000,
  B100100,
  B111000,
  B100100,
  B100100,
  B000000,

  B000000, // S
  B011110,
  B100000,
  B011100,
  B000010,
  B111100,
  B000000,

  B000000, // T
  B111110,
  B001000,
  B001000,
  B001000,
  B001000,
  B000000,

  B000000, // U
  B100100,
  B100100,
  B100100,
  B100100,
  B011000,
  B000000,

  B000000, // V
  B100010,
  B010010,
  B010100,
  B010100,
  B001000,
  B000000,

  B000000, // W
  B100010,
  B100010,
  B101010,
  B101010,
  B010100,
  B000000,

  B000000, // X
  B100010,
  B010100,
  B001000,
  B010100,
  B100010,
  B000000,

  B000000, // Y
  B100010,
  B010100,
  B001000,
  B001000,
  B001000,
  B000000,

  B000000, // Z
  B111110,
  B000100,
  B001000,
  B010000,
  B111110,
  B000000
};

typedef enum transitionType {
  nothing,  // just update the display
  rollingVertical, // exchange the digits by rolling the changed digits
  rollingHorizontal  // roll the number horizontal
} transitionTypeTypeDef;

class AN_LCDRow
{
  LedControl lc = LedControl(12,11,10,numberOfDisplays);
  byte currentDisplayBuffer [numberOfDisplays * 8+1] = {0}; // the buffer for the display
  byte newDisplayBuffer [numberOfDisplays * 8+1] = {0};     // soon to be display (will be used by transition animations)

  int currentHours = 77;
  int currentMinutes = 77;

  bool snoozeState = false;

  public:
    void showTime(int hours, int minutes, transitionType transition);
    void initialise(void);
    void setIntens(int intensity);
    void showAlarm(int alarmNr, bool positive);
    void clearLCDs(void);
    void showString(char* string, int nOfDigits, int duration, transitionTypeTypeDef transition);
    void setSnoozeState(bool state);
    
  private: 
    void addDigitToNewBuffer(int startPosition, int digit);
    void addSpecialToNewBuffer(int startPosition, int special);
    void addCharacterToNewBuffer(int startPosition, char character);
    byte getDigitColumn(int digit, int column);
    void addDblPtnToNewBuffer(int startPosition, bool active);
    void updateDisplay(void);
    void clearCurrentBuffer(void);
    void clearNewBuffer(void);
    void copyNewToCurrentBuffer(void);
    void bufferTransition(transitionType transition, int transitionTimeMS);
};
