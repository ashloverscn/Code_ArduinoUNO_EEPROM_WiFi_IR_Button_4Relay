#define S 11
#define D 10
#define R 12

int datArray[18] = {
  0b10000001, //0
  0b11110011, //1
  0b01001001, //2
  0b01100001, //3
  0b00110011, //4
  0b00100101, //5
  0b00000101, //6
  0b11110001, //7
  0b00000001, //8
  0b00100001, //9
  0b00010001, //A
  0b00000111, //b
  0b10001101, //C
  0b01000011, //d
  0b00001101, //E
  0b00011101, //F
  0b11111110, //. is -1
  0b11111111, //all off
  };

void setup() 
{
  pinMode(S,OUTPUT);
  pinMode(D,OUTPUT);
  pinMode(R,OUTPUT);

  for(int i=0; i<18; i++)
  {
    shiftOut(D, S, R, datArray[i]);
    delay(1000);
  } 
}

void loop()
{
   
}
