#include "BoardSupport.h"
#include "Encoder.h"
#include "DisplayDriver.h"
#include "AudioInfra.h"
#include "ValueInterface.h"

Encoder *encoders[8];
DisplayDriver *disp;
AudioInfra audioInfra;
int values[8] = {0, 0, 0, 0, 0, 0, 0, 0};
const char *encoder_names[8] = {"ENC1", "ENC2", "ENC3", "ENC4", "ENC5", "ENC6", "ENC7", "ENC8"};
Value *globalVolume = new Value(0.0, 1.0, 0.8, "VOLUME", 40, false);

void printAllValues()
{
  for (char i = 0; i < 8; ++i)
  {
    Serial.printf("%d ", values[i]);
  }
  Serial.println();
}

void setup()
{
  //setup serial
  Serial.begin(112500);
  while (!Serial)
    ;
  Serial.println("serial started");

  //SETUP ENCODERS
  for (char i = 0; i < 8; ++i)
  {
    encoders[i] = new Encoder();

    // Setup increment
    encoders[i]->onIncrement = [i]() -> void {
      // Gets respective value in AudioInfra
      Value *v = audioInfra.getCurrentValue(i);

      if (v == nullptr)
        return;

      // increments value
      v->increment();

      // Display and serial info print
      Serial.printf("e%d  increment: %f \n", i, v->value);
      disp->putScreen(v->nameTag, v->value);

      // Actually updates sound
      audioInfra.updateIList(i);
    };

    // Setup decrement
    encoders[i]->onDecrement = [i]() -> void {
      Value *v = audioInfra.getCurrentValue(i);
      if (v == nullptr)
        return;
      v->decrement();
      Serial.printf("e%d  increment: %f \n", i, v->value);
      disp->putScreen(v->nameTag, v->value);
      audioInfra.updateIList(i);
    };

    // Setup click
    encoders[i]->onClick = [i]() -> void {
      // Get and updates selected value for i in AudioInfra
      Value *v = audioInfra.getNextValue(i);

      if (v == nullptr)
        return;

      Serial.printf("e%d  click: %f \n", i, v->value);
      disp->putScreen(v->nameTag, v->value);
    };
  }

  //SETUP MIDI CALLBACKS
  usbMIDI.setHandleNoteOn(handleNoteOn);
  usbMIDI.setHandleNoteOff(handleNoteOff);

  Serial.println("callbacks allocated");

  //SETUP PORT DIRECTIONS
  setupPorts();
  Serial.println("ports setup");

  //SETUP AUDIO INFRA
  audioInfra.begin();
  Serial.println("audioInfra initialized");

  //SETUP DISPLAY
  disp->init();
  disp->putScreen("0X303", "BEGIN");
  Serial.println("display initialized");
  Serial.println("all setup finished");
}

void loop()
{
  // Reads MIDI info from PC
  usbMIDI.read();

  // Iterates encoders
  for (char j = 0; j < 8; ++j)
  {
    char i = j;
    sendBits(i); // Toggle encoder
    delayMicroseconds(100);

    // Update encoder reading, which calls callbacks
    encoders[i]->setReading(digitalRead(E_A), digitalRead(E_B), digitalRead(E_C));
  }
}

// Callbacks called when note is pressed or released
void handleNoteOn(byte channel, byte note, byte velocity)
{
  audioInfra.handleNoteOn(channel, note, velocity);
}

void handleNoteOff(byte channel, byte note, byte velocity){
  audioInfra.handleNoteOff();
}