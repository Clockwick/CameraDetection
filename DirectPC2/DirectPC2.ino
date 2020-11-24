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
#include <Vector.h>
#define fullFrameSize 21


#define r_slope 400
// Config later
#define defaultFreq 1700
#define f0 500
#define f1 800
#define f2 1100
#define f3 1400


#define N 4

#define IFrame 7
#define UFrame 4
#define SFrame 6

#define INIT_MODE 0
#define SPLIT_AXIS 1
#define PIXEL_MODE 2




double S[N];
uint16_t S_DAC[N];
Adafruit_MCP4725 dac;
TEA5767 radio = TEA5767();

bool canSend = true;
bool intialize = true;
bool startTimer = false;
bool receivedAck = false;
bool boolFrame = false;
bool boolAck = true;

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


int queueIndex = 0;


int prev = 0;
int count = 0;

uint16_t baud_check = 0;

uint16_t bit_check = -1;

bool check_amp = false;
bool check_baud = false;

const float frequency = 89.6; //Enter your own Frequency // old wave = 89.7

uint32_t baud_begin = 0;

String pictureStore[3] = {"1011L", "1010C", "1001R"};
int pictureIndex = 0;
//String pictureStore[3];

//Servo setup

int x = 0;
Servo myservo;
int num = 40;

uint32_t storage_array[50];
Vector<uint32_t> vector(storage_array);
//End
void setup() {
  Serial.begin(115200);
  dac.begin(0x64);
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

  radio.setFrequency(frequency);
  // Servo init
  myservo.attach(9); // กำหนดขา 9 ควบคุม Servo
  if (x < num) {
    while (x <= num) {
      myservo.write(x);
      x++;
      if (x == num) {
        myservo.write(x);
      }
      delay(10);
    }
  }
  else if (x > num) {
    while (x >= num) {
      myservo.write(x);
      x--;
      if (x == num) {
        myservo.write(x);
      }
      delay(10);
    }
  }
  // Servo end

  Serial.flush();
  //    delay(2000);
}

void checkFrame()
{
  if (checkError(inFrame) == false) // No error
  {
    int inFrameType = inFrame >> 10;
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
    else if (inFrameType == SFrame)
    {
      uint8_t tmp = (inFrame >> 6) & B1111;
//      Serial.print("Receive ack Number : ");
//      Serial.println(tmp, HEX);
//      Serial.println(tmp, BIN);
      if (tmp == 0x9)
      {
        Serial.println("Received ack.");
        startTimer = false;
        boolFrame = !boolFrame;
        canSend = true;
        if (mode == SPLIT_AXIS) prepareCapture();
      }
    }

    else if (inFrameType == IFrame)
    {
      Serial.print("Received I-Frame : ");
      Serial.println(inFrame >> 6, BIN);
      int cmd = (inFrame >> 6) & B1111;
      if (cmd == B1111) captureAll();
//      sendFrame(false);
    }
  }
  else
  {
    Serial.println("Error detected in data");
  }
}

void prepareCapture()
{
  if (pictureIndex < 3) {
    type = 'I';
    String colorCode = pictureStore[pictureIndex].substring(0, 4);
    Serial.print("Color code : ");
    Serial.println(colorCode);
    String rotate = pictureStore[pictureIndex].substring(4);
    uint32_t tempData = 0;
    if (colorCode == "0000") tempData = 0;
   
    else if (colorCode == "0001") tempData = 1;
   
    else if (colorCode == "0010") tempData = 2;
  
    else if (colorCode == "0011") tempData = 3;
    
    else if (colorCode == "0100") tempData = 4;
   
    else if (colorCode == "0101") tempData = 5;
    
    else if (colorCode == "0110") tempData = 6;
    
    else if (colorCode == "0111") tempData = 7;
    
    else if (colorCode == "1000") tempData = 8;
    
    else if (colorCode == "1001") tempData = 9;
    
    else if (colorCode == "1010") tempData = 10;
    
    else if (colorCode == "1011") tempData = 11;
   
    else if (colorCode == "1100") tempData = 12;
    
    else if (colorCode == "1101") tempData = 13;
    
    else if (colorCode == "1110") tempData = 14;
   
    else if (colorCode == "1111") tempData = 15;
    
    if (rotate == "L")
    {
      tempData <<= 2;
      tempData |= B00;
    }
    else if (rotate == "C")
    {
      tempData <<= 2;
      tempData |= B01;
    }
    else if (rotate == "R")
    {
      tempData <<= 2;
      tempData |= B10;
    }
    
    tempData *= 64;
    data = tempData;
    sendFrame(true);
    pictureIndex++;
  }
  else mode = PIXEL_MODE;

}

void captureAll()
{
  int i = 0;
  int cameraBit = 0;
  Serial.flush();
  Serial.println(" - - Capture mode - - ");
  Serial.print("g"); // to communicate with camera
//  mode = SPLIT_AXIS;
//  prepareCapture();
  //  sendFrame(true);
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
  /*
    Frame
    size : 21
    type: U-frame, S-Frame, I-frame
    U-Frame = [   100    |  00000...(13ตัว) | 00000]
                Type         cmd             CRC
    S-Frame = [   111    |     1000...(12ตัว)     |   0  | 00000 ]
               Type             cmd               ackNo   CRC
    I-Frame = [   110   | 110011...(12ตัว) |    0   | 00000 ]
               Type            Data         FrameNo    CRC
  */
  outFrame = 0;
  switch (type)
  {
    case 'I':
      outFrame = IFrame;
      outFrame <<= 12;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= frameNo;
      break;
    case 'S':
      outFrame = SFrame;
      outFrame <<= 12;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= ackNo;
      break;

    case 'U':
      outFrame = UFrame;
      outFrame <<= 13;
      outFrame |= data;
      break;

    default:
      Serial.println("Please defined type of the frame.");
      break;

  }
  //    Serial.print("Before CRC : ");
  //    Serial.println(outFrame, BIN);
  CRC();
  //    Serial.print("After CRC : ");
  //    Serial.println(outFrame, BIN);
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
    startTimer = true;
  }
  else
  {
    type = 'S';
    data = 0x801;
  }
  switch (boolFrame)
  {
    case true:
      frameNo = 1;
      break;
    case false:
      frameNo = 0;
      break;
  }
  makeFrame();
  Serial.print("Sending Frame ");
  Serial.print("{");
  Serial.print(type);
  Serial.print("} : ");
  Serial.println(outFrame, BIN);
  // Frame มีทั้งหมด 16 (ขนาดเฟรม) + 5 (บิต CRC) = 21
  for (int i = 0; i < fullFrameSize; i ++)
  {
    int out = outFrame & 1;
    if (out == 0) sendFSK(f0, delay0);
    else if (out == 1) sendFSK(f3, delay3);
    outFrame >>= 1;
  }
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
      if (count >= 4 && count < 9) count = 5;
      else if (count >= 9) count = 14;

      uint16_t last = (((count - 5) / 9) & 1) << (bit_check);

      inFrame |= last;

      baud_check++;

      //Serial.println("BBBB-------");

      if (baud_check == 13) // 13 bits
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
    if (input == 'A') {
      num = 40;
    }
    else if (input == 'B') {
      num = 90;
    }
    else if (input == 'C') {
      num = 144;
    }
    if (x < num) {
      while (x <= num) {
        myservo.write(x);
        x++;
        if (x == num) {
          myservo.write(x);
          //          Serial.println(x/10);
        }
        delay(10);
      }
    }
    else if (x > num) {
      while (x >= num) {
        myservo.write(x);
        x--;
        if (x == num) {
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
    if (Serial.available() > 0 and mode == INIT_MODE)
    {
      String cameraBit = Serial.readString();
      Serial.print("Data from camera : ");
      Serial.println(cameraBit);
  
      String tmpS = "";
      int j = 0;
      for (int i = 0; i < cameraBit.length(); i++)
      {
        if (cameraBit[i] == ',')
        {
          pictureStore[j] = tmpS;
          tmpS = "";
          j++;
        }
        else
        {
          tmpS += cameraBit[i];
        }
      }
      for (int i = 0; i < 3; i++)
      {
        String temp = pictureStore[i].substring(0, 4);
        String rotateTemp = pictureStore[i].substring(4);
        if (rotateTemp == "C") temp += "01";
        else if (rotateTemp == "L") temp += "00";
        else if (rotateTemp == "R") temp += "10";
      }
      prepareCapture();
      mode = SPLIT_AXIS;
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
