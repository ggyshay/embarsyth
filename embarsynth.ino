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

  // for (char i = 0; i < 7; ++i)
  // {
  //   encoders[i] = new Encoder();

  //   encoders[i]->onIncrement = [i]() -> void {
  //     values[i]++;
  //     Serial.printf("e%d  increment: %d \n", i, values[i]);
  //     disp->putScreen(encoder_names[i], (float)values[i]);
  //     printAllValues();
  //   };
  //   encoders[i]->onDecrement = [i]() -> void {
  //     values[i]--;
  //     Serial.printf("e%d  decrement: %d \n", i, values[i]);
  //     disp->putScreen(encoder_names[i], (float)values[i]);
  //     printAllValues();
  //   };
  //   encoders[i]->onClick = [i]() -> void {
  //     Serial.printf("e%d  click: %d \n", i, values[i]);
  //     disp->putScreen(encoder_names[i], (float)values[i]);
  //     printAllValues();
  //   };
  // }

  // encoders[7] = new Encoder();
  // encoders[7]->onIncrement = []() -> void {
  //   globalVolume->increment();
  //   Serial.printf("e%d  increment: %d \n", 7, values[7]);
  //   disp->putScreen(globalVolume->nameTag, globalVolume->value);
  //   audioInfra.setVolume(globalVolume->value);
  // };
  // encoders[7]->onDecrement = []() -> void {
  //   globalVolume->decrement();
  //   Serial.printf("e%d  decrement: %d \n", 7, values[7]);
  //   disp->putScreen(globalVolume->nameTag, globalVolume->value);
  //   audioInfra.setVolume(globalVolume->value);
  // };

  //SETUP ENCODERS
  for (char i = 0; i < 8; ++i)
  {
    encoders[i] = new Encoder();

    encoders[i]->onIncrement = [i]() -> void {
      Value *v = audioInfra.getCurrentValue(i);
      if (v == nullptr)
        return;
      v->increment();
      Serial.printf("e%d  increment: %f \n", i, v->value);
      disp->putScreen(v->nameTag, v->value);
      audioInfra.updateIList(i);
    };
    encoders[i]->onDecrement = [i]() -> void {
      Value *v = audioInfra.getCurrentValue(i);
      if (v == nullptr)
        return;
      v->decrement();
      Serial.printf("e%d  increment: %f \n", i, v->value);
      disp->putScreen(v->nameTag, v->value);
      audioInfra.updateIList(i);
    };
    encoders[i]->onClick = [i]() -> void {
      Value *v = audioInfra.getNextValue(i);
      if (v == nullptr)
        return;
      Serial.printf("e%d  click: %f \n", i, v->value);
      disp->putScreen(v->nameTag, v->value);
    };
  }

  //SETUP MIDI CALLBACKS
  usbMIDI.setHandleNoteOn(handleNoteOn);
  // usbMIDI.setHandleNoteOff(handleNoteOff);

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
  usbMIDI.read();
  for (char j = 0; j < 8; ++j)
  {
    char i = j;
    sendBits(i);
    delayMicroseconds(100);
    encoders[i]->setReading(digitalRead(E_A), digitalRead(E_B), digitalRead(E_C));
  }
}

void handleNoteOn(byte channel, byte note, byte velocity)
{
  // Serial.printf("note on %d %d %d\n", channel, note, velocity);
  audioInfra.handleNoteOn(channel, note, velocity);
}