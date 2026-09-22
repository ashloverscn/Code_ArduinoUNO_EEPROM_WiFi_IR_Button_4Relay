#include <ArduinoJson.h>

const uint8_t relayPins[4] = {
  13, 5, 6, 7
};

const uint8_t dimmerPin = 9;

bool relayStates[4] = {
  false,
  false,
  false,
  false
};

uint8_t dimmerValue = 0;

String inputBuffer;

void setup()
{
  Serial.begin(115200);

  for (uint8_t i = 0; i < 4; i++)
  {
    pinMode(relayPins[i], OUTPUT);
    relayStates[i] = false;
    digitalWrite(relayPins[i], LOW);
  }

  pinMode(dimmerPin, OUTPUT);
  analogWrite(dimmerPin, 0);

  inputBuffer.reserve(256);

  delay(500);

  sendReadyStatus();
}

void loop()
{
  readSerial();
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

        if (inputBuffer.startsWith("{") &&
            inputBuffer.endsWith("}"))
        {
          parseTasmotaCommand(inputBuffer);
        }

        inputBuffer = "";
      }

      continue;
    }

    if (inputBuffer.length() < 255)
    {
      inputBuffer += c;
    }
    else
    {
      inputBuffer = "";
    }
  }
}

void parseTasmotaCommand(const String &jsonStr)
{
  StaticJsonDocument<256> doc;

  DeserializationError error =
    deserializeJson(doc, jsonStr);

  if (error)
  {
    return;
  }

  bool commandReceived = false;

  for (uint8_t i = 0; i < 4; i++)
  {
    String powerKey =
      "POWER" + String(i + 1);

    if (doc.containsKey(powerKey))
    {
      const char *state = doc[powerKey];

      if (state == nullptr)
      {
        continue;
      }

      if (strcmp(state, "ON") == 0 ||
          strcmp(state, "1") == 0)
      {
        relayStates[i] = true;
        digitalWrite(relayPins[i], HIGH);
        commandReceived = true;
      }
      else if (strcmp(state, "OFF") == 0 ||
               strcmp(state, "0") == 0)
      {
        relayStates[i] = false;
        digitalWrite(relayPins[i], LOW);
        commandReceived = true;
      }
    }
  }

  if (doc.containsKey("Dimmer"))
  {
    int value = doc["Dimmer"];

    value = constrain(value, 0, 100);

    dimmerValue = (uint8_t)value;

    int pwmValue =
      map(dimmerValue, 0, 100, 0, 255);

    analogWrite(dimmerPin, pwmValue);

    commandReceived = true;
  }

  if (commandReceived)
  {
    sendStatusToTasmota();
  }
}

void sendReadyStatus()
{
  StaticJsonDocument<256> doc;

  doc["Device"] = "READY";
  doc["POWER1"] = "0";
  doc["POWER2"] = "0";
  doc["POWER3"] = "0";
  doc["POWER4"] = "0";
  doc["Dimmer"] = 0;

  serializeJson(doc, Serial);
  Serial.println();
}

void sendStatusToTasmota()
{
  StaticJsonDocument<256> doc;

  doc["POWER1"] =
    relayStates[0] ? "1" : "0";

  doc["POWER2"] =
    relayStates[1] ? "1" : "0";

  doc["POWER3"] =
    relayStates[2] ? "1" : "0";

  doc["POWER4"] =
    relayStates[3] ? "1" : "0";

  doc["Dimmer"] =
    dimmerValue;

  serializeJson(doc, Serial);
  Serial.println();
}