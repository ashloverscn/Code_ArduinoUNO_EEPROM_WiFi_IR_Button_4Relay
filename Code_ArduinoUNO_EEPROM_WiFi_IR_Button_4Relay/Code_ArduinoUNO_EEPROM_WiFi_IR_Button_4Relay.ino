#include <EEPROM.h>
#include <IRremote.hpp>
#include <AceButton.h>
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

void relayOnOff(int relay)
{
  switch (relay)
  {
    case 1:
      digitalWrite(RelayPin1, !digitalRead(RelayPin1));
      EEPROM.update(EEPROM_RELAY1, digitalRead(RelayPin1));
      delay(100);
      break;

    case 2:
      digitalWrite(RelayPin2, !digitalRead(RelayPin2));
      EEPROM.update(EEPROM_RELAY2, digitalRead(RelayPin2));
      delay(100);
      break;

    case 3:
      digitalWrite(RelayPin3, !digitalRead(RelayPin3));
      EEPROM.update(EEPROM_RELAY3, digitalRead(RelayPin3));
      delay(100);
      break;

    case 4:
      digitalWrite(RelayPin4, !digitalRead(RelayPin4));
      EEPROM.update(EEPROM_RELAY4, digitalRead(RelayPin4));
      delay(100);
      break;

    default:
      break;
  }
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

  digitalWrite(RelayPin1, relay1);
  delay(50);
  digitalWrite(RelayPin2, relay2);
  delay(50);
  digitalWrite(RelayPin3, relay3);
  delay(50);
  digitalWrite(RelayPin4, relay4);
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
          relayOnOff(1);
          break;
        case IR_Button_2:
          relayOnOff(2);
          break;
        case IR_Button_3:
          relayOnOff(3);
          break;
        case IR_Button_4:
          relayOnOff(4);
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
  digitalWrite(RelayPin1, HIGH);
  EEPROM.update(EEPROM_RELAY1, HIGH);
  delay(100);
  digitalWrite(RelayPin2, HIGH);
  EEPROM.update(EEPROM_RELAY2, HIGH);
  delay(100);
  digitalWrite(RelayPin3, HIGH);
  EEPROM.update(EEPROM_RELAY3, HIGH);
  delay(100);
  digitalWrite(RelayPin4, HIGH);
  EEPROM.update(EEPROM_RELAY4, HIGH);
  delay(100);
}

void all_Switch_OFF()
{
  digitalWrite(RelayPin1, LOW);
  EEPROM.update(EEPROM_RELAY1, LOW);
  delay(100);
  digitalWrite(RelayPin2, LOW);
  EEPROM.update(EEPROM_RELAY2, LOW);
  delay(100);
  digitalWrite(RelayPin3, LOW);
  EEPROM.update(EEPROM_RELAY3, LOW);
  delay(100);
  digitalWrite(RelayPin4, LOW);
  EEPROM.update(EEPROM_RELAY4, LOW);
  delay(100);
}

void sendStatus()
{
  pinStatus = String(digitalRead(RelayPin1)) +
              String(digitalRead(RelayPin2)) +
              String(digitalRead(RelayPin3)) +
              String(digitalRead(RelayPin4));
}

void setup()
{
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

  digitalWrite(RelayPin1, LOW);
  digitalWrite(RelayPin2, LOW);
  digitalWrite(RelayPin3, LOW);
  digitalWrite(RelayPin4, LOW);
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
  if (eventType == AceButton::kEventReleased) relayOnOff(1);
}

void button2Handler(AceButton* button, uint8_t eventType, uint8_t buttonState)
{
  if (eventType == AceButton::kEventReleased) relayOnOff(2);
}

void button3Handler(AceButton* button, uint8_t eventType, uint8_t buttonState)
{
  if (eventType == AceButton::kEventReleased) relayOnOff(3);
}

void button4Handler(AceButton* button, uint8_t eventType, uint8_t buttonState)
{
  if (eventType == AceButton::kEventReleased) relayOnOff(4);
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
