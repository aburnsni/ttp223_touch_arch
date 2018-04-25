#include <MIDI.h>
#include "midi_notes.h"

const int buttons = 5;

//MIDI Setup
int* song[buttons] = {NOTE_C3, NOTE_D3, NOTE_E3, NOTE_F3, NOTE_G3};
int midiChannel[buttons] = {5,5,5,5,5}; // midi channel for each button
int instruments[16] = {102, 999, 999, 999, 999, 999, 999, 999, 999, 999 /*Drums*/, 999, 999, 999, 999, 999, 999};
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);  


const int buttonPin[buttons] = {2, 3, 4, 5,6};
int buttonState[buttons] = {1, 1, 1, 1,1};
const int powerled = 13;

bool playing[buttons] = {false, false, false, false, false};  //Is note currently playing
unsigned long lasttrig[buttons];
unsigned long debounce = 10;

void setup() {
  MIDI.begin();
  Serial.begin(115200);  //for hairless midi
  delay(200);

  MIDIsoftreset();  // Midi Reset
  delay(200);
  MIDIinstrumentset();  // Set intruments for MIDI channels
  delay(200);

  // Setup I/Os
  pinMode(powerled, OUTPUT);
  digitalWrite(powerled, HIGH);
  for (int i = 0; i < buttons; i++)
  {
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

  for (int i = 0; i < buttons; i++) {
  lasttrig[i] = millis();
}
}

void loop() {
  for (uint8_t i = 0; i < buttons; i++) {
    buttonState[i] = digitalRead(buttonPin[i]);
    if ((buttonState[i] == HIGH) && (playing[i] == false) && (millis() - lasttrig[i] > debounce)) {

      MIDI.sendNoteOn(song[i], 100, midiChannel[i]);
      playing[i] = true;
      lasttrig[i] = millis();
    } else if ((buttonState[i] == LOW) && (playing[i] == true)) {

      MIDI.sendNoteOff(song[i], 100, midiChannel[i]);
      playing[i] = false;
    }
  }
  delay(1);
}

void MIDIsoftreset()  // switch off ALL notes on channel 1 to 16
{
  for (int channel = 0; channel < 16; channel++)
  {
    MIDI.sendControlChange(123, 0, channel);
  }
}

void MIDIinstrumentset() {
  for (uint8_t i = 0; i < 16; i++) {  // Set instruments for all 16 MIDI channels
    if (instruments[i] < 128) {
      MIDI.sendProgramChange(instruments[i], i + 1);
    }
  }
}
