#ifdef JUNOG_DEBUGGER
#include <Arduino.h>
#include "test_datas.h"

#ifndef DEBUG_DELAY_US
#define DEBUG_DELAY_US 500
#endif

inline void writeData(uint16_t data)
{
  for (int i = 0; i < 8; i++)
  {
    digitalWriteFast(JUNO_D0 + i, bitRead(data, i));
  }
  digitalWriteFast(JUNO_RS, bitRead(data, RS_BIT));
}

void setup()
{
  // Serial setup
  Serial.begin(115200);
  while (!Serial)
  {
  }
  delay(100);
  Serial.println("JUNO LCD Tester");

  // pin configuration
  for (int i = 0; i < 8; i++)
  {
    pinMode(JUNO_D0 + i, OUTPUT);
  }
  pinMode(JUNO_RS, OUTPUT);
  pinMode(JUNO_CS1, OUTPUT);
  pinMode(JUNO_CS2, OUTPUT);
}

void loop()
{
  Serial.println("Start testing");
  for (int i = 0; i < SAMPLE_SIZE; i++)
  {
    if (bitRead(sample_datas[i], CS1_BIT))
    {
      writeData(sample_datas[i]);
      digitalWriteFast(JUNO_CS1, HIGH);
      delayMicroseconds(100);
      digitalWriteFast(JUNO_CS1, LOW);
    }
    else if (bitRead(sample_datas[i], CS2_BIT))
    {
      writeData(sample_datas[i]);
      digitalWriteFast(JUNO_CS2, HIGH);
      delayMicroseconds(100);
      digitalWriteFast(JUNO_CS2, LOW);
    }
    else
    {
      Serial.println("Invalid CS1/CS2 bit");
    }
    delayMicroseconds(DEBUG_DELAY_US);
  }
  Serial.println("End testing. Restarting in 5 seconds.");
  delay(5000);
}
#endif