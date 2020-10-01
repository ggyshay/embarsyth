#include "BoardSupport.h"
#include "Encoder.h"

Encoder *encoders[8];
int values[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void printAllValues()
{
  for (char i = 0; i < 8; ++i)
  {
    Serial.printf("%d ", values[i]);
  }
  Serial.println();
  Serial.println();
}

void setup()
{
  Serial.begin(112500);
  while (!Serial)
    ;
  Serial.println("serial started");

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

  Serial.println("callbacks allocated");

  setupPorts();
  Serial.println("ports setup");
}

void loop()
{
  for (char j = 0; j < 8; ++j)
  {
    char i = j;
    sendBits(i);
    delayMicroseconds(100);
    encoders[i]->setReading(digitalRead(E_A), digitalRead(E_B), digitalRead(E_C));
  }

  //atualiza encoder 0
}
