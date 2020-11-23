#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include <Wire.h>
#include <Adafruit_MCP4725.h>

#include <Servo.h>
#include <TEA5767.h>

#define fullFrameSize 32


#define r_slope 400
// Config later
#define defaultFreq 1700
#define f0 500
#define f1 800
#define f2 1100
#define f3 1400

#define garbageSize 10

#define N 4

#define IFrame 0
#define UFrame 2
#define SFrame 3

double S[N];
uint16_t S_DAC[N];
Adafruit_MCP4725 dac;
TEA5767 radio = TEA5767();

bool canSend = true;
bool intialize = true;
bool startTimer = false;
bool receivedAck = false;
char type = 'O';
String cmd = "";
int mode = 0;
unsigned int frameNo = 0, ackNo = 0;
uint32_t data = 0x0;

int delay0, delay1, delay2, delay3;
unsigned long long currentTime = 0, startTime = 0, timeOut = 10000;
unsigned long long sendTime = 0, sendTimeOut = 10000;
uint32_t outFrame = 0;
uint16_t inFrame = 0;
unsigned int baud_count = 0;
int inFrameType = 0;



int prev = 0;
int count = 0;

uint16_t baud_check = 0;

uint16_t bit_check = -1;

bool check_amp = false;
bool check_baud = false;

const float frequency = 89.6; //Enter your own Frequency // old wave = 89.7

uint32_t baud_begin = 0;

String pictureValueStore[3] = {"X", "X", "X"};


//Servo setup

int x = 0;
Servo myservo; 
int num=40;

//End
void setup() {
  Serial.begin(9600);
  dac.begin(0x60);
  Wire.begin();
  for (int i = 0; i < N; i++) {
    S[i] = sin(2 * PI * (i / (double)N));
    S_DAC[i] = 4095 / 2.0 * (1 + S[i]);
//    Serial.println(S_DAC[i]);
  }


  delay0 = (1000000 / f0 - 1000000 / defaultFreq) / N;
  delay1 = (1000000 / f1 - 1000000 / defaultFreq) / N;
  delay2 = (1000000 / f2 - 1000000 / defaultFreq) / N;
  delay3 = (1000000 / f3 - 1000000 / defaultFreq) / N;

  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  pinMode(A0, INPUT);

  radio.set_frequency(frequency);
  // Servo init
  myservo.attach(9); // กำหนดขา 9 ควบคุม Servo
  if (x < num){
      while(x<=num){
        myservo.write(x);
        x++;
        if (x == num){
          myservo.write(x);
        }
        delay(10);
      }
    }
    else if (x > num){
      while(x >= num){
        myservo.write(x);
        x--;
        if (x == num){
          myservo.write(x);
        }
        delay(10);
       }
    }
   // Servo end
    
  Serial.flush();
//    delay(2000);
}
// 11101100110111001111111100111011
// 11111111111111111111111100111011

void checkFrame()
{

  if (checkError(inFrame) == false) // No error
  {

    int inFrameType = inFrame >> 10;

    Serial.println("Received connection ...");
//    delay(500);
    if (inFrameType == UFrame)
    {
      
//      type = 'S';
//      data = 0x1ffffff;
      
//      data = 0x1555555;

      sendFrame(false);

      int cmd = (inFrame >> 5) & B11111;
      if (cmd == B00110)
      {
        
        Serial.println("Waiting for PC1 command ...");
      }
      else if (cmd == B00111)
      {
        Serial.println("---- End ----");
      }
    }
    else if (inFrameType == IFrame)
    {
      sendFrame(false); // Send ack back
      int cmd = (inFrame >> 5) & B1111;
      if (cmd == B1111)
      {
        captureAll();
      }
    }
  }
  else
  {
    Serial.println("Error detected in data");
  }
}

void captureAll()
{
  int i = 0;
  int cameraBit = 0;
  Serial.flush();
//  Serial.println(" - - Capture mode - - ");
  Serial.print("g");
  delay(500);
  sendFrame(false);
//  delay(500);
//  if (Serial.available() > 0)
//  {
//    String cameraBit = Serial.readString();
//    String tmpS = "";
//    int j = 0;
//    for (int i = 0; i < cameraBit.length(); i++)
//    {
//      if (cameraBit[i] == ",")
//      {
//        pictureValueStore[j] = tmpS;
//        tmpS = "";
//        j++;
//      }
//      else
//      {
//        tmpS += cameraBit[i];
//      }
//    }
//    type = 'I';
//    for (int i = 0; i < sizeof(pictureValueStore); i++)
//    {
//      String temp = pictureValueStore[i].substring(0, 4);
//      String rotateTemp = pictureValueStore[i].substring(4);
//      byte value = B0;
//      if (rotateTemp == "C")
//      {
//        temp += "01";
//      }
//      else if (rotateTemp == "L")
//      {
//        temp += "00";
//      }
//
//      else if (rotateTemp == "R")
//      {
//        temp += "10";
//      }
//      data = temp.toInt();
//
//    }
//    //        sendFrame(true);
//    delay(1000);
//
//  }


}
void CRC() //เอา Data มาต่อ CRC
{
  if (outFrame != 0)
  {
    unsigned long long canXOR = 0x800000000;//ตั้งใหญ่ๆไว้ก่อนเพื่อเอามาเช็คขนาดดาต้า
    unsigned long long remainder = outFrame << 5;//ตัวแปรใหม่ที่มาจากการเติม 0 หลัง outFrame 5 ตัว
    unsigned long divisor = B110101;//กำหนด divisor
    unsigned long long tmp = canXOR & remainder;//ไว้ตรวจว่าจะเอา remainder ไป or ตรงไหนใน data(ต้อง XOR ตัวหน้าสุดก่อน)
    while (tmp == 0)
    {
      canXOR >>= 1;//ปรับขนาดให้เท่ากับดาต้า
      tmp = canXOR & remainder;
    }
    tmp = canXOR & divisor;//ไว้ตรวจว่าจะเริ่ม XOR ตำแหน่งไหน
    while (tmp == 0)
    {
      divisor <<= 1;//ชิพไปเรื่อยๆจนกว่าจะถึงตำแหน่ง XOR แรก
      tmp = canXOR & divisor;
    }
    while (divisor >= B110101)//ทำจนกว่า divisor จะน้อยกว่าค่าที่กำหนด
    {
      tmp = remainder & canXOR;//ดูว่าตำแหน่งนี้ XOR ได้หรือไม่(XOR ตำแหน่งที่เป็น 1xxxxx)
      if (tmp > 0) remainder = remainder ^ divisor;//ทำการ XOR
      divisor >>= 1;
      canXOR >>= 1;
    }
    outFrame <<= 5;//เติม0 5ตัว
    outFrame += remainder;//เปลี่ยน5บิตสุดท้ายเป็นremainder
  }

}
void makeFrame()
{
  outFrame = 0;
  /*
    Frame
    size : 32
    type: U-frame, S-Frame, I-frame
    U-Frame = [   00    |  00000...(25ตัว) | 00000]
                Type         cmd             CRC
    S-Frame = [   01    |     0000...(24ตัว)     |   0  | 00000 ]
               Type             cmd               ackNo   CRC
    I-Frame = [   11   |    0     | 110011...(24ตัว) | 00000 ]
               Type    FrameNo        Data             CRC
  */

  switch (type)
  {
    case 'I':
      outFrame = IFrame;
      outFrame <<= 1;
      outFrame |= frameNo;
      outFrame <<= 24;
      outFrame |= data;
      break;
    case 'S':
      outFrame = SFrame;
      outFrame <<= 24;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= ackNo;
      break;

    case 'U':
      outFrame = UFrame;
      outFrame <<= 25;
      outFrame |= data;
      break;

    default:
      Serial.println("Please defined type of the frame.");
      break;

  }

  CRC();

}
bool checkError(unsigned int frame)
{
  if (frame != 0)
  {
    unsigned long long canXOR = 0x80000000;//ตั้งใหญ่ๆไว้ก่อนเพื่อเอามาเช็คขนาดดาต้า
    unsigned long long remainder = frame;
    unsigned long divisor = B110101;//กำหนด divisor
    unsigned long long tmp = canXOR & remainder;//ไว้ตรวจว่าจะเอา remainder ไป or ตรงไหนใน data(ต้อง XOR ตัวหน้าสุดก่อน)
    while (tmp == 0)
    {
      canXOR >>= 1;//ปรับขนาดให้เท่ากับดาต้า
      tmp = canXOR & remainder;
    }

    tmp = canXOR & divisor;//ไว้ตรวจว่าจะเริ่ม XOR ตำแหน่งไหน
    while (tmp == 0)
    {
      divisor <<= 1;//ชิพไปเรื่อยๆจนกว่าจะถึงตำแหน่ง XOR แรก
      tmp = canXOR & divisor;
    }
    while (divisor >= B110101)//ทำจนกว่า divisor จะน้อยกว่าค่าที่กำหนด
    {
      tmp = remainder & canXOR;//ดูว่าตำแหน่งนี้ XOR ได้หรือไม่(XOR ตำแหน่งที่เป็น 1xxxxx)
      if (tmp > 0)remainder = remainder ^ divisor;//ทำการ XOR
      divisor >>= 1;
      canXOR >>= 1;
    }

    if (remainder == 0) return false;
    else return true;
  }
}
void sendFSK(int freq, int in_delay) //Config later
{
  int cyc;
  switch (freq)
  {
    case 500:
      cyc = 5;
      break;
    //        case 800:
    //            cyc = 8;
    //            break;
    //        case 1100:
    //            cyc = 11;
    //            break;
    case 1400:
      cyc = 14;
      break;
  }

  for (int sl = 0; sl < cyc; sl++)
  {
    for (int s = 0; s < N; s++) //4 sample/cycle
    {
      dac.setVoltage(S_DAC[s], false);//modify amplitude
      delayMicroseconds(in_delay);
    }
  }
}

void sendFrame(bool isFrame)
{
  if (isFrame)
  {
    if (currentTime != 0) startTime = currentTime;
    else startTime = 0;
    startTimer = true;
  }
  else
  {
    type = 'S';
    data = 0x1ffffff;
  }
  makeFrame();
  
  Serial.print("Sending Frame: ");
  Serial.println(outFrame, BIN);
  // Frame มีทั้งหมด 32 (รวม Parity แล้ว)
//  dac.setVoltage(900, false);
//  delay(20);
  for (int i = 0; i < fullFrameSize; i += 1)
  {
    
    int out = outFrame & 1;
    if (out == 0) sendFSK(f0, delay0);
    //        else if (out == B01) sendFSK(f1, delay1);
    //        else if (out == B10) sendFSK(f2, delay2);
    else if (out == 1) sendFSK(f3, delay3);
    outFrame >>= 1;

  }
//  delay(1000);
  dac.setVoltage(0, false);
}
void receiveFrame() {
  int tmp = analogRead(A0);
//    Serial.println(tmp);
  if ( tmp > r_slope and prev < r_slope and !check_amp ) // check amplitude
  {
    //Serial.println(tmp);
    check_amp = true; // is first max amplitude in that baud
    if ( !check_baud )
    {
      baud_begin = micros();
      bit_check++;
    }
  }

  if (tmp < r_slope and check_baud) {
    if (micros() - baud_begin  > 9900) // full baud
    {
      //Serial.println(count);
      //      if (count > 1 && count < 7) {
      //        count = 5 ;
      //      }
      //      else if (count >= 7 && count < 10) {
      //        count = 8;
      //      }
      //      else if (count >= 10 && count < 13) {
      //        count = 11;
      //      }
      //      else if (count >= 13) {
      //        count = 14;
      //      }
      if (count >= 4 && count < 9)
      {
        count = 5;
      }
      else if (count >= 9)
      {
        count = 14;
      }
      //Serial.println(count);
      
      uint16_t last = (((count - 5) / 9) & 1) << (bit_check);

      inFrame |= last;

      baud_check++;
    
      //Serial.println("BBBB-------");
      
      if (baud_check == 12) // 12 bits
      {
        Serial.print("Receive Frame : ");
        Serial.println(inFrame, BIN); // add two new bits in data
        checkFrame();
//        Serial.flush();
//        radio.setFrequency(frequency);
        inFrame = 0;
        baud_check = 0;
        bit_check = -1;
//        delay(500);
      }
      check_baud = false;
      count = 0;
    }
  }

  if (tmp > r_slope and check_amp) {
    count++;
    //Serial.println(tmp);
    check_baud = true;
    check_amp = false;
  }
  prev = tmp;
  //Serial.println(micros() - baud_begin);
}

//-------------------------------------Servo-------------------------------------//

void servoControl()
{
  if (Serial.available() > 0) {
    int input = Serial.read();
    if (input == 'A'){
      num = 40;
    }
    else if(input == 'B'){
      num = 90;
    }
    else if(input == 'C'){
      num = 144;
    }
    if (x < num){
      while(x<=num){
        myservo.write(x);
        x++;
        if (x == num){
          myservo.write(x);
//          Serial.println(x/10);
        }
        delay(10);
      }
    }
    else if (x > num){
      while(x >= num){
        myservo.write(x);
        x--;
        if (x == num){
          myservo.write(x);
//          Serial.println(x/10);
        }
        delay(10);
       }
    }
    delay(10);
  }
}
//-------------------------------------Timer-------------------------------------//

void timer()
{
  if (startTimer)
    {
        unsigned long long currentTime = millis();
        if (currentTime - startTime > timeOut)
        {
          startTime = currentTime;
          Serial.println("Retransmitting the frame ...");
          sendFrame(true);
        }
    } 
}

//-----------------------------------Timer end-------------------------------------//

void loop()
{

  receiveFrame();
  servoControl();
  timer();
  if (Serial.available() > 0)
  {
    String cameraBit = Serial.readString();
    Serial.print("Data from camera : ");
    Serial.println(cameraBit);
//    if (cameraBit == "A" or cameraBit == "B" or cameraBit == "C")
//    {
//      sendFrame(false);
//    }
    String tmpS = "";
    int j = 0;
    for (int i = 0; i < cameraBit.length(); i++)
    {
      if (cameraBit[i] == ',')
      {
        pictureValueStore[j] = tmpS;
        tmpS = "";
        j++;
      }
      else
      {
        tmpS += cameraBit[i];
      }
    }
    type = 'I';
    for (int i = 0; i < 3; i++)
    {
      String temp = pictureValueStore[i].substring(0, 4);
      String rotateTemp = pictureValueStore[i].substring(4);
      byte value = B0;
      if (rotateTemp == "C")
      {
        temp += "01";
      }
      else if (rotateTemp == "L")
      {
        temp += "00";
      }

      else if (rotateTemp == "R")
      {
        temp += "10";
      }
      
      
      long tmp_data = temp.toInt();
      Serial.println(tmp_data);
//
//    }
//    //        sendFrame(true);
//    delay(1000);

  }
  }


  
//  if (startTimer)
//  {
//    currentTime = millis();
//    if (currentTime - startTime > timeOut)
//    {
//      Serial.println("Retransmitting the frame ...");
//      sendFrame(true);
//    }
//  }
//  else currentTime = 0;

}#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include <Wire.h>
#include <Adafruit_MCP4725.h>

#include <Servo.h>
#include <TEA5767.h>

#define fullFrameSize 32


#define r_slope 400
// Config later
#define defaultFreq 1700
#define f0 500
#define f1 800
#define f2 1100
#define f3 1400

#define garbageSize 10

#define N 4

#define IFrame 0
#define UFrame 2
#define SFrame 3

double S[N];
uint16_t S_DAC[N];
Adafruit_MCP4725 dac;
TEA5767 radio = TEA5767();

bool canSend = true;
bool intialize = true;
bool startTimer = false;
bool receivedAck = false;
char type = 'O';
String cmd = "";
int mode = 0;
unsigned int frameNo = 0, ackNo = 0;
uint32_t data = 0x0;

int delay0, delay1, delay2, delay3;
unsigned long long currentTime = 0, startTime = 0, timeOut = 10000;
unsigned long long sendTime = 0, sendTimeOut = 10000;
uint32_t outFrame = 0;
uint16_t inFrame = 0;
unsigned int baud_count = 0;
int inFrameType = 0;



int prev = 0;
int count = 0;

uint16_t baud_check = 0;

uint16_t bit_check = -1;

bool check_amp = false;
bool check_baud = false;

const float frequency = 89.6; //Enter your own Frequency // old wave = 89.7

uint32_t baud_begin = 0;

String pictureValueStore[3] = {"X", "X", "X"};


//Servo setup

int x = 0;
Servo myservo; 
int num=40;

//End
void setup() {
  Serial.begin(9600);
  dac.begin(0x60);
  Wire.begin();
  for (int i = 0; i < N; i++) {
    S[i] = sin(2 * PI * (i / (double)N));
    S_DAC[i] = 4095 / 2.0 * (1 + S[i]);
//    Serial.println(S_DAC[i]);
  }


  delay0 = (1000000 / f0 - 1000000 / defaultFreq) / N;
  delay1 = (1000000 / f1 - 1000000 / defaultFreq) / N;
  delay2 = (1000000 / f2 - 1000000 / defaultFreq) / N;
  delay3 = (1000000 / f3 - 1000000 / defaultFreq) / N;

  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  pinMode(A0, INPUT);

  radio.set_frequency(frequency);
  // Servo init
  myservo.attach(9); // กำหนดขา 9 ควบคุม Servo
  if (x < num){
      while(x<=num){
        myservo.write(x);
        x++;
        if (x == num){
          myservo.write(x);
        }
        delay(10);
      }
    }
    else if (x > num){
      while(x >= num){
        myservo.write(x);
        x--;
        if (x == num){
          myservo.write(x);
        }
        delay(10);
       }
    }
   // Servo end
    
  Serial.flush();
//    delay(2000);
}
// 11101100110111001111111100111011
// 11111111111111111111111100111011

void checkFrame()
{

  if (checkError(inFrame) == false) // No error
  {

    int inFrameType = inFrame >> 10;

    Serial.println("Received connection ...");
//    delay(500);
    if (inFrameType == UFrame)
    {
      
//      type = 'S';
//      data = 0x1ffffff;
      
//      data = 0x1555555;

      sendFrame(false);

      int cmd = (inFrame >> 5) & B11111;
      if (cmd == B00110)
      {
        
        Serial.println("Waiting for PC1 command ...");
      }
      else if (cmd == B00111)
      {
        Serial.println("---- End ----");
      }
    }
    else if (inFrameType == IFrame)
    {
      sendFrame(false); // Send ack back
      int cmd = (inFrame >> 5) & B1111;
      if (cmd == B1111)
      {
        captureAll();
      }
    }
  }
  else
  {
    Serial.println("Error detected in data");
  }
}

void captureAll()
{
  int i = 0;
  int cameraBit = 0;
  Serial.flush();
//  Serial.println(" - - Capture mode - - ");
  Serial.print("g");
  delay(500);
  sendFrame(false);
//  delay(500);
//  if (Serial.available() > 0)
//  {
//    String cameraBit = Serial.readString();
//    String tmpS = "";
//    int j = 0;
//    for (int i = 0; i < cameraBit.length(); i++)
//    {
//      if (cameraBit[i] == ",")
//      {
//        pictureValueStore[j] = tmpS;
//        tmpS = "";
//        j++;
//      }
//      else
//      {
//        tmpS += cameraBit[i];
//      }
//    }
//    type = 'I';
//    for (int i = 0; i < sizeof(pictureValueStore); i++)
//    {
//      String temp = pictureValueStore[i].substring(0, 4);
//      String rotateTemp = pictureValueStore[i].substring(4);
//      byte value = B0;
//      if (rotateTemp == "C")
//      {
//        temp += "01";
//      }
//      else if (rotateTemp == "L")
//      {
//        temp += "00";
//      }
//
//      else if (rotateTemp == "R")
//      {
//        temp += "10";
//      }
//      data = temp.toInt();
//
//    }
//    //        sendFrame(true);
//    delay(1000);
//
//  }


}
void CRC() //เอา Data มาต่อ CRC
{
  if (outFrame != 0)
  {
    unsigned long long canXOR = 0x800000000;//ตั้งใหญ่ๆไว้ก่อนเพื่อเอามาเช็คขนาดดาต้า
    unsigned long long remainder = outFrame << 5;//ตัวแปรใหม่ที่มาจากการเติม 0 หลัง outFrame 5 ตัว
    unsigned long divisor = B110101;//กำหนด divisor
    unsigned long long tmp = canXOR & remainder;//ไว้ตรวจว่าจะเอา remainder ไป or ตรงไหนใน data(ต้อง XOR ตัวหน้าสุดก่อน)
    while (tmp == 0)
    {
      canXOR >>= 1;//ปรับขนาดให้เท่ากับดาต้า
      tmp = canXOR & remainder;
    }
    tmp = canXOR & divisor;//ไว้ตรวจว่าจะเริ่ม XOR ตำแหน่งไหน
    while (tmp == 0)
    {
      divisor <<= 1;//ชิพไปเรื่อยๆจนกว่าจะถึงตำแหน่ง XOR แรก
      tmp = canXOR & divisor;
    }
    while (divisor >= B110101)//ทำจนกว่า divisor จะน้อยกว่าค่าที่กำหนด
    {
      tmp = remainder & canXOR;//ดูว่าตำแหน่งนี้ XOR ได้หรือไม่(XOR ตำแหน่งที่เป็น 1xxxxx)
      if (tmp > 0) remainder = remainder ^ divisor;//ทำการ XOR
      divisor >>= 1;
      canXOR >>= 1;
    }
    outFrame <<= 5;//เติม0 5ตัว
    outFrame += remainder;//เปลี่ยน5บิตสุดท้ายเป็นremainder
  }

}
void makeFrame()
{
  outFrame = 0;
  /*
    Frame
    size : 32
    type: U-frame, S-Frame, I-frame
    U-Frame = [   00    |  00000...(25ตัว) | 00000]
                Type         cmd             CRC
    S-Frame = [   01    |     0000...(24ตัว)     |   0  | 00000 ]
               Type             cmd               ackNo   CRC
    I-Frame = [   11   |    0     | 110011...(24ตัว) | 00000 ]
               Type    FrameNo        Data             CRC
  */

  switch (type)
  {
    case 'I':
      outFrame = IFrame;
      outFrame <<= 1;
      outFrame |= frameNo;
      outFrame <<= 24;
      outFrame |= data;
      break;
    case 'S':
      outFrame = SFrame;
      outFrame <<= 24;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= ackNo;
      break;

    case 'U':
      outFrame = UFrame;
      outFrame <<= 25;
      outFrame |= data;
      break;

    default:
      Serial.println("Please defined type of the frame.");
      break;

  }

  CRC();

}
bool checkError(unsigned int frame)
{
  if (frame != 0)
  {
    unsigned long long canXOR = 0x80000000;//ตั้งใหญ่ๆไว้ก่อนเพื่อเอามาเช็คขนาดดาต้า
    unsigned long long remainder = frame;
    unsigned long divisor = B110101;//กำหนด divisor
    unsigned long long tmp = canXOR & remainder;//ไว้ตรวจว่าจะเอา remainder ไป or ตรงไหนใน data(ต้อง XOR ตัวหน้าสุดก่อน)
    while (tmp == 0)
    {
      canXOR >>= 1;//ปรับขนาดให้เท่ากับดาต้า
      tmp = canXOR & remainder;
    }

    tmp = canXOR & divisor;//ไว้ตรวจว่าจะเริ่ม XOR ตำแหน่งไหน
    while (tmp == 0)
    {
      divisor <<= 1;//ชิพไปเรื่อยๆจนกว่าจะถึงตำแหน่ง XOR แรก
      tmp = canXOR & divisor;
    }
    while (divisor >= B110101)//ทำจนกว่า divisor จะน้อยกว่าค่าที่กำหนด
    {
      tmp = remainder & canXOR;//ดูว่าตำแหน่งนี้ XOR ได้หรือไม่(XOR ตำแหน่งที่เป็น 1xxxxx)
      if (tmp > 0)remainder = remainder ^ divisor;//ทำการ XOR
      divisor >>= 1;
      canXOR >>= 1;
    }

    if (remainder == 0) return false;
    else return true;
  }
}
void sendFSK(int freq, int in_delay) //Config later
{
  int cyc;
  switch (freq)
  {
    case 500:
      cyc = 5;
      break;
    //        case 800:
    //            cyc = 8;
    //            break;
    //        case 1100:
    //            cyc = 11;
    //            break;
    case 1400:
      cyc = 14;
      break;
  }

  for (int sl = 0; sl < cyc; sl++)
  {
    for (int s = 0; s < N; s++) //4 sample/cycle
    {
      dac.setVoltage(S_DAC[s], false);//modify amplitude
      delayMicroseconds(in_delay);
    }
  }
}

void sendFrame(bool isFrame)
{
  if (isFrame)
  {
    if (currentTime != 0) startTime = currentTime;
    else startTime = 0;
    startTimer = true;
  }
  else
  {
    type = 'S';
    data = 0x1ffffff;
  }
  makeFrame();
  
  Serial.print("Sending Frame: ");
  Serial.println(outFrame, BIN);
  // Frame มีทั้งหมด 32 (รวม Parity แล้ว)
//  dac.setVoltage(900, false);
//  delay(20);
  for (int i = 0; i < fullFrameSize; i += 1)
  {
    
    int out = outFrame & 1;
    if (out == 0) sendFSK(f0, delay0);
    //        else if (out == B01) sendFSK(f1, delay1);
    //        else if (out == B10) sendFSK(f2, delay2);
    else if (out == 1) sendFSK(f3, delay3);
    outFrame >>= 1;

  }
//  delay(1000);
  dac.setVoltage(0, false);
}
void receiveFrame() {
  int tmp = analogRead(A0);
//    Serial.println(tmp);
  if ( tmp > r_slope and prev < r_slope and !check_amp ) // check amplitude
  {
    //Serial.println(tmp);
    check_amp = true; // is first max amplitude in that baud
    if ( !check_baud )
    {
      baud_begin = micros();
      bit_check++;
    }
  }

  if (tmp < r_slope and check_baud) {
    if (micros() - baud_begin  > 9900) // full baud
    {
      //Serial.println(count);
      //      if (count > 1 && count < 7) {
      //        count = 5 ;
      //      }
      //      else if (count >= 7 && count < 10) {
      //        count = 8;
      //      }
      //      else if (count >= 10 && count < 13) {
      //        count = 11;
      //      }
      //      else if (count >= 13) {
      //        count = 14;
      //      }
      if (count >= 4 && count < 9)
      {
        count = 5;
      }
      else if (count >= 9)
      {
        count = 14;
      }
      //Serial.println(count);
      
      uint16_t last = (((count - 5) / 9) & 1) << (bit_check);

      inFrame |= last;

      baud_check++;
    
      //Serial.println("BBBB-------");
      
      if (baud_check == 12) // 12 bits
      {
        Serial.print("Receive Frame : ");
        Serial.println(inFrame, BIN); // add two new bits in data
        checkFrame();
//        Serial.flush();
//        radio.setFrequency(frequency);
        inFrame = 0;
        baud_check = 0;
        bit_check = -1;
//        delay(500);
      }
      check_baud = false;
      count = 0;
    }
  }

  if (tmp > r_slope and check_amp) {
    count++;
    //Serial.println(tmp);
    check_baud = true;
    check_amp = false;
  }
  prev = tmp;
  //Serial.println(micros() - baud_begin);
}

//-------------------------------------Servo-------------------------------------//

void servoControl()
{
  if (Serial.available() > 0) {
    int input = Serial.read();
    if (input == 'A'){
      num = 40;
    }
    else if(input == 'B'){
      num = 90;
    }
    else if(input == 'C'){
      num = 144;
    }
    if (x < num){
      while(x<=num){
        myservo.write(x);
        x++;
        if (x == num){
          myservo.write(x);
//          Serial.println(x/10);
        }
        delay(10);
      }
    }
    else if (x > num){
      while(x >= num){
        myservo.write(x);
        x--;
        if (x == num){
          myservo.write(x);
//          Serial.println(x/10);
        }
        delay(10);
       }
    }
    delay(10);
  }
}
//-------------------------------------Timer-------------------------------------//

void timer()
{
  if (startTimer)
    {
        unsigned long long currentTime = millis();
        if (currentTime - startTime > timeOut)
        {
          startTime = currentTime;
          Serial.println("Retransmitting the frame ...");
          sendFrame(true);
        }
    } 
}

//-----------------------------------Timer end-------------------------------------//

void loop()
{

  receiveFrame();
  servoControl();
  timer();
  if (Serial.available() > 0)
  {
    String cameraBit = Serial.readString();
    Serial.print("Data from camera : ");
    Serial.println(cameraBit);
//    if (cameraBit == "A" or cameraBit == "B" or cameraBit == "C")
//    {
//      sendFrame(false);
//    }
    String tmpS = "";
    int j = 0;
    for (int i = 0; i < cameraBit.length(); i++)
    {
      if (cameraBit[i] == ',')
      {
        pictureValueStore[j] = tmpS;
        tmpS = "";
        j++;
      }
      else
      {
        tmpS += cameraBit[i];
      }
    }
    type = 'I';
    for (int i = 0; i < 3; i++)
    {
      String temp = pictureValueStore[i].substring(0, 4);
      String rotateTemp = pictureValueStore[i].substring(4);
      byte value = B0;
      if (rotateTemp == "C")
      {
        temp += "01";
      }
      else if (rotateTemp == "L")
      {
        temp += "00";
      }

      else if (rotateTemp == "R")
      {
        temp += "10";
      }
      
      
      long tmp_data = temp.toInt();
      Serial.println(tmp_data);
//
//    }
//    //        sendFrame(true);
//    delay(1000);

  }
  }


  
//  if (startTimer)
//  {
//    currentTime = millis();
//    if (currentTime - startTime > timeOut)
//    {
//      Serial.println("Retransmitting the frame ...");
//      sendFrame(true);
//    }
//  }
//  else currentTime = 0;

}
