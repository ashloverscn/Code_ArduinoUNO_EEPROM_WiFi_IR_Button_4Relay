#include <EEPROM.h>
#include <AceButton.h>
#include <IRremote.h>
#include <arduino-timer.h>
#include <atmega328_16mhz_ac_phase_control.h>

using namespace ace_button;

const uint16_t spd[14] = {
  600, 500, 480, 450, 400, 380, 350,
  300, 250, 180, 150, 80, 50, 1
};

auto timer = timer_create_default();

#define RelayPin1 5
#define RelayPin2 6
#define RelayPin3 7
#define RelayPin4 8
#define TriacPin 9

#define SwitchPin1 A0
#define SwitchPin2 A1
#define SwitchPin3 A2
#define SwitchPin4 A3
#define SwitchPin5 A4
#define SwitchPin6 A5

#define ZeroCrossPin 2
#define IR_RECV_PIN 3

#define IR_Button_1    0xF50A7F80
#define IR_Button_2    0xE41B7F80
#define IR_Button_3    0xE01F7F80
#define IR_Button_4    0xF30C7F80
#define IR_Button_5    0xF20D7F80
#define IR_Button_Up   0xF9067F80
#define IR_Button_Dn   0xFA057F80
#define IR_All_Off     0xED127F80
#define IR_All_On      0xE11E7F80

#define EEPROM_RELAY1      0
#define EEPROM_RELAY2      1
#define EEPROM_RELAY3      2
#define EEPROM_RELAY4      3
#define EEPROM_DIMMER      7
#define EEPROM_TRIAC_STATE 8

#define DIMMER_MIN 0
#define DIMMER_MAX 13

String pinStatus = "0000";
String inputBuffer;

uint8_t dimm_value = 0;
bool triacState = false;

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

void button1Handler(AceButton*, uint8_t, uint8_t);
void button2Handler(AceButton*, uint8_t, uint8_t);
void button3Handler(AceButton*, uint8_t, uint8_t);
void button4Handler(AceButton*, uint8_t, uint8_t);
void button5Handler(AceButton*, uint8_t, uint8_t);
void button6Handler(AceButton*, uint8_t, uint8_t);

void all_Switch_ON();
void all_Switch_OFF();
void sendTasmotaStatus(int relayNum, bool state);

void applyDimmer()
{
  if (triacState)
  {
    atmega328_16mhz_ac_phase_control.set_ac_power(spd[dimm_value]);
  }
  else
  {
    atmega328_16mhz_ac_phase_control.set_ac_power(0);
  }
}

void dimm_Up()
{
  if (!triacState)
  {
    return;
  }

  if (dimm_value < DIMMER_MAX)
  {
    dimm_value++;
  }

  applyDimmer();
  EEPROM.update(EEPROM_DIMMER, dimm_value);
}

void dimm_Dn()
{
  if (!triacState)
  {
    return;
  }

  if (dimm_value > DIMMER_MIN)
  {
    dimm_value--;
  }

  applyDimmer();
  EEPROM.update(EEPROM_DIMMER, dimm_value);
}

void setDimmerLevel(uint8_t val)
{
  if (val > DIMMER_MAX) val = DIMMER_MAX;
  dimm_value = val;
  triacState = (dimm_value > 0);
  applyDimmer();
  EEPROM.update(EEPROM_DIMMER, dimm_value);
  EEPROM.update(EEPROM_TRIAC_STATE, triacState ? 1 : 0);
}

void triacOn()
{
  triacState = true;

  if (dimm_value > DIMMER_MAX)
  {
    dimm_value = DIMMER_MAX;
  }

  applyDimmer();
  EEPROM.update(EEPROM_DIMMER, dimm_value);
  EEPROM.update(EEPROM_TRIAC_STATE, 1);
}

void triacOff()
{
  triacState = false;
  atmega328_16mhz_ac_phase_control.set_ac_power(0);
  EEPROM.update(EEPROM_TRIAC_STATE, 0);
}

void triacOnOff()
{
  if (triacState)
  {
    triacOff();
  }
  else
  {
    triacOn();
  }
}

// Active-Low logic: turnOn = true sets pin LOW (Relay ON)
void setRelayState(int relay, bool turnOn)
{
  int pin = 0;
  int eepromAddress = 0;

  switch (relay)
  {
    case 1: pin = RelayPin1; eepromAddress = EEPROM_RELAY1; break;
    case 2: pin = RelayPin2; eepromAddress = EEPROM_RELAY2; break;
    case 3: pin = RelayPin3; eepromAddress = EEPROM_RELAY3; break;
    case 4: pin = RelayPin4; eepromAddress = EEPROM_RELAY4; break;
    default: return;
  }

  digitalWrite(pin, turnOn ? LOW : HIGH);
  EEPROM.update(eepromAddress, turnOn ? HIGH : LOW);
  sendTasmotaStatus(relay, turnOn);
  delay(50);
}

void relayToggle(int relay)
{
  int pin = 0;
  switch (relay)
  {
    case 1: pin = RelayPin1; break;
    case 2: pin = RelayPin2; break;
    case 3: pin = RelayPin3; break;
    case 4: pin = RelayPin4; break;
    default: return;
  }

  bool currentState = (digitalRead(pin) == LOW); // Currently ON if LOW
  setRelayState(relay, !currentState);
}

void sendTasmotaStatus(int relayNum, bool state)
{
  Serial.print("{\"POWER");
  Serial.print(relayNum);
  Serial.print("\":\"");
  Serial.print(state ? "ON" : "OFF");
  Serial.println("\"}");
}

void eepromState()
{
  uint8_t relay1 = EEPROM.read(EEPROM_RELAY1);
  uint8_t relay2 = EEPROM.read(EEPROM_RELAY2);
  uint8_t relay3 = EEPROM.read(EEPROM_RELAY3);
  uint8_t relay4 = EEPROM.read(EEPROM_RELAY4);

  if (relay1 > 1) relay1 = LOW;
  if (relay2 > 1) relay2 = LOW;
  if (relay3 > 1) relay3 = LOW;
  if (relay4 > 1) relay4 = LOW;

  digitalWrite(RelayPin1, relay1 == HIGH ? LOW : HIGH);
  delay(50);
  digitalWrite(RelayPin2, relay2 == HIGH ? LOW : HIGH);
  delay(50);
  digitalWrite(RelayPin3, relay3 == HIGH ? LOW : HIGH);
  delay(50);
  digitalWrite(RelayPin4, relay4 == HIGH ? LOW : HIGH);
  delay(50);

  uint8_t storedDimmer = EEPROM.read(EEPROM_DIMMER);
  if (storedDimmer <= DIMMER_MAX)
  {
    dimm_value = storedDimmer;
  }
  else
  {
    dimm_value = DIMMER_MAX;
  }

  uint8_t storedTriacState = EEPROM.read(EEPROM_TRIAC_STATE);
  if (storedTriacState == 1)
  {
    triacState = true;
  }
  else
  {
    triacState = false;
  }

  applyDimmer();
}

void ir_remote()
{
  if (IrReceiver.decode())
  {
    uint32_t code = IrReceiver.decodedIRData.decodedRawData;

    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT))
    {
      switch (code)
      {
        case IR_Button_1:
          relayToggle(1);
          break;
        case IR_Button_2:
          relayToggle(2);
          break;
        case IR_Button_3:
          relayToggle(3);
          break;
        case IR_Button_4:
          relayToggle(4);
          break;
        case IR_Button_5:
          triacOnOff();
          break;
        case IR_Button_Up:
          dimm_Up();
          break;
        case IR_Button_Dn:
          dimm_Dn();
          break;
        case IR_All_Off:
          all_Switch_OFF();
          break;
        case IR_All_On:
          all_Switch_ON();
          break;
        default:
          break;
      }
    }
    IrReceiver.resume();
  }
}

void all_Switch_ON()
{
  setRelayState(1, true);
  setRelayState(2, true);
  setRelayState(3, true);
  setRelayState(4, true);
}

void all_Switch_OFF()
{
  setRelayState(1, false);
  setRelayState(2, false);
  setRelayState(3, false);
  setRelayState(4, false);
}

void sendStatus()
{
  pinStatus = String(digitalRead(RelayPin1) == LOW ? "1" : "0") +
              String(digitalRead(RelayPin2) == LOW ? "1" : "0") +
              String(digitalRead(RelayPin3) == LOW ? "1" : "0") +
              String(digitalRead(RelayPin4) == LOW ? "1" : "0");
}

void handleSerialControl()
{
  while (Serial.available() > 0)
  {
    char incomingChar = (char)Serial.read();

    if (incomingChar == '\n' || incomingChar == '\r')
    {
      inputBuffer.trim();
      inputBuffer.toUpperCase();

      if (inputBuffer.length() > 0)
      {
        // JSON format support (e.g., {"POWER3":"OFF"})
        if (inputBuffer.startsWith("{") && inputBuffer.endsWith("}"))
        {
          for (int i = 1; i <= 4; i++)
          {
            String targetOn = "{\"POWER" + String(i) + "\":\"ON\"}";
            String targetOff = "{\"POWER" + String(i) + "\":\"OFF\"}";

            if (inputBuffer == targetOn)
            {
              setRelayState(i, true);
              break;
            }
            else if (inputBuffer == targetOff)
            {
              setRelayState(i, false);
              break;
            }
          }
        }
        // Standard text commands
        else if (inputBuffer == "POWER1 ON" || inputBuffer == "R1_ON")
        {
          setRelayState(1, true);
        }
        else if (inputBuffer == "POWER1 OFF" || inputBuffer == "R1_OFF")
        {
          setRelayState(1, false);
        }
        else if (inputBuffer == "POWER2 ON" || inputBuffer == "R2_ON")
        {
          setRelayState(2, true);
        }
        else if (inputBuffer == "POWER2 OFF" || inputBuffer == "R2_OFF")
        {
          setRelayState(2, false);
        }
        else if (inputBuffer == "POWER3 ON" || inputBuffer == "R3_ON")
        {
          setRelayState(3, true);
        }
        else if (inputBuffer == "POWER3 OFF" || inputBuffer == "R3_OFF")
        {
          setRelayState(3, false);
        }
        else if (inputBuffer == "POWER4 ON" || inputBuffer == "R4_ON")
        {
          setRelayState(4, true);
        }
        else if (inputBuffer == "POWER4 OFF" || inputBuffer == "R4_OFF")
        {
          setRelayState(4, false);
        }
        else if (inputBuffer.startsWith("DIMMER "))
        {
          int val = inputBuffer.substring(7).toInt();
          setDimmerLevel(val);
        }
        else if (inputBuffer == "ALL_ON" || inputBuffer == "POWER ALL ON")
        {
          all_Switch_ON();
        }
        else if (inputBuffer == "ALL_OFF" || inputBuffer == "POWER ALL OFF")
        {
          all_Switch_OFF();
        }
        else if (inputBuffer == "STATUS")
        {
          sendStatus();
          Serial.println(pinStatus);
        }
      }

      inputBuffer = "";
    }
    else
    {
      inputBuffer += incomingChar;
    }
  }
}

void setup()
{
  Serial.begin(115200);
  inputBuffer.reserve(128);

  IrReceiver.begin(IR_RECV_PIN, ENABLE_LED_FEEDBACK);

  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  pinMode(RelayPin3, OUTPUT);
  pinMode(RelayPin4, OUTPUT);
  pinMode(TriacPin, OUTPUT);

  pinMode(ZeroCrossPin, INPUT_PULLUP);
  pinMode(SwitchPin1, INPUT_PULLUP);
  pinMode(SwitchPin2, INPUT_PULLUP);
  pinMode(SwitchPin3, INPUT_PULLUP);
  pinMode(SwitchPin4, INPUT_PULLUP);
  pinMode(SwitchPin5, INPUT_PULLUP);
  pinMode(SwitchPin6, INPUT_PULLUP);

  digitalWrite(RelayPin1, HIGH);
  digitalWrite(RelayPin2, HIGH);
  digitalWrite(RelayPin3, HIGH);
  digitalWrite(RelayPin4, HIGH);
  digitalWrite(TriacPin, LOW);

  atmega328_16mhz_ac_phase_control.init();

  config1.setEventHandler(button1Handler);
  config2.setEventHandler(button2Handler);
  config3.setEventHandler(button3Handler);
  config4.setEventHandler(button4Handler);
  config5.setEventHandler(button5Handler);
  config6.setEventHandler(button6Handler);

  config5.setFeature(ButtonConfig::kFeatureLongPress);
  config5.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);

  config6.setFeature(ButtonConfig::kFeatureLongPress);
  config6.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);

  button1.init(SwitchPin1);
  button2.init(SwitchPin2);
  button3.init(SwitchPin3);
  button4.init(SwitchPin4);
  button5.init(SwitchPin5);
  button6.init(SwitchPin6);

  delay(500);
  eepromState();

  timer.every(2000, sendStatus);
}

void loop()
{
  ir_remote();
  handleSerialControl();

  button1.check();
  button2.check();
  button3.check();
  button4.check();
  button5.check();
  button6.check();

  timer.tick();
}

void button1Handler(AceButton* button, uint8_t eventType, uint8_t buttonState)
{
  if (eventType == AceButton::kEventReleased) relayToggle(1);
}

void button2Handler(AceButton* button, uint8_t eventType, uint8_t buttonState)
{
  if (eventType == AceButton::kEventReleased) relayToggle(2);
}

void button3Handler(AceButton* button, uint8_t eventType, uint8_t buttonState)
{
  if (eventType == AceButton::kEventReleased) relayToggle(3);
}

void button4Handler(AceButton* button, uint8_t eventType, uint8_t buttonState)
{
  if (eventType == AceButton::kEventReleased) relayToggle(4);
}

void button5Handler(AceButton* button, uint8_t eventType, uint8_t buttonState)
{
  switch (eventType)
  {
    case AceButton::kEventLongPressed:
      triacOff();
      break;
    case AceButton::kEventReleased:
      dimm_Dn();
      break;
  }
}

void button6Handler(AceButton* button, uint8_t eventType, uint8_t buttonState)
{
  switch (eventType)
  {
    case AceButton::kEventLongPressed:
      triacOn();
      break;
    case AceButton::kEventReleased:
      dimm_Up();
      break;
  }
}
