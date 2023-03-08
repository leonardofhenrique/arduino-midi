
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
      leds[i].setRGB(LEDS_CH_0[i][0], LEDS_CH_0[i][1], LEDS_CH_0[i][2]);
    }
  } else {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].setRGB(LEDS_CH_1[i][0], LEDS_CH_1[i][1], LEDS_CH_1[i][2]);
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
      LEDS_CH_0[botao][0] = corR;
      LEDS_CH_0[botao][1] = corG;
      LEDS_CH_0[botao][2] = corB;
    }
    if (midiCh == 1) {
      LEDS_CH_1[botao][0] = corR;
      LEDS_CH_1[botao][1] = corG;
      LEDS_CH_1[botao][2] = corB;
    }

  } while (message.header != 0);
}



void buttons() {

  for (int i = 0; i < muxNButtons; i++) {  //reads buttons on mux
    int buttonReading = mplex.readChannel(muxButtonPin[i]);
    if (buttonReading > 1000) {
      buttonCState[i] = HIGH;
    } else {
      buttonCState[i] = LOW;
    }
  }

  for (int i = 0; i < arduinoNButtons; i++) {                   //read buttons on Arduino
    buttonCState[i + muxNButtons] = digitalRead(buttonPin[i]);  // stores in the rest of buttonCState
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

// Arduino (pro)micro midi functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOn);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}