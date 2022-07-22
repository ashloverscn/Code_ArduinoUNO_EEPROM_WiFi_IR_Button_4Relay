#include <EEPROM.h>
#include <IRremote.h>
#include <AceButton.h>
#include <arduino-timer.h>
#include <atmega328_16mhz_ac_phase_control.h>

int spd[14]={600,500,480,450,400,380,350,300,250,180,150,80,50,1};//our good stage's values
//valus from 623 to 1 NOTE: MIN_AC_POWER=623 and MAX_AC_POWER=1

auto timer = timer_create_default(); // create a timer with default settings
using namespace ace_button;

// define the GPIO connected with Relays and switches
#define RelayPin1 5  //D4
#define RelayPin2 6  //D5
#define RelayPin3 7  //D6
#define RelayPin4 8  //D7
#define TriacPin 9  //D9

#define SwitchPin1 A0  //A0
#define SwitchPin2 A1  //A1
#define SwitchPin3 A2  //A2
#define SwitchPin4 A3  //A3
#define SwitchPin5 A4  //A4
#define SwitchPin6 A5  //A5

#define ZeroCrossPin 2 //D2
#define IR_RECV_PIN 4  //D3

//Update the HEX code of IR Remote buttons 0x<HEX CODE>
#define IR_Button_1   0x1FE50AF
#define IR_Button_2   0x1FED827
#define IR_Button_3   0x1FEF807
#define IR_Button_4   0x1FE30CF
#define IR_Button_5   0x1FEB04F
#define IR_Button_Up   0x1FE609F
#define IR_Button_Dn   0x1FEA05F
#define IR_All_Off    0x1FE7887
#define IR_All_On     0x1FE48B7

IRrecv irrecv(IR_RECV_PIN);
decode_results results;

String pinStatus = "0000";
int dimm_value = 0;

ButtonConfig config1;
AceButton button1(&config1);
ButtonConfig config2;
AceButton button2(&config2);
ButtonConfig config3;
AceButton button3(&config3);
ButtonConfig config4;
AceButton button4(&config4);
ButtonConfig config5;
AceButton button5(&config5);
ButtonConfig config6;
AceButton button6(&config6);

void handleEvent1(AceButton*, uint8_t, uint8_t);
void handleEvent2(AceButton*, uint8_t, uint8_t);
void handleEvent3(AceButton*, uint8_t, uint8_t);
void handleEvent4(AceButton*, uint8_t, uint8_t);
void handleEvent5(AceButton*, uint8_t, uint8_t);
void handleEvent6(AceButton*, uint8_t, uint8_t);

void dimm_Up(){
  if(dimm_value<13)
            {
             dimm_value += 1;
             atmega328_16mhz_ac_phase_control.set_ac_power(spd[dimm_value]);//spd[-1 to 13] or 623 to 1 
             EEPROM.update(7, dimm_value);
            }
}

void dimm_Dn(){
  if(dimm_value>=0)
            {
              dimm_value -= 1 ;
              atmega328_16mhz_ac_phase_control.set_ac_power(spd[dimm_value]);//spd[-1 to 13] or 623 to 1 
              EEPROM.update(7, dimm_value);        
            }
}

void triacOn(){
    dimm_value = 13;
    atmega328_16mhz_ac_phase_control.set_ac_power(spd[dimm_value]);//spd[-1 to 13] or 623 to 1
    EEPROM.update(7, dimm_value);
}

void triacOff(){
    dimm_value = -1;
    atmega328_16mhz_ac_phase_control.set_ac_power(spd[dimm_value]);//spd[-1 to 13] or 623 to 1
    EEPROM.update(7, dimm_value);
}

void triacOnOff(){
    if(dimm_value == -1)
             {
              dimm_value = 13;
              atmega328_16mhz_ac_phase_control.set_ac_power(spd[dimm_value]);//spd[-1 to 13] or 623 to 1
              EEPROM.update(7, dimm_value);
             }
             else
             {
              dimm_value = -1;
              atmega328_16mhz_ac_phase_control.set_ac_power(spd[dimm_value]);//spd[-1 to 13] or 623 to 1
              EEPROM.update(7, dimm_value);
             }
}

void relayOnOff(int relay){
 switch(relay){
      case 1:
            digitalWrite(RelayPin1, !digitalRead(RelayPin1)); // change state for relay-1
            EEPROM.update(0,digitalRead(RelayPin1));
            delay(100);
      break;
      case 2:
            digitalWrite(RelayPin2, !digitalRead(RelayPin2)); // change state for relay-2
            EEPROM.update(1,digitalRead(RelayPin2));
            delay(100);
      break;
      case 3:
            digitalWrite(RelayPin3, !digitalRead(RelayPin3)); // change state for relay-3
            EEPROM.update(2,digitalRead(RelayPin3));
            delay(100);
      break;
      case 4:
            digitalWrite(RelayPin4, !digitalRead(RelayPin4)); // change state for relay-4
            EEPROM.update(3,digitalRead(RelayPin4));
            delay(100);
      break;
      default : break;      
      }  
}

void eepromState()
{
  digitalWrite(RelayPin1, EEPROM.read(0)); delay(200);
  digitalWrite(RelayPin2, EEPROM.read(1)); delay(200);
  digitalWrite(RelayPin3, EEPROM.read(2)); delay(200);
  digitalWrite(RelayPin4, EEPROM.read(3)); delay(200);
  dimm_value = EEPROM.read(7); delay(200);
}

void ir_remote(){
  if (irrecv.decode(&results)) {
    
      switch(results.value){
          case IR_Button_1:  relayOnOff(1);     break;
          case IR_Button_2:  relayOnOff(2);     break;
          case IR_Button_3:  relayOnOff(3);     break;
          case IR_Button_4:  relayOnOff(4);     break;
          case IR_Button_5:  triacOnOff();      break;
          case IR_Button_Up:  dimm_Up();        break;
          case IR_Button_Dn:  dimm_Dn();        break;
          case IR_All_Off:   all_Switch_OFF();  break;
          case IR_All_On:    all_Switch_ON();   break;
          default : break;         
        }   
        Serial.println(results.value, HEX);    
        irrecv.resume();   
  } 
}

void all_Switch_ON(){
  digitalWrite(RelayPin1, LOW); EEPROM.update(0,LOW); delay(100);
  digitalWrite(RelayPin2, LOW); EEPROM.update(1,LOW); delay(100);
  digitalWrite(RelayPin3, LOW); EEPROM.update(2,LOW); delay(100);
  digitalWrite(RelayPin4, LOW); EEPROM.update(3,LOW); delay(100);
  triacOn();  delay(100);
}

void all_Switch_OFF(){
  digitalWrite(RelayPin1, HIGH); EEPROM.update(0,HIGH); delay(100);
  digitalWrite(RelayPin2, HIGH); EEPROM.update(1,HIGH); delay(100);
  digitalWrite(RelayPin3, HIGH); EEPROM.update(2,HIGH); delay(100);
  digitalWrite(RelayPin4, HIGH); EEPROM.update(3,HIGH); delay(100);
  triacOff(); delay(100);
}

void sendStatus(){  
  pinStatus = String(!digitalRead(RelayPin1)) + String(!digitalRead(RelayPin2)) + String(!digitalRead(RelayPin3)) + String(!digitalRead(RelayPin4));
  Serial.println(pinStatus);
  Serial.println(dimm_value);
}

void setup() {
  Serial.begin(9600);
  
  irrecv.enableIRIn(); // Start the receiver

  atmega328_16mhz_ac_phase_control.init(); //Initialize AC Phase dimmer
  
  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  pinMode(RelayPin3, OUTPUT);
  pinMode(RelayPin4, OUTPUT);
  pinMode(TriacPin,OUTPUT);

  pinMode(ZeroCrossPin, INPUT_PULLUP);
  pinMode(SwitchPin1, INPUT_PULLUP);
  pinMode(SwitchPin2, INPUT_PULLUP);
  pinMode(SwitchPin3, INPUT_PULLUP);
  pinMode(SwitchPin4, INPUT_PULLUP);
  pinMode(SwitchPin5, INPUT_PULLUP);
  pinMode(SwitchPin6, INPUT_PULLUP);

  //During Starting all Relays should TURN OFF
  digitalWrite(RelayPin1, HIGH);
  digitalWrite(RelayPin2, HIGH);
  digitalWrite(RelayPin3, HIGH);
  digitalWrite(RelayPin4, HIGH);

  config1.setEventHandler(button1Handler);
  config2.setEventHandler(button2Handler);
  config3.setEventHandler(button3Handler);
  config4.setEventHandler(button4Handler);
  config5.setEventHandler(button5Handler);
  config6.setEventHandler(button6Handler);
  
  button1.init(SwitchPin1);
  button2.init(SwitchPin2);
  button3.init(SwitchPin3);
  button4.init(SwitchPin4);
  button5.init(SwitchPin5);
  button6.init(SwitchPin6);
  
  delay(500);
  eepromState();
  // call the toggle_led function every 2000 millis (2 seconds)
  timer.every(2000, sendStatus);  
}

void loop() {

  ir_remote();

  atmega328_16mhz_ac_phase_control.set_ac_power(spd[dimm_value]);//spd[-1 to 13] or 623 to 1 
  
  button1.check();
  button2.check();
  button3.check();
  button4.check();
  button5.check();
  button6.check();

  timer.tick(); // tick the timer
}

void button1Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventReleased:
      relayOnOff(1);
      break;
  }
}
void button2Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventReleased:
      relayOnOff(2);
      break;
  }
}
void button3Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventReleased:
      relayOnOff(3);
      break;
  }
}
void button4Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventReleased:
      relayOnOff(4);
      break;
  }
}
void button5Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventReleased:
      dimm_Dn();
      break;
  }
}
void button6Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  switch (eventType) {
    case AceButton::kEventReleased:
      dimm_Up();
      break;
  }
}
