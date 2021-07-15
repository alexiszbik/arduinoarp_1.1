
#include <Wire.h>
#include <Adafruit_MCP4725.h>

#define NOTE_COUNT 12
#define MAX_NOTE 48

#define NOTE_PIN_START 2

enum eMode {
    up = 0,
    down,
    upAndDown,
    arpRandom
};

Adafruit_MCP4725 dac;
bool notes[NOTE_COUNT];
int clockState = LOW;
int iterator = 0;
bool goUp = false;

enum eMode mode = arpRandom;

void setup(void) {
    Serial.begin(9600);

    // For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
    // For MCP4725A0 the address is 0x60 or 0x61
    // For MCP4725A2 the address is 0x64 or 0x65
    dac.begin(0x62);
  
    for (int i = 0; i < NOTE_COUNT; i++) {
        notes[i] = false;
        pinMode(NOTE_PIN_START + i, INPUT_PULLUP);
    }
}

int noteToVolt(int noteIn) {
    return (int)round(((float)(4095.0/5.0)/12.0) * noteIn);
}

void loop(void) {
    
    int newClockState = analogRead(0) > 200;

    int startPot = analogRead(1);
    int rangePot = analogRead(6);

    int noteRange = (int)fmin(round(rangePot/21.), (double)MAX_NOTE);
    int minRange = (int)fmin(round(startPot/21.), (double)MAX_NOTE);
    int maxRange = (int)fmin(minRange + noteRange, (double)MAX_NOTE);

    bool modeSwitchUp = analogRead(2) > 500;
    bool modeSwitchDown = analogRead(3) > 500;

    enum eMode mode;
    if (modeSwitchUp && modeSwitchDown) {
        mode = upAndDown;
        
    } else if (modeSwitchUp && !modeSwitchDown) {
        mode = up;
    } else if (!modeSwitchUp && modeSwitchDown) {
        mode = down;
    } else {
        mode = arpRandom;
    }

    for (int i = 0; i < NOTE_COUNT; i++) {
        notes[i] = digitalRead(NOTE_PIN_START + i) == HIGH;
    }

    bool trueNoteExist = false;
    //if all notes false, all notes true
    for (int i = 0; i < NOTE_COUNT; i++) {
        trueNoteExist = trueNoteExist || notes[i];
    }
    
    if (!trueNoteExist) {
        for (int i = 0; i < NOTE_COUNT; i++) {
            notes[i] = true;
        }
    }

    //compute min and max note
    int minNote = MAX_NOTE;
    int maxNote = 0;
    
    for (int i = minRange; i <= maxRange; i++) {
        if (notes[i%NOTE_COUNT]) {
            if (i < minNote) {
                minNote = i;
            }
            if (i > maxNote) {
                maxNote = i;
            }
        }
    }

    if (minNote > maxNote) {
        for (int i = minRange; i <= MAX_NOTE; i++) {
            if (notes[i%NOTE_COUNT]) {
                minNote = i;
                maxNote = i;
                break;
            }
        }
    }

    if (mode == arpRandom) {
        iterator = random(minNote, maxNote+1);
        mode = random(100) > 50 ? up : down;
    }

    if (newClockState != clockState) {
      
        clockState = newClockState;
        if (newClockState == HIGH) {

            for (int i = minNote; i <= maxNote; i++) {

                switch (mode) {
                  case up: {
                      iterator++;
                      goUp = true;
                      if (iterator > maxNote) {
                          iterator = minNote;
                      }
                  }
                      break;
                
                  case down: {
                      iterator--;
                      goUp = false;
                      if (iterator < minNote) {
                          iterator = maxNote;
                      }
                  }
                      break;
                
                  case upAndDown: {
                      if (goUp) {
                          iterator++;
                          if (iterator > maxNote) {
                              iterator = maxNote-1;
                              goUp = false;
                          }
                      } else {
                          iterator--;
                          if (iterator < minNote) {
                              iterator = minNote+1;
                              goUp = true;
                          }
                      }
                  }
                      break;
                
                  default:
                      break;
               }
        
              iterator = fmin(fmax(iterator, minNote), maxNote);
        
              if (notes[iterator%NOTE_COUNT]) {
                  break;
              }
           }

           int volt = noteToVolt(iterator);
           dac.setVoltage(volt, false);
        }
    }
}
