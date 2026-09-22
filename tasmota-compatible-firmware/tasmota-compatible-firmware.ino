#include <EEPROM.h>
#include <AceButton.h>
#include <IRremote.h>

using namespace ace_button;

const uint8_t relayPins[4] = {
  13, 5, 6, 7
};

const uint8_t dimmerPin = 9;

const uint8_t buttonPins[6] = {
  A0, A1, A2, A3, A4, A5
};

const uint8_t irPin = 3;

#define IR_Button_1   0x1FE50AF
#define IR_Button_2   0x1FED827
#define IR_Button_3   0x1FEF807
#define IR_Button_4   0x1FE30CF
#define IR_Button_5   0x1FEB04F
#define IR_Button_Up  0x1FE609F
#define IR_Button_Dn  0x1FEA05F
#define IR_All_Off    0x1FE7887
#define IR_All_On     0x1FE48B7

bool relayStates[4] = {
  false,
  false,
  false,
  false
};

uint8_t dimmerValue = 0;

String inputBuffer;

ButtonConfig config1;
ButtonConfig config2;
ButtonConfig config3;
ButtonConfig config4;
ButtonConfig config5;
ButtonConfig config6;

AceButton button1(&config1);
AceButton button2(&config2);
AceButton button3(&config3);
AceButton button4(&config4);
AceButton button5(&config5);
AceButton button6(&config6);

IRrecv irrecv(irPin);
decode_results results;

void buttonHandler(AceButton* button, uint8_t eventType, uint8_t buttonState);

void setup()
{
  Serial.begin(115200);

  for (uint8_t i = 0; i < 4; i++)
  {
    pinMode(relayPins[i], OUTPUT);

    byte savedState = EEPROM.read(i);

    if (savedState > 1)
    {
      savedState = 0;
      EEPROM.update(i, 0);
    }

    relayStates[i] = savedState;

    digitalWrite(
      relayPins[i],
      relayStates[i] ? HIGH : LOW
    );
  }

  pinMode(dimmerPin, OUTPUT);

  byte savedDimmer = EEPROM.read(10);

  if (savedDimmer > 100)
  {
    savedDimmer = 0;
    EEPROM.update(10, 0);
  }

  dimmerValue = savedDimmer;

  analogWrite(
    dimmerPin,
    map(dimmerValue, 0, 100, 0, 255)
  );

  config1.setEventHandler(buttonHandler);
  config2.setEventHandler(buttonHandler);
  config3.setEventHandler(buttonHandler);
  config4.setEventHandler(buttonHandler);
  config5.setEventHandler(buttonHandler);
  config6.setEventHandler(buttonHandler);

  button1.init(buttonPins[0]);
  button2.init(buttonPins[1]);
  button3.init(buttonPins[2]);
  button4.init(buttonPins[3]);
  button5.init(buttonPins[4]);
  button6.init(buttonPins[5]);

  irrecv.enableIRIn();

  inputBuffer.reserve(128);
}

void loop()
{
  readSerial();

  button1.check();
  button2.check();
  button3.check();
  button4.check();
  button5.check();
  button6.check();

  irRemote();
}

void readSerial()
{
  while (Serial.available())
  {
    char c = Serial.read();

    if (c == '\n' || c == '\r')
    {
      if (inputBuffer.length() > 0)
      {
        inputBuffer.trim();
        processCommand(inputBuffer);
        inputBuffer = "";
      }
    }
    else
    {
      if (inputBuffer.length() < 127)
      {
        inputBuffer += c;
      }
      else
      {
        inputBuffer = "";
      }
    }
  }
}

void processCommand(String command)
{
  if (command.indexOf("\"POWER1\":\"ON\"") >= 0)
  {
    relayStates[0] = true;
    digitalWrite(relayPins[0], HIGH);
    EEPROM.update(0, 1);
    Serial.println("{\"POWER1\":\"1\"}");
  }

  if (command.indexOf("\"POWER1\":\"OFF\"") >= 0)
  {
    relayStates[0] = false;
    digitalWrite(relayPins[0], LOW);
    EEPROM.update(0, 0);
    Serial.println("{\"POWER1\":\"0\"}");
  }

  if (command.indexOf("\"POWER2\":\"ON\"") >= 0)
  {
    relayStates[1] = true;
    digitalWrite(relayPins[1], HIGH);
    EEPROM.update(1, 1);
    Serial.println("{\"POWER2\":\"1\"}");
  }

  if (command.indexOf("\"POWER2\":\"OFF\"") >= 0)
  {
    relayStates[1] = false;
    digitalWrite(relayPins[1], LOW);
    EEPROM.update(1, 0);
    Serial.println("{\"POWER2\":\"0\"}");
  }

  if (command.indexOf("\"POWER3\":\"ON\"") >= 0)
  {
    relayStates[2] = true;
    digitalWrite(relayPins[2], HIGH);
    EEPROM.update(2, 1);
    Serial.println("{\"POWER3\":\"1\"}");
  }

  if (command.indexOf("\"POWER3\":\"OFF\"") >= 0)
  {
    relayStates[2] = false;
    digitalWrite(relayPins[2], LOW);
    EEPROM.update(2, 0);
    Serial.println("{\"POWER3\":\"0\"}");
  }

  if (command.indexOf("\"POWER4\":\"ON\"") >= 0)
  {
    relayStates[3] = true;
    digitalWrite(relayPins[3], HIGH);
    EEPROM.update(3, 1);
    Serial.println("{\"POWER4\":\"1\"}");
  }

  if (command.indexOf("\"POWER4\":\"OFF\"") >= 0)
  {
    relayStates[3] = false;
    digitalWrite(relayPins[3], LOW);
    EEPROM.update(3, 0);
    Serial.println("{\"POWER4\":\"0\"}");
  }

  int dimmerIndex = command.indexOf("\"Dimmer\":");

  if (dimmerIndex >= 0)
  {
    int valueStart = dimmerIndex + 9;

    while (valueStart < command.length() &&
           command.charAt(valueStart) == ' ')
    {
      valueStart++;
    }

    int value = command.substring(valueStart).toInt();

    value = constrain(value, 0, 100);

    dimmerValue = value;

    analogWrite(
      dimmerPin,
      map(dimmerValue, 0, 100, 0, 255)
    );

    EEPROM.update(10, dimmerValue);

    Serial.print("{\"Dimmer\":");
    Serial.print(dimmerValue);
    Serial.println("}");
  }
}

void irRemote()
{
  if (irrecv.decode(&results))
  {
    unsigned long code = results.value;

    if (code == IR_Button_1)
    {
      toggleRelay(0);
    }
    else if (code == IR_Button_2)
    {
      toggleRelay(1);
    }
    else if (code == IR_Button_3)
    {
      toggleRelay(2);
    }
    else if (code == IR_Button_4)
    {
      toggleRelay(3);
    }
    else if (code == IR_Button_5)
    {
      if (dimmerValue == 0)
      {
        setDimmer(100);
      }
      else
      {
        setDimmer(0);
      }
    }
    else if (code == IR_Button_Up)
    {
      if (dimmerValue < 100)
      {
        setDimmer(dimmerValue + 1);
      }
    }
    else if (code == IR_Button_Dn)
    {
      if (dimmerValue > 0)
      {
        setDimmer(dimmerValue - 1);
      }
    }
    else if (code == IR_All_On)
    {
      allSwitchOn();
    }
    else if (code == IR_All_Off)
    {
      allSwitchOff();
    }

    irrecv.resume();
  }
}

void toggleRelay(uint8_t index)
{
  relayStates[index] = !relayStates[index];

  digitalWrite(
    relayPins[index],
    relayStates[index] ? HIGH : LOW
  );

  EEPROM.update(
    index,
    relayStates[index] ? 1 : 0
  );

  Serial.print("{\"POWER");
  Serial.print(index + 1);
  Serial.print("\":\"");
  Serial.print(relayStates[index] ? "1" : "0");
  Serial.println("\"}");
}

void setDimmer(uint8_t value)
{
  dimmerValue = constrain(value, 0, 100);

  analogWrite(
    dimmerPin,
    map(dimmerValue, 0, 100, 0, 255)
  );

  EEPROM.update(10, dimmerValue);

  Serial.print("{\"Dimmer\":");
  Serial.print(dimmerValue);
  Serial.println("}");
}

void allSwitchOn()
{
  for (uint8_t i = 0; i < 4; i++)
  {
    relayStates[i] = true;
    digitalWrite(relayPins[i], HIGH);
    EEPROM.update(i, 1);
  }

  Serial.println("{\"POWER1\":\"1\",\"POWER2\":\"1\",\"POWER3\":\"1\",\"POWER4\":\"1\"}");
}

void allSwitchOff()
{
  for (uint8_t i = 0; i < 4; i++)
  {
    relayStates[i] = false;
    digitalWrite(relayPins[i], LOW);
    EEPROM.update(i, 0);
  }

  Serial.println("{\"POWER1\":\"0\",\"POWER2\":\"0\",\"POWER3\":\"0\",\"POWER4\":\"0\"}");
}

void buttonHandler(
  AceButton* button,
  uint8_t eventType,
  uint8_t buttonState)
{
  if (eventType != AceButton::kEventPressed)
  {
    return;
  }

  uint8_t pin = button->getPin();

  if (pin == A0)
  {
    toggleRelay(0);
  }
  else if (pin == A1)
  {
    toggleRelay(1);
  }
  else if (pin == A2)
  {
    toggleRelay(2);
  }
  else if (pin == A3)
  {
    toggleRelay(3);
  }
  else if (pin == A4)
  {
    if (dimmerValue > 0)
    {
      setDimmer(dimmerValue - 1);
    }
  }
  else if (pin == A5)
  {
    if (dimmerValue < 100)
    {
      setDimmer(dimmerValue + 1);
    }
  }
}