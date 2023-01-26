#include "MIDIUSB.h" 
#include <Multiplexer4067.h> // Multiplexer CD4067 library >> https://github.com/sumotoy/Multiplexer4067
#include <Thread.h> // Threads library >> https://github.com/ivanseidel/ArduinoThread
#include <ThreadController.h> // Mesma lib de cima

// buttons
const int muxNButtons = 16; // *coloque aqui o numero de entradas digitais utilizadas no multiplexer
const int NButtons = 1; // *coloque aqui o numero de entradas digitais utilizadas
const int totalButtons = muxNButtons + NButtons;
const byte muxButtonPin[muxNButtons] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}; // *neste array coloque na ordem desejada os pinos das portas digitais utilizadas
const byte buttonPin[NButtons] = {15}; // *neste array coloque na ordem desejada os pinos das portas digitais utilizadas
int buttonCState[totalButtons] = {0}; // estado atual da porta digital
int buttonPState[totalButtons] = {0}; // estado previo da porta digital

/////////////////////////////////////////////
// debounce
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 5;    // the debounce time; increase if the output flickers

/////////////////////////////////////////////
// potentiometers
const int NPots = 3; // *coloque aqui o numero de entradas analogicas utilizadas
const byte potPin[NPots] = {A0, A1, A2};
int potCState[NPots] = {0}; // estado atual da porta analogica
int potPState[NPots] = {0}; // estado previo da porta analogica
byte midiState[NPots] = { 0 };
byte midiPState[NPots] = { 0 };
int varThreshold = 20;

boolean openGate[NPots] = { false };

unsigned long lastPot[NPots] = { 0 };
unsigned long potTimer[NPots] = { 0 };

int potTimeout = 300;

/////////////////////////////////////////////
// led
const int N_LED = 3;
byte ledPin[N_LED] = { 8, 9, 16 };

/////////////////////////////////////////////
// midi
byte midiCh = 0; // *Canal midi a ser utilizado
byte note = 1; // *Nota mais grave que sera utilizada
byte cc = 1; // *CC mais baixo que sera utilizado

/////////////////////////////////////////////
// Multiplexer
Multiplexer4067 mplex = Multiplexer4067(9, 8, 7, 6, A6);

void setup() {

  mplex.begin(); // inicializa o multiplexer
  
  for (int i = 0; i < NButtons; i++) { // inicializa os botoes como input utilizando o pull up resistor
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

  pinMode(A6, INPUT_PULLUP);
}



void loop() {

  buttons();
  potentiometers();
  readMidi();

}

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

  for (int i = 0; i < muxNButtons; i++) { //reads buttons on mux
    int buttonReading = mplex.readChannel(muxButtonPin[i]);
    //buttonCState[i] = map(mplex.readChannel(muxButtonPin[i]), 22, 1023, 0, 2); // stores on buttonCState
    if (buttonReading > 1000) {
      buttonCState[i] = HIGH;
    }
    else {
      buttonCState[i] = LOW;
    }
    //Serial.print(buttonCState[i]); Serial.print("   ");
  }
  //Serial.println();

  for (int i = 0; i < NButtons; i++) { //read buttons on Arduino
    buttonCState[i + muxNButtons] = digitalRead(buttonPin[i]); // stores in the rest of buttonCState
  }

  for (int i = 0; i < totalButtons; i++) {

    if ((millis() - lastDebounceTime) > debounceDelay) {

      if (buttonCState[i] != buttonPState[i]) {
        lastDebounceTime = millis();

        if (buttonCState[i] == LOW) {
          noteOn(midiCh, note + i, 127);  // Channel 0, middle C, normal velocity
          MidiUSB.flush();
          //MIDI.sendNoteOn(note + i, 127, potMidiCh()); // envia NoteOn(nota, velocity, canal midi)
          //Serial.print("Note: "); Serial.print(note + i); Serial.println(" On");
          buttonPState[i] = buttonCState[i];
        }
        else {
          noteOn(midiCh, note + i, 0);  // Channel 0, middle C, normal velocity
          MidiUSB.flush();
          //MIDI.sendNoteOn(note + i, 0, potMidiCh());
          //Serial.print("Note: "); Serial.print(note + i); Serial.println(" Off");
          buttonPState[i] = buttonCState[i];
        }
      }
    }
  }
}

void potentiometers() {

  for (int i = 0; i < NPots; i++) {
    potCState[i] = analogRead(potPin[i]); 
    midiState[i] = map(potCState[i], 0, 1020, 63.5, 127);

    int potVar = abs(potCState[i] - potPState[i]);

    if (potVar > varThreshold) {
      lastPot[i] = millis();
    }

    potTimer[i] = millis() - lastPot[i];

    if (potTimer[i] < potTimeout) {
      if (midiState[i] != midiPState[i]) { 
        
        controlChange(midiCh, cc + i, midiState[i]);
        MidiUSB.flush();

        midiPState[i] = midiState[i];
        potPState[i] = potCState[i];
      }
    }
  }
}

// Arduino (pro)micro midi functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOn);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}