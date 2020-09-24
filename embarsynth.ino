#include "BoardSupport.h"
#include "Encoder.h"

Encoder *encoders[8];
int values[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void printAllValues()
{
  for (char i = 0; i < 8; ++i)
  {
    Serial.printf("%d ", i);
  }
  Serial.println();
}

void setup()
{
  Serial.begin(112500);
  for (char i = 0; i < 8; ++i)
  {
    encoders[i] = new Encoder();

    encoders[i]->onIncrement = [i]() -> void {
      values[i]++;
      Serial.printf("e%d  increment: %d \n", i, values[i]);
      printAllValues();
    };
    encoders[i]->onDecrement = [i]() -> void {
      values[i]--;
      Serial.printf("e%d  decrement: %d \n", i, values[i]);
      printAllValues();
    };
    encoders[i]->onClick = [i]() -> void {
      Serial.printf("e%d  click: %d \n", i, values[i]);
      printAllValues();
    };
  }
}

void loop()
{
  for (char i = 0; i < 8; ++i)
  {
    sendBits(0);
    delayMicroseconds(100);
    encoders[i]->setReading(digitalRead(E_A), digitalRead(E_B), digitalRead(E_C));
  }

  //atualiza encoder 0
}
