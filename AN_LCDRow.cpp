#include "LedControl.h"
#include "AN_LCDRow.h"

void AN_LCDRow::initialise(void) {
  // set up the lcd displays
  for (int disp=0; disp<numberOfDisplays; disp++) {
    lc.shutdown(disp,false);// turn off power saving, enables display
    lc.setIntensity(disp,8);// sets brightness (0~15 possible values)
    lc.clearDisplay(disp);// clear screen
  }
}

void AN_LCDRow::setIntens(int intensity) {
  if (intensity > 15) {
    intensity = 15;
  }
  for (int disp=0; disp<numberOfDisplays; disp++) {
    lc.setIntensity(disp, intensity);
  }
}

// add digits to the buffer
void AN_LCDRow::addDigitToNewBuffer(int startPosition, int digit) {
  // the digits are 5 pixels wide
  for (int i=0; i<5; i++) {
    byte tmpRow = 0;
    for (int j=0; j<6; j++) {
      byte digitRow = pgm_read_byte_near(numbers + digit * 7 + j);
      tmpRow = tmpRow | (bitRead(digitRow, (7-i)) << (j+1));
    }
    newDisplayBuffer[startPosition+i] |= tmpRow;
  }
}

// add a special character to the buffer
void AN_LCDRow::addSpecialToNewBuffer(int startPosition, int special)
{
  for (int i=0; i<7; i++) {
    byte tmpRow = 0;
    for (int j=0; j<7; j++) {
      byte charRow = pgm_read_byte_near(specials + special * 7 + j);
      tmpRow = tmpRow | (bitRead(charRow, (7-i)) << (j+1));
    }
    newDisplayBuffer[startPosition+i] |= tmpRow;
  }
}

// add a characeter to the buffer
void AN_LCDRow::addCharacterToNewBuffer(int startPosition, char character)
{
  // check if the position is within the buffer
  if (startPosition > numberOfDisplays * 8-5) {
    return;
  }
  // transform the character to the index in the memory
  int characterIndex = 0;
  if (int(character) >= 0x41 && int(character) <= 0x5A) {
    characterIndex = int(character) - 0x41;
  } else if (int(character) >= 0x61 && int(character) <= 0x7A) {
    characterIndex = int(character) - 0x61;
  }
  for (int i=0; i<5; i++) {
    byte tmpRow = 0;
    for (int j=0; j<7; j++) {
      byte charRow = pgm_read_byte_near(characters + characterIndex * 7 + j);
      tmpRow = tmpRow | (bitRead(charRow, (5-i)) << (j+1));
    }
    newDisplayBuffer[startPosition+i] |= tmpRow;
  }
}

// return the column of one digit
byte AN_LCDRow::getDigitColumn(int digit, int column) {
  // the digits are 5 pixels wide
  byte tmpRow = 0;
  for (int j=0; j<6; j++) {
    byte digitRow = pgm_read_byte_near(numbers + digit * 7 + j);
    tmpRow = tmpRow | (bitRead(digitRow, (7-column)) << (j+1));
  }
  return tmpRow;
}

void AN_LCDRow::addDblPtnToNewBuffer(int startPosition, bool active) {
  if (active) {
    newDisplayBuffer[startPosition] = B00100100;
  } else {
    newDisplayBuffer[startPosition] = B00000000;
  }
  newDisplayBuffer[startPosition+1] = 0;
}

void AN_LCDRow::updateDisplay(void) {
  int posInBuffer = 0;
  for (int disp=0; disp<numberOfDisplays; disp++) {
    for (int row = 0; row<8; row++) {
        lc.setRow(disp, row, currentDisplayBuffer[posInBuffer]);
        posInBuffer++;
    }
  }
}

void AN_LCDRow::showTime(int hours, int minutes, transitionType transition) {
  // the display is 32 pixels wide. Each digit takes 5 pixels (6 including a space on the right). Plus we need
  // the double point in the middle (2 pixels)
  // we therefore need 25 pixels
  // we leave 3 pixels space on the left and 4 on the right

  // check if we need to do anything at all
  if (hours == currentHours && minutes == currentMinutes) {
    return;
  }

  // empty the new buffer
  clearNewBuffer();

  int startOffset = 12;  // offset from the left to where to display the time
  // start of the hours
  int hrsTens = (hours - hours % 10)/10;
  int hrs = hours % 10;
  int minsTens = (minutes - minutes % 10)/10;
  int mins = minutes % 10;
  // calculate previous settings
  int hrsTensPrev = (currentHours - currentHours % 10)/10;
  int hrsPrev = currentHours % 10;
  int minsTensPrev = (currentMinutes - currentMinutes % 10)/10;
  int minsPrev = currentMinutes % 10;

  // if we don't need any transition, just update the display
  if (transition == nothing) {
    // add the hours
    addDigitToNewBuffer(startOffset, hrsTens);
    addDigitToNewBuffer(startOffset+6, hrs);
    // add the separator
    addDblPtnToNewBuffer(startOffset+12, true);
    // add the minutes
    addDigitToNewBuffer(startOffset+14, minsTens);
    addDigitToNewBuffer(startOffset+20, mins);
    memcpy( currentDisplayBuffer, newDisplayBuffer, sizeof(currentDisplayBuffer)*sizeof(char) );
    updateDisplay();
  } else if (transition == rollingVertical) {
    for (int transitionStep = 0; transitionStep <= 7; transitionStep++) {
      if (hrsTens != hrsTensPrev) {
        int counter = 0;
        for (int i=startOffset; i<startOffset+4; i++) {
          currentDisplayBuffer[i] = currentDisplayBuffer[i] >> 1 | getDigitColumn(hrsTens, counter) << (7-transitionStep);
          counter++;
         }
      }
      if (hrs != hrsPrev) {
        int counter = 0;
        for (int i=startOffset+6; i<startOffset+11; i++) {
          currentDisplayBuffer[i] = currentDisplayBuffer[i] >> 1 | getDigitColumn(hrs, counter) << (7-transitionStep);
          counter++;
         }
      }
      if (minsTens != minsTensPrev) {
        int counter = 0;
        for (int i=startOffset+14; i<startOffset+19; i++) {
          currentDisplayBuffer[i] = currentDisplayBuffer[i] >> 1 | getDigitColumn(minsTens, counter) << (7-transitionStep);
          counter++;
         }
      }
      if (mins != minsPrev) {
        int counter = 0;
        for (int i=startOffset+20; i<startOffset+25; i++) {
          currentDisplayBuffer[i] = currentDisplayBuffer[i] >> 1 | getDigitColumn(mins, counter) << (7-transitionStep);
          counter++;
         }
      }
      // add the double points
      currentDisplayBuffer[startOffset+12] = B00100100;
      // if we are snoozing, add a Z to the front
      if (snoozeState) {
        currentDisplayBuffer[0] = B10100000;
        currentDisplayBuffer[1] = B11100000;
        currentDisplayBuffer[2] = B10100000;
      }
      updateDisplay();
      delay(20);
    }
  } else if (transition == rollingHorizontal) {
    // for this animation is the current buffer needed
    // main animation looop
    for (int transitionStep = 0; transitionStep < 6; transitionStep++) {
      // modify the positions of the digits (5 pixel wide)
      if (hrsTensPrev != hrsTens) {
        for (int i=startOffset; i<startOffset+4; i++) {
          currentDisplayBuffer[i] = currentDisplayBuffer[i+1];
        }
        if (transitionStep == 0) {
          currentDisplayBuffer[startOffset+4] = 0; // whitespace
        } else {
          currentDisplayBuffer[startOffset+4] = getDigitColumn(hrsTens, transitionStep-1);
        }
      }

      if (hrs != hrsPrev) {        
        for (int i=startOffset+6; i<startOffset+10; i++) {
          currentDisplayBuffer[i] = currentDisplayBuffer[i+1];
        }
        if (transitionStep == 0) {
          currentDisplayBuffer[startOffset+10] = 0; // whitespace
        } else {
          currentDisplayBuffer[startOffset+10] = getDigitColumn(hrs, transitionStep-1);
        }
      }

      if (minsTens != minsTensPrev) {        
        for (int i=startOffset+14; i<startOffset+18; i++) {
          currentDisplayBuffer[i] = currentDisplayBuffer[i+1];
        }
        if (transitionStep == 0) {
          currentDisplayBuffer[startOffset+18] = 0; // whitespace
        } else {
          currentDisplayBuffer[startOffset+18] = getDigitColumn(minsTens, transitionStep-1);
        }
      }

      if (mins != minsPrev) {        
        for (int i=startOffset+20; i<startOffset+24; i++) {
          currentDisplayBuffer[i] = currentDisplayBuffer[i+1];
        }
        if (transitionStep == 0) {
          currentDisplayBuffer[startOffset+24] = 0; // whitespace
        } else {
          currentDisplayBuffer[startOffset+24] = getDigitColumn(mins, transitionStep-1);
        }
      }
      // add the double points
      currentDisplayBuffer[startOffset+12] = B00100100;
      updateDisplay();
      delay(20);
    }
  }
  currentHours = hours;
  currentMinutes = minutes;
}

//
//  Show alarm on display
//
void AN_LCDRow::showAlarm(int alarmNr, bool positive)
{
  // empty the new buffer
  clearNewBuffer();
  // add the text in the middle
  addCharacterToNewBuffer(9, 'W');
  addCharacterToNewBuffer(15, 'A');
  addCharacterToNewBuffer(20, 'K');
  addCharacterToNewBuffer(25, 'E');
  addCharacterToNewBuffer(32, 'U');
  addCharacterToNewBuffer(37, 'P');
  // add the alarm symbol character
  addSpecialToNewBuffer(1, 0);
  addSpecialToNewBuffer(40, 0);

  if (!positive) {
    // invert the buffer
    for (int i=0; i<sizeof(newDisplayBuffer); i++) {
      newDisplayBuffer[i] = ~(newDisplayBuffer[i]);
    }
  }

  // display the alarm
  copyNewToCurrentBuffer();
  updateDisplay();
}

//
//  Clear the LCDs
//
void AN_LCDRow::clearLCDs(void)
{
  for (int i=0; i<sizeof(currentDisplayBuffer); i++) {
    currentDisplayBuffer[i] = 0;
  }
  // reset time
  currentHours = 77;
  currentMinutes = 77;
  // display
  updateDisplay();
}

//
//  Show a string for a period of time
//
void AN_LCDRow::showString(char* string, int nOfDigits, int duration, transitionTypeTypeDef transition)
{
  // clear the displayBuffer
  clearNewBuffer();
  // write the characters to the buffer
  // each character takes 6 pixels in width. This includes a space
  int position = 0;
  int counter = 0;
  while (counter < nOfDigits) {
    char character = string[counter];
    if (character != ' ') {
      addCharacterToNewBuffer(position, character);
    }
    counter += 1;
    position += 6;
  }
  // update the display
  copyNewToCurrentBuffer();
  updateDisplay();
  delay(duration);
  clearCurrentBuffer();
  updateDisplay();
}

//
//  Clear the new buffer
//
void AN_LCDRow::clearNewBuffer(void)
{
  for (int i=0; i<sizeof(newDisplayBuffer); i++) {
    newDisplayBuffer[i] = 0;
  }
}

//
//  Clear the current buffer
//
void AN_LCDRow::clearCurrentBuffer(void)
{
  for (int i=0; i<sizeof(currentDisplayBuffer); i++) {
    currentDisplayBuffer[i] = 0;
  }
}

//
//  push the new buffer to the current buffer
//
void AN_LCDRow::copyNewToCurrentBuffer(void)
{
  memcpy( currentDisplayBuffer, newDisplayBuffer, sizeof(currentDisplayBuffer)*sizeof(char) );
}

//
//  execute a transition between the buffers
//
void AN_LCDRow::bufferTransition(transitionType transition, int transitionTimeMS)
{
  switch (transition)
  {
    case nothing:
    // no transition selected. Display immediately
    copyNewToCurrentBuffer();
    updateDisplay();
    break;

    case rollingVertical:
    break;

    case rollingHorizontal:
    break;

    default:
    break;
  }
}

//
//  Set/unset snooze state. This adds an indicator to the clock
//
void AN_LCDRow::setSnoozeState(bool state)
{
  snoozeState = state;
}
