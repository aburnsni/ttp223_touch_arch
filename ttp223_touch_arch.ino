#include <MIDI.h>
#include "midi_notes.h"

const int buttons = 5;

//MIDI Setup
int* song[buttons] = {NOTE_C3, NOTE_D3, NOTE_F3, NOTE_G3, NOTE_A3};
int midiChannel[buttons] = {5, 5, 5, 5, 5}; // midi channel for each button
int instruments[16] = {102, 999, 999, 999, 999, 999, 999, 999, 999, 999 /*Drums*/, 999, 999, 999, 999, 999, 999};
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);


const int buttonPin[buttons] = {2, 3, 4, 5, 6};  //y gr st w o
int buttonState[buttons] = {1, 1, 1, 1, 1};
const int ledPin[buttons] = {8, 9, 10, 11, 12};
const int powerled = 13;

//Rotary enconder
const int encClk = 2; // Needs Interupt pin
const int encDt = 3;
const int encSw = 4;
int lastCount = 0;
volatile int virtualPosition = 0;
const int maxValue = 7; //The number of values on the roatary encoder

bool playing[buttons] = {false, false, false, false, false};  //Is note currently playing
unsigned long lasttrig[buttons];
unsigned long debounce = 10;

// Interupt routine for rotary enconder
void isr() {
  static unsigned long lastInterupTime = 0;
  unsigned long interuptTime = millis();

  // Debounce signals to 5ms
  if (interuptTime - lastInterupTime > 5) {
    if (digitalRead(encDt) ==LOW) {
      virtualPosition++;
    } else {
      virtualPosition--;
    }
    //Restrict rotary encoder values
    virtualPosition = min(maxValue-1, max(0, virtualPosition));
    lastInterupTime = interuptTime;
  }
}

void setup() {
  MIDI.begin();
 // Serial.begin(115200);  //for hairless midi
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
    pinMode(ledPin[i], OUTPUT);
    digitalWrite(ledPin[i], LOW);
  }
  for (int i = 0; i < buttons; i++) {
    lasttrig[i] = millis();
  }
  pinMode(encClk, INPUT);
  pinMode(encDt, INPUT);
  pinMode(encSw, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encClk), isr, LOW);

  //Flash LEDs to signal power on
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

void loop() {

  for (uint8_t i = 0; i < buttons; i++) {
    buttonState[i] = digitalRead(buttonPin[i]);
    if ((buttonState[i] == HIGH) && (playing[i] == false) && (millis() - lasttrig[i] > debounce)) {
      // turn LED on:
      digitalWrite(ledPin[i], HIGH);
      MIDI.sendNoteOn(song[i], 100, midiChannel[i]);
      playing[i] = true;
      lasttrig[i] = millis();
    } else if ((buttonState[i] == LOW) && (playing[i] == true)) {
      // turn LED off:
      digitalWrite(ledPin[i], LOW);
      MIDI.sendNoteOff(song[i], 100, midiChannel[i]);
      playing[i] = false;
    }
  }

  //Check rotary encoder
  if (virtualPosition != lastCount) {
    Serial.print(virtualPosition > lastCount ? "Up :" : "Down :");
    Serial.println(virtualPosition);
    lastCount = virtualPosition;
  }
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
