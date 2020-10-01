#include <evilOLED.h>

#include "BoardSupport.h"
#include "Encoder.h"

Encoder *encoders[8];
int values[8] = {0, 0, 0, 0, 0, 0, 0, 0};
evilOLED disp(19, 18); // init display

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
  Serial.begin(112500);
  while (!Serial)
    ;
  Serial.println("serial started");
    //init display

  
  

  for (char i = 0; i < 8; ++i)
  {
    encoders[i] = new Encoder();

    encoders[i]->onIncrement = [i]() -> void {
      values[i]++;
      Serial.printf("e%d  increment: %d \n", i, values[i]);
      // printAllValues();
    };
    encoders[i]->onDecrement = [i]() -> void {
      values[i]--;
      Serial.printf("e%d  decrement: %d \n", i, values[i]);
      // printAllValues();
    };
    encoders[i]->onClick = [i]() -> void {
      Serial.printf("e%d  click: %d \n", i, values[i]);
      // printAllValues();
    };
  }

  Serial.println("callbacks allocated");

  setupPorts();
  Serial.println("ports setup");
}

void printValue(int index, int value) {
  disp.cls(0x00);
  delay(500);
  disp.setCursor(5, 2); 
  disp.putString(index); // Strings MUST be double quoted and capitalized if using default font
  delay(500);

  disp.setCursor(5, 4);
  disp.putString(index);
  delay(500);
}

void loop()
{
    
  
 for (char j = 0; j < 8; ++j)
 {
   char i = j;
   sendBits(i);
   delayMicroseconds(100);
   bool changed = encoders[i]->setReading(digitalRead(E_A), digitalRead(E_B), digitalRead(E_C));

   if(changed) {
     printValue(i, values[i])
   }
 }

  //atualiza encoder 0
}
