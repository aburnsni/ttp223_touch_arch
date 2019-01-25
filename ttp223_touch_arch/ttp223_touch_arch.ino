#include <MIDI.h>
#include <EEPROM.h>
#include "midi_notes.h"
#include "midi_chords.h"

const bool DEBUG = 0;

const int buttons = 5;
int mode = 0;
int strum = 5;  // delay between each note of strum

//MIDI Setup
int* song[][buttons] = {
  {NOTE_C3, NOTE_D3, NOTE_F3, NOTE_G3, NOTE_A3},
  {NOTE_C3, NOTE_D3, NOTE_E3, NOTE_F3, NOTE_G3},
  {NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4},
  {NOTE_C2, NOTE_D2, NOTE_FS2, NOTE_DS2, NOTE_CS2},  // Set midiChannel to 10 for drums
  {CHORD_Em, CHORD_EmD, CHORD_EmC, CHORD_EmB, CHORD_B},
  {NOTE_FS3, NOTE_GS3, NOTE_AS3, NOTE_CS4, NOTE_DS4},
  {NOTE_C3, NOTE_D3, NOTE_E3, NOTE_FS3, NOTE_GS3}
  };
char style[] {'n', 'n', 'n', 'n', 'c', 'n', 'n'};

int midiChannel[] = {2, 2, 2, 10, 2, 2, 2}; // midi channel for each mode
// int instruments[16] = {102, 999, 999, 999, 999, 999, 999, 999, 999, 999 /*Drums*/, 999, 999, 999, 999, 999, 999};
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);


const int buttonPin[buttons] = {14, 15, 16, 17, 18};  //y gr st w o
int buttonState[buttons] = {1, 1, 1, 1, 1};
const int ledPin[buttons] = {12, 11, 10, 9, 8};
const int powerled = 13;

//Rotary enconder
const int encClk = 2; // Needs Interupt pin
const int encDt = 3;
const int encSw = 4;
int lastCount = 0;
volatile int virtualPosition = 0;
const int maxValue = sizeof(song) / sizeof(song[0]); //The number of values on the roatary encoder
int swState = 1;

bool playing[buttons] = {false, false, false, false, false};  //Is note currently playing
unsigned long lasttrig[buttons];
unsigned long debounce = 10;

// HC595 LEDs
const int ledClockPin = 5;
const int ledLatchPin = 6;
const int ledDataPin = 7;
const char common = 'c';    // set to a/c for common anode/cathode

void setup() {
  MIDI.begin();
  if (DEBUG) {
    Serial.begin(115200);  //for hairless midi
  }
  delay(200);

  //MIDIsoftreset();  // Midi Reset
  delay(200);
  //MIDIinstrumentset();  // Set intruments for MIDI channels
  delay(200);

  // Setup I/Os
  pinMode(powerled, OUTPUT);
  digitalWrite(powerled, HIGH);
  for (int i = 0; i < buttons; i++)
  {
    pinMode(buttonPin[i], INPUT_PULLUP);
    pinMode(ledPin[i], OUTPUT);
    digitalWrite(ledPin[i], LOW);
  }
  for (int i = 0; i < buttons; i++) {
    lasttrig[i] = millis();
  }
  //rotary encoder I/Os
  pinMode(encClk, INPUT);
  pinMode(encDt, INPUT);
  pinMode(encSw, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encClk), isr, LOW);
  //hc595 I/Os
  pinMode(ledClockPin, OUTPUT);
  pinMode(ledLatchPin, OUTPUT);
  pinMode(ledDataPin, OUTPUT);

  //Flash LEDs to signal power on
  flashLEDs();

//Read stored mode from EEPROM
  virtualPosition = EEPROM.read(0);
  if (virtualPosition > maxValue) {  // Set to 0 if out of range
    virtualPosition = 0;
  }
  lastCount = virtualPosition;

  //Select mode and light 7seg display
  modeSelect();
}

void loop() {
  for (uint8_t i = 0; i < buttons; i++) {
    buttonState[i] = digitalRead(buttonPin[i]);
    if ((buttonState[i] == HIGH) && (playing[i] == false) && (millis() - lasttrig[i] > debounce)) {
      // turn LED on:
      digitalWrite(ledPin[i], HIGH);
      if (style[mode] == 'n') {
        if (DEBUG) {
          Serial.print("Note: ");
          int value = song[mode][i];
          Serial.print(value);
          Serial.print("\t");
          Serial.print("Channel: ");
          Serial.println(midiChannel[mode]);
        } else {
          MIDI.sendNoteOn(song[mode][i], 100, midiChannel[mode]);
        }
      } else {
        if (DEBUG) {
          Serial.print("Chord: ");
          Serial.println(i);
        } else {
          playChord(song[mode][i], midiChannel[mode]);
        }
      }
      playing[i] = true;
      lasttrig[i] = millis();
    } else if ((buttonState[i] == LOW) && (playing[i] == true)) {
      // turn LED off:
      digitalWrite(ledPin[i], LOW);
      if (style[mode] == 'n') {
        if (DEBUG) {
          Serial.println("NoteOff");
        } else {
          MIDI.sendNoteOff(song[mode][i], 100, midiChannel[mode]);
        }
      } else {
        if (DEBUG) {
          Serial.println("NoteOff");
        } else {
          stopChord(song[mode][i], midiChannel[mode]);
        }
      }
      playing[i] = false;
    }


    //Check rotary encoder
    if (virtualPosition != lastCount) {
      // Serial.print(virtualPosition > lastCount ? "Up :" : "Down :");
      // Serial.println(virtualPosition);

      byte bits = myfnNumToBits(virtualPosition) ;
      myfnUpdateDisplay(bits);    // display alphanumeric digit

      lastCount = virtualPosition;
    }

    swState = digitalRead(encSw);
    if (swState == LOW) {
      modeSelect();
    }
  }
}

// Interupt routine for rotary enconder
void isr() {
  static unsigned long lastInterupTime = 0;
  unsigned long interuptTime = millis();

  // Debounce signals to 5ms
  if (interuptTime - lastInterupTime > 5) {
    if (digitalRead(encDt) == LOW) {
      virtualPosition++;
    } else {
      virtualPosition--;
    }
    //Restrict rotary encoder values
    virtualPosition = min(maxValue - 1, max(0, virtualPosition));
    lastInterupTime = interuptTime;
  }
}

void flashLEDs() {
  for (int i = 0 ; i < buttons; i++) {
    digitalWrite(ledPin[i], HIGH);
    for (int j = 0; j < buttons; j++) {
      if (j != i) {
        digitalWrite(ledPin[j], LOW);
      }
    }
    delay(100);
  }
  for (int i = ( buttons - 2) ; i >= 0; i--) {
    digitalWrite(ledPin[i], HIGH);
    for (int j = 0; j < buttons; j++) {
      if (j != i) {
        digitalWrite(ledPin[j], LOW);
      }
    }
    delay(100);
  }
  digitalWrite(ledPin[0], LOW);
}

void modeSelect() {
  byte bits = myfnNumToBits(virtualPosition) ;
  bits = bits | B00000001;  // add decimal point if needed
  myfnUpdateDisplay(bits);    // display alphanumeric digit
  mode = virtualPosition;
  if (DEBUG) {
    Serial.print("Switch pressed value: ");
    Serial.print(virtualPosition);
    Serial.print("\t");
    Serial.println(bits, BIN);
  }
  EEPROM.update(0, virtualPosition);
  delay(500);
}

void playChord(int i[], int channel) {
  for (uint8_t note = 0; note < 6; note++) {
    if (i[note]) {
      MIDI.sendNoteOn((i[note]), 100, channel);
      delay(strum);
    }
  }
}

void stopChord(int i[], int channel) {
  for (uint8_t note = 0; note < 6; note++) {
    if (i[note]) {
      MIDI.sendNoteOff((i[note]), 100, channel);
      delay(strum);
    }
  }
}

void MIDIsoftreset() { // switch off ALL notes on channel 1 to 16
  for (int channel = 0; channel < 16; channel++)
  {
    MIDI.sendControlChange(123, 0, channel);
  }
}

// void MIDIinstrumentset() {
//   for (uint8_t i = 0; i < 16; i++) {  // Set instruments for all 16 MIDI channels
//     if (instruments[i] < 128) {
//       MIDI.sendProgramChange(instruments[i], i + 1);
//     }
//   }
// }

void myfnUpdateDisplay(byte eightBits) {
  if (common == 'a') {                  // using a common anonde display?
    eightBits = eightBits ^ B11111111;  // then flip all bits using XOR
  }
  digitalWrite(ledLatchPin, LOW);  // prepare shift register for data
  shiftOut(ledDataPin, ledClockPin, LSBFIRST, eightBits); // send data
  digitalWrite(ledLatchPin, HIGH); // update display
}

byte myfnNumToBits(int someNumber) {
  switch (someNumber) {
    case 0:
      return B01111110;
      break;
    case 1:
      return B00010010;
      break;
    case 2:
      return B10111100;
      break;
    case 3:
      return B10110110;
      break;
    case 4:
      return B11010010;
      break;
    case 5:
      return B11100110;
      break;
    case 6:
      return B11101110;
      break;
    case 7:
      return B00110010;
      break;
    case 8:
      return B11111110;
      break;
    case 9:
      return B11110110;
      break;
    case 10:
      return B11111010; // Hexidecimal A
      break;
    case 11:
      return B11001110; // Hexidecimal B
      break;
    case 12:
      return B01101100; // Hexidecimal C or use for Centigrade
      break;
    case 13:
      return B10011110; // Hexidecimal D
      break;
    case 14:
      return B11101100; // Hexidecimal E
      break;
    case 15:
      return B11101000; // Hexidecimal F or use for Fahrenheit
      break;
    default:
      return B10100100; // Error condition, displays three vertical bars
      break;
  }
}
