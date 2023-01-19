#include "MIDIUSB.h"

// BOTOES
const int N_BUTTON = 9;                                   // Numero total de botoes
byte buttonPin[N_BUTTON] = { 2, 3, 4, 5, 6, 7, 15, 14, 10 };  // Pinos digitais
byte buttonState[N_BUTTON] = { 0 };
byte buttonPState[N_BUTTON] = { 0 };
unsigned long lastBounce[N_BUTTON] = { 0 };
unsigned long buttonTimer[N_BUTTON] = { 0 };
int buttonTimeout = 15;

// POTENCIOMETROS
const int N_POTS = 3;                  // Numero total de pots
byte potPin[N_POTS] = { A0, A1, A2 };  // Pinos analogicos
int potState[N_POTS] = { 0 };
int potPState[N_POTS] = { 0 };
byte midiState[N_POTS] = { 0 };
byte midiPState[N_POTS] = { 0 };
int varThreshold = 20;


boolean openGate[N_POTS] = { false };

unsigned long lastPot[N_POTS] = { 0 };
unsigned long potTimer[N_POTS] = { 0 };

int potTimeout = 300;

int MIDI_CH = 0;  // 0-15
int NN[N_BUTTON] = {};
int primeiraNN = 1;


int CC[N_POTS] = {};
int primeiroCC = 1;

// LEDS

const int N_LED = 3;
byte ledPin[N_LED] = { 8, 9, 16 };

void setup() {

  Serial.begin(115200);

  pinMode(10, INPUT_PULLUP);

  for (int i = 0; i < N_BUTTON; i++) {
    NN[i] = primeiraNN;
    primeiraNN = primeiraNN + 1;
  }
  for (int i = 0; i < N_POTS; i++) {
    CC[i] = primeiroCC;
    primeiroCC = primeiroCC + 1;
  }
  // delclarar os pinos do arduino
  for (int i = 0; i < N_BUTTON; i++) {
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

  for (int i = 0; i < N_LED; i++) {
    pinMode(ledPin[i], OUTPUT);
  }
}
// - - - - - - - - - - - - - -

void loop() {

  buttons();
  potentiometers();
  readMidi();
}

// - - - - - - - - - - - - - -

void readMidi() {
  midiEventPacket_t message;
  do {
    message = MidiUSB.read();
    if (message.header !=0) {
      Serial.print("Mensagem MIDI: ");
      Serial.print(message.header);
      Serial.print(" - ");
      Serial.print(message.byte1);
      Serial.print(" - ");
      Serial.print(message.byte2);
      Serial.print(" - ");
      Serial.println(message.byte3);
    }
    if (message.byte3 == 96) {
      digitalWrite(8, HIGH);
    } else if (message.byte3 == 97) {
      digitalWrite(8, LOW);
    } else if (message.byte3 == 98) {
      digitalWrite(9, HIGH);
    } else if (message.byte3 == 99) {
      digitalWrite(9, LOW);
    } else if (message.byte3 == 100) {
      digitalWrite(16, HIGH);
    } else if (message.byte3 == 101) {
      digitalWrite(16, LOW);
    }
  } while (message.header != 0);
}

void buttons() {

  for (int i = 0; i < N_BUTTON; i++) {
    buttonState[i] = digitalRead(buttonPin[i]);

    buttonTimer[i] = millis() - lastBounce[i];  // corre o timer

    if (buttonTimer[i] > buttonTimeout) {

      if (buttonState[i] != buttonPState[i]) {
        lastBounce[i] = millis();  // armazena a hora

        if (buttonState[i] == 0) {

          noteOn(MIDI_CH, NN[i], 127);
          MidiUSB.flush();

          // Serial.print("Botao ");
          // Serial.print(i);
          // Serial.println(": on");
        } else {

          noteOn(MIDI_CH, NN[i], 0);
          MidiUSB.flush();

          // Serial.print("Botao ");
          // Serial.print(i);
          // Serial.println(": off");
        }

        buttonPState[i] = buttonState[i];
      }
    }
  }
}

// - - - - - - - - - - - - - -

void potentiometers() {

  for (int i = 0; i < N_POTS; i++) {
    potState[i] = analogRead(potPin[i]);
    midiState[i] = map(potState[i], 0, 1020, 63.5, 127);

    int potVar = abs(potState[i] - potPState[i]);

    if (potVar > varThreshold) {
      lastPot[i] = millis();
    }

    potTimer[i] = millis() - lastPot[i];

    if (potTimer[i] < potTimeout) {
      if (midiState[i] != midiPState[i]) {

        controlChange(MIDI_CH, CC[i], midiState[i]);
        MidiUSB.flush();

        // Serial.print("Pot ");
        // Serial.print(i);
        // Serial.print(": ");
        // Serial.println(midiState[i]);

        midiPState[i] = midiState[i];
        potPState[i] = potState[i];
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - -

// Arduino (pro)micro midi functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOn);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}