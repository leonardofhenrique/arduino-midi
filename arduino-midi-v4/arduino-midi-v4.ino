#include <MIDIUSB.h>
#include <Multiplexer4067.h>   // Multiplexer CD4067 library >> https://github.com/sumotoy/Multiplexer4067
#include <Thread.h>            // Threads library >> https://github.com/ivanseidel/ArduinoThread
#include <ThreadController.h>  // Mesma lib de cima
#include <FastLED.h>

/*********DEBOUNCE*********/
unsigned long lastDebounceTime = 0;  // ultimo momento em que foi ativado
unsigned long debounceDelay = 5;     // tempo de debounce

/*********MULTIPLEXER*********/
Multiplexer4067 mplex = Multiplexer4067(9, 8, 7, 6, A6);  // (S1, S2, S3, S4, SIG)

/*********MIDI*********/
byte midiCh = 0;  // canal midi principal
byte note = 1;    // menor nota
byte cc = 1;      // menor CC

/*********BUTTONS*********/
const int muxNButtons = 16;     // numero de entradas no multiplexer
const int arduinoNButtons = 1;  // numero de entradas digitais
const int totalButtons = muxNButtons + arduinoNButtons;
const byte muxButtonPin[muxNButtons] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };  // ordem desejada dos pinos das portas do multiplexer
const byte buttonPin[arduinoNButtons] = { 15 };                                                   // ordem desejada dos pinos das portas do arduino
int buttonCState[totalButtons] = { 0 };                                                           // estado atual da porta digital
int buttonPState[totalButtons] = { 0 };                                                           // estado previo da porta digital

/*********POTENTIOMETERS*********/
const int NPots = 3;                        // numero de entradas analogicas
const byte potPin[NPots] = { A0, A1, A2 };  //ordem desejada dos pinos das portas do arduino
int potCState[NPots] = { 0 };               // estado atual da porta analogica
int potPState[NPots] = { 0 };               // estado previo da porta analogica
byte midiState[NPots] = { 0 };
byte midiPState[NPots] = { 0 };
int varThreshold = 20;

boolean openGate[NPots] = { false };

unsigned long lastPot[NPots] = { 0 };
unsigned long potTimer[NPots] = { 0 };

int potTimeout = 300;

/*********LEDS*********/
#define DATA_PIN 3
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS 20
CRGB leds[NUM_LEDS];
byte ledIndex[NUM_LEDS] = { 0, 1, 2, 3, 16, 4, 5, 6, 7, 17, 8, 9, 10, 11, 18, 12, 13, 14, 15, 19 };

#define BRIGHTNESS 25
#define FRAMES_PER_SECOND 120

int ledsColorCh0[NUM_LEDS][3] = {};
int ledsColorCh1[NUM_LEDS][3] = {};

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB(234, 95, 2));
  FastLED.show();  // Inicializa os leds

  mplex.begin();  // inicializa o multiplexer

  pinMode(10, INPUT_PULLUP);
  pinMode(A6, INPUT_PULLUP);

  for (int i = 0; i < arduinoNButtons; i++) {  // inicializa os botoes como input utilizando o pull up resistor
    pinMode(buttonPin[i], INPUT_PULLUP);
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    for (int j = 0; j < 3; j++) {
      ledsColorCh0[i][j] = 0;
      ledsColorCh1[i][j] = 0;
    }
  }
}

void loop() {
  buttons();
  potentiometers();
  readMidi();
  changeChannel();
}

void changeChannel() {
  if (digitalRead(10) == LOW) {
    midiCh = (midiCh + 1) % 2;
    // Serial.print("Channel: ");
    // Serial.println(midiCh);
    changeLEDcolorChannel();
    delay(500);
  }
}

void changeLEDcolorChannel() {
  if (midiCh == 0) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].setRGB(ledsColorCh0[i][0], ledsColorCh0[i][1], ledsColorCh0[i][2]);
    }
  } else {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].setRGB(ledsColorCh1[i][0], ledsColorCh1[i][1], ledsColorCh1[i][2]);
    }
  }
}

void readMidi() {
  midiEventPacket_t message;
  do {
    message = MidiUSB.read();
    // if (message.header != 0) {
    //   Serial.print("Mensagem MIDI: ");
    //   Serial.print(message.header);
    //   Serial.print(" - ");
    //   Serial.print(message.byte1);
    //   Serial.print(" - ");
    //   Serial.print(message.byte2);
    //   Serial.print(" - ");
    //   Serial.println(message.byte3);
    // }

    int botao = 100;
    if (message.byte2 == 20) {
      botao = 0;
    } else if (message.byte2 == 0) {
    } else if (message.byte2 == 127) {

      fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
      FastLED.show();

    } else if (message.byte2 == 126) {
      changeLEDcolorChannel();
    } else {
      botao = message.byte2;
    }

    int corR = 0;
    int corG = 0;
    int corB = 0;

    switch (message.byte3) {
      case 0:  //keep
        corR = 0;
        corG = 0;
        corB = 0;
        break;
      case 1:  //keep
        corR = 255;
        corG = 255;
        corB = 255;
        break;
      case 2:  //keep
        corR = 255;
        corG = 0;
        corB = 0;
        break;
      case 3:  //keep
        corR = 0;
        corG = 0;
        corB = 255;
        break;
      case 4:  //keep
        corR = 0;
        corG = 255;
        corB = 0;
        break;
      case 5:  //keep
        corR = 74;
        corG = 239;
        corB = 170;
        break;
      case 6:  //keep
        corR = 234;
        corG = 95;
        corB = 2;
        break;
      case 7:  //keep
        corR = 179;
        corG = 193;
        corB = 0;
        break;
      case 8:  //keep
        corR = 76;
        corG = 181;
        corB = 245;
        break;
      case 9:  //keep
        corR = 232;
        corG = 46;
        corB = 105;
        break;
    }

    leds[botao].setRGB(corR, corG, corB);
    FastLED.show();

    if (midiCh == 0) {
      ledsColorCh0[botao][0] = corR;
      ledsColorCh0[botao][1] = corG;
      ledsColorCh0[botao][2] = corB;
    }
    if (midiCh == 1) {
      ledsColorCh1[botao][0] = corR;
      ledsColorCh1[botao][1] = corG;
      ledsColorCh1[botao][2] = corB;
    }

  } while (message.header != 0);
}

void buttons() {

  for (int i = 0; i < muxNButtons; i++) {
    int buttonReading = mplex.readChannel(muxButtonPin[i]);
    if (buttonReading > 1000) {
      buttonCState[i] = HIGH;
    } else {
      buttonCState[i] = LOW;
    }
  }

  for (int i = 0; i < arduinoNButtons; i++) {
    buttonCState[i + muxNButtons] = digitalRead(buttonPin[i]);
  }

  for (int i = 0; i < totalButtons; i++) {

    if ((millis() - lastDebounceTime) > debounceDelay) {

      if (buttonCState[i] != buttonPState[i]) {
        lastDebounceTime = millis();

        if (buttonCState[i] == LOW) {
          noteOn(midiCh, note + i, 127);
          MidiUSB.flush();
          // Serial.print("Channel: ");
          // Serial.print(midiCh);
          // Serial.print(" Note: ");
          // Serial.print(note + i);
          // Serial.println(" On");
          buttonPState[i] = buttonCState[i];
        } else {
          noteOn(midiCh, note + i, 0);
          MidiUSB.flush();
          // Serial.print("Channel: ");
          // Serial.print(midiCh);
          // Serial.print(" Note: ");
          // Serial.print(note + i);
          // Serial.println(" Off");
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

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOn);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}