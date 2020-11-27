
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <TEA5767.h>

/********************************************FSK********************************************/

#define r_slope 400
// Config later
#define defaultFreq 1700
#define f0 500
#define f3 1400

#define N 4

#define IFrame 7
#define UFrame 4
#define SFrame 6

#define fullFrameSize 13
#define receiveFullFrameSize 21

// Mode ฝั่ง PC1
#define CAPTURE_MODE 0
#define DISPLAY_MODE 1
#define ANALYSIS_MODE 2
#define DECISION_MODE 3
#define RESTART_MODE 4
#define TERMINATE_MODE 5

// Dac
double S[N];
uint16_t S_DAC[N];
Adafruit_MCP4725 dac;
TEA5767 radio = TEA5767();

// VAR
bool canSend = true;
bool initialize = true;
bool startTimer = false;
bool receivedAck = false;
bool startCapture = true;
bool boolFrame = false;
bool boolAck = true;


int delay0, delay3;
char type = 'O';
String cmd = "";
int mode = CAPTURE_MODE;
unsigned int frameNo = 0, ackNo = 0;
int countPic = 0;
String pictureStore[3];

unsigned long startTime = 0, timeOut = 20000;
uint16_t outFrame = 0;
int prev = 0;
int count = 0;
int pictureIndex = 0;

int baud_check = 0;
uint32_t data = 0;
int bit_check = -1;

int test[21];
uint32_t inFrame = 0;
bool check_amp = false;
bool check_baud = false;

const float frequency = 102.4; //Enter your own Frequency

uint32_t baud_begin = 0;

// เก็บข้อมูล x,y, grayscale ที่ส่งมาจาก PC2
String quadrant1[5];
int qIndex1 = 0;
String quadrant2[5];
int qIndex2 = 0;
String quadrant3[5];
int qIndex3 = 0;
String quadrant4[5];
int qIndex4 = 0;
String X[5];
int xIndex = 0;
String Y[5];
int yIndex = 0;


void setup() {
  Serial.begin(115200);
  dac.begin(0x60);
  Wire.begin();

  for (int i = 0; i < 21; i++) test[i] = 0;

  for (int i = 0; i < N; i++) {
    S[i] = sin(2 * PI * (i / (double)N));
    S_DAC[i] = 4095 / 2.0 * (1 + S[i]);
  }

  delay0 = (1000000 / f0 - 1000000 / defaultFreq) / N;
  delay3 = (1000000 / f3 - 1000000 / defaultFreq) / N;
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  pinMode(A0, INPUT);

  // เช็ทค่า frequency ที่ต้องการให้ radio รับ
  radio.setFrequency(frequency);
  Serial.flush();
}
// Reset ค่าทุกอย่างเป็นค่าเริ่มต้น
void flushData()
{
  canSend = true;
  initialize = true;
  startTimer = false;
  receivedAck = false;
  startCapture = true;
  boolFrame = false;
  boolAck = true;

  type = 'O';
  cmd = "";
  mode = CAPTURE_MODE;
  frameNo = 0;
  ackNo = 0;
  countPic = 0;
  for (int i = 0; i < 3; i++) pictureStore[i] = "";

  startTime = 0;
  timeOut = 10000;
  outFrame = 0;
  prev = 0;
  count = 0;
  pictureIndex = 0;

  baud_check = 0;
  data = 0;
  bit_check = -1;

  clearInFrame();
  inFrame = 0;
  check_amp = false;
  check_baud = false;

  baud_begin = 0;

  for (int i = 0; i < 5; i++)quadrant1[i] = "";
  qIndex1 = 0;
  for (int i = 0; i < 5; i++)quadrant2[i] = "";
  qIndex2 = 0;
  for (int i = 0; i < 5; i++)quadrant3[i] = "";
  qIndex3 = 0;
  for (int i = 0; i < 5; i++)quadrant4[i] = "";
  qIndex4 = 0;
  for (int i = 0; i < 5; i++)X[i] = "";
  xIndex = 0;
  for (int i = 0; i < 5; i++)Y[i] = "";
  yIndex = 0;
}

// สำหรับส่ง Frame ไป PC2
void sendFrame(bool isFrame)
{
  if (isFrame)
  {
    startTimer = true;
    startTime = millis();
  }
  else
  {
    type = 'S';
    data = 0x9;
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
  // Frame มีทั้งหมด 8 (ขนาดเฟรม) + 5 (บิต CRC) = 13
  for (int i = 0; i < fullFrameSize; i += 1)
  {
    int out = outFrame & 1;
    if (out == 0) sendFSK(f0, delay0);
    else if (out == 1) sendFSK(f3, delay3);
    outFrame >>= 1;
  }
  dac.setVoltage(0, false);
}

void sendFSK(int freq, int in_delay)
{
  int cyc;
  switch (freq)
  {
    case 500: //500
      cyc = 5;
      break;

    case 1400: //1400
      cyc = 14;
      break;
  }
  for (int sl = 0; sl < cyc; sl++)
  {
    for (int s = 0; s < N; s++)
    {
      dac.setVoltage(S_DAC[s], false);
      delayMicroseconds(in_delay);
    }
  }

}


void makeFrame()
{
  outFrame = 0;
  /*
    Frame + CRC
    Frame size: 13 bits
    type: U-frame, S-Frame, I-frame
    U-Frame = [   100    |  00000 | 00000]
                 Type       cmd      CRC
    S-Frame = [   111    |  0000 | 0 | 00000]
               Type         cmd  ackNo  CRC
    I-Frame = [   110    |   0000   |    0     | 00000 ]
               Type          Data     FrameNo    CRC
  */
  switch (type)
  {
    case 'I':
      outFrame = IFrame;
      outFrame <<= 4;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= frameNo;
      break;
    case 'S':
      outFrame = SFrame;
      outFrame <<= 4;
      outFrame |= data;
      outFrame <<= 1;
      outFrame |= ackNo;
      break;
    case 'U':
      outFrame = UFrame;
      outFrame <<= 5;
      outFrame |= data;
      break;
    default:
      Serial.println("Please defined type of the frame.");
      break;

  }
  CRC();
}

void CRC()
{
  if (outFrame != 0)
  {
    unsigned long long canXOR = 0x800000000;// ตัวเช็คขนาด data
    unsigned long long remainder = outFrame << 5;//ตัวแปรใหม่ที่มาจากการเติม 0 หลัง outFrame 5 ตัว
    unsigned long divisor = B101100;// Divisor
    unsigned long long tmp = canXOR & remainder;// เช็คตำแหน่งการ or ของ remainder กับ data
    while (tmp == 0)
    {
      canXOR >>= 1;
      tmp = canXOR & remainder;
    }
    tmp = canXOR & divisor;
    while (tmp == 0)
    {
      divisor <<= 1;
      tmp = canXOR & divisor;
    }
    while (divisor >= B101100)
    {
      tmp = remainder & canXOR;
      if (tmp > 0)remainder = remainder ^ divisor;// XOR
      divisor >>= 1;
      canXOR >>= 1;
    }
    outFrame <<= 5; //เติม0 5ตัว
    outFrame += remainder; //เปลี่ยน5บิตสุดท้ายเป็นremainder
  }
}
bool checkError(uint32_t frame)
{
  // เช็ค CRC , Remainder ต้อง = 0 ข้อมูลที่ได้รับจึงจะถูกต้อง
  if (frame != 0)
  {
    unsigned long long canXOR = 0x800000000;
    unsigned long long remainder = frame;
    unsigned long divisor = B101100;
    unsigned long long tmp = canXOR & remainder;
    while (tmp == 0)
    {
      canXOR >>= 1;
      tmp = canXOR & remainder;
    }
    tmp = canXOR & divisor;
    while (tmp == 0)
    {
      divisor <<= 1;
      tmp = canXOR & divisor;
    }
    while (divisor >= B101100)
    {
      tmp = remainder & canXOR;
      if (tmp > 0)remainder = remainder ^ divisor; // XOR
      divisor >>= 1;
      canXOR >>= 1;
    }
    if (remainder == 0) return false;
    else return true;
  }
}
void checkFrame()
{
  if (checkError(inFrame) == false) // No error
  {
    int inFrameType = (inFrame >> 18);
    String t = "";
    switch (inFrameType)
    {
      case 7:
        t = "I";
        break;
      case 6:
        t = "S";
        break;
      case 4:
        t = "U";
        break;
    }
    Serial.print("Receive Frame {");
    Serial.print(t);
    Serial.print("} : ");
    Serial.println(inFrame, BIN);
    if (inFrameType == SFrame)
    {

      uint32_t tmp = (inFrame >> 6) & 0x1fff;
      if (tmp == 0x801) // ถ้า unpack เฟรมออกมาแล้วได้ค่า 0x801 คือแสดงว่าได้รับ ack ที่ถูกต้อง
      {
        Serial.println("Received ack.");
        startTimer = false;
        boolFrame = !boolFrame;
        canSend = true;
        if (mode == CAPTURE_MODE) // เริ่มต้นการโหมดถ่ายรูป 3 มุม
        {
          Serial.println("/*--------------------- The connection has been established. ---------------------*/");
          Serial.println("Type \"capture\" or \"c\" to start Capturing.");
          mode = DISPLAY_MODE;
        }

        else if (mode == ANALYSIS_MODE) // เริ่มต้นโหมดวิเคราะห์รูปภาพ
        {
          Serial.println("/*--------------------- Analysis mode ---------------------*/");
        }
      }


    }
    if (inFrameType == UFrame) //U-Frame
    {
      // For terminated connection .
      uint32_t tmp = (inFrame >> 5) & 0x1ffffff;

    }
    else if (inFrameType == IFrame) //I-Frame
    {
      delay(10);
      sendFrame(false); // ส่ง ack กลับ
      
      /* unpack frame เอาข้อมูลมาเก็บไว้ใน array ฝั่ง PC1 */
      if (mode == DISPLAY_MODE)
      {
        inFrame &= 0x3f000;
        inFrame /= 4096;
      }
      else if (mode == ANALYSIS_MODE)
      {
        inFrame &= 0xfffe0;
        inFrame /= 64;
        inFrame &= 0x3ff;
      }


      switch (mode)
      {
        /*เก็บค่ารหัสมุมต่างๆที่ได้รับมาจาก PC2 */
        case DISPLAY_MODE:
          pictureStore[pictureIndex] = String(inFrame, BIN);
          pictureIndex++;
          break;
        /*เก็บค่า x,y, grayscle ที่ได้รับมาจาก PC2 */
        case ANALYSIS_MODE:
          String readyInFrame = String(inFrame, BIN);
          addZeroTo12Bit(&readyInFrame);
          String number = convertToInt(readyInFrame);
          if (qIndex1 < 5)
          {
            quadrant1[qIndex1] = number;
            qIndex1++;
          }
          else if (qIndex2 < 5)
          {
            quadrant2[qIndex2] = number;
            qIndex2++;
          }
          else if (qIndex3 < 5)
          {
            quadrant3[qIndex3] = number;
            qIndex3++;
          }
          else if (qIndex4 < 5)
          {
            quadrant4[qIndex4] = number;
            qIndex4++;
          }
          else if (xIndex < 5)
          {
            X[xIndex] = number;
            xIndex++;
          }
          else if (yIndex < 5)
          {
            Y[yIndex] = number;
            yIndex++;
          }
          break;

      }
      if (pictureIndex == 3 and mode == DISPLAY_MODE) //ได้รับข้อมูลครบแล้ว แสดงผลบน Serial Monitor
      {
        for (int i = 0 ; i < pictureIndex; i++) addZero(&pictureStore[i]);
        displayValue();
      }

      if (yIndex == 4 and mode == ANALYSIS_MODE) //ได้รับข้อมูลครบแล้ว แสดงผลบน Serial Monitor
      {
        mode = DECISION_MODE;
        displayPixel();
      }


    }
  }
  else
  {
    Serial.println("Error detected in data");
  }
}

void displayValue()
{
  Serial.println("/*--------------------- Display ---------------------*/");
  for (int i = 0; i < sizeof(pictureStore) / sizeof(pictureStore[0]); i++)
  {
    String checkDirection = "";
    for (int j = 4; j < pictureStore[i].length(); j++) checkDirection += pictureStore[i][j];
    if (checkDirection == "00")
    {
      Serial.print("Left Picture is ");
      for (int k = 0; k < 4; k++) Serial.print(pictureStore[i][k]);
      Serial.println();
    }
    else if (checkDirection == "01")
    {
      Serial.print("Center Picture is ");
      for (int k = 0; k < 4; k++) Serial.print(pictureStore[i][k]);
      Serial.println();
    }
    else if (checkDirection == "10")
    {
      Serial.print("Right Picture is ");
      for (int k = 0; k < 4; k++) Serial.print(pictureStore[i][k]);
      Serial.println();
    }
  }
  mode = ANALYSIS_MODE;
  canSend = true;

}
void addZeroTo12Bit(String* str) // Format รูปแบบ
{
  int sizeStr = str->length();
  String preset = "";
  for (int i = 0; i < 12 - sizeStr; i++)
  {
    preset += "0";
  }
  *str = preset + *str;
}
void addZero(String* str) // Format รูปแบบ
{
  int sizeStr = str->length();
  String preset = "";
  for (int i = 0; i < 6 - sizeStr; i++)
  {
    preset += "0";
  }
  *str = preset + *str;
}


void clearInFrame() 
{
  for (int i = 0; i < receiveFullFrameSize; i++) test[i] = 0;
}

String convertToInt(String str) // Format รูปแบบ
{
  String firstPart = "";
  String secPart = "";
  String thirdPart = "";

  for (int a = 0; a < 4; a++) firstPart += str[a];

  for (int b = 4; b < 8; b++) secPart += str[b];

  for (int c = 8; c < 12; c++) thirdPart += str[c];

  String total = strToInt(firstPart) + strToInt(secPart) + strToInt(thirdPart);

  return total;

}

String strToInt(String colorCode) // Format รูปแบบ
{
  String tempData = "";

  if (colorCode == "0000") tempData = "0";

  else if (colorCode == "0001") tempData = "1";

  else if (colorCode == "0010") tempData = "2";

  else if (colorCode == "0011") tempData = "3";

  else if (colorCode == "0100") tempData = "4";

  else if (colorCode == "0101") tempData = "5";

  else if (colorCode == "0110") tempData = "6";

  else if (colorCode == "0111") tempData = "7";

  else if (colorCode == "1000") tempData = "8";

  else if (colorCode == "1001") tempData = "9";

  return tempData;

}

void displayPixel() // แสดงผลค่า x,y , grayscale
{
  sendFrame(false);
  for (int i = 0; i < 4; i ++)
  {
    int intX = X[i].toInt() - 500;
    int intY = Y[i].toInt() - 500;

    Serial.print("X : ");
    Serial.println(intX);
    Serial.print("Y : ");
    Serial.println(intY);
  }
  Serial.println(" ---------- Quadrant 1 ---------- ");
  formatQ(quadrant1[0], quadrant1[1], quadrant1[2], quadrant1[3], quadrant1[4]);
  Serial.println(" ---------- Quadrant 2 ---------- ");
  formatQ(quadrant2[0], quadrant2[1], quadrant2[2], quadrant2[3], quadrant2[4]);
  Serial.println(" ---------- Quadrant 3 ---------- ");
  formatQ(quadrant3[0], quadrant3[1], quadrant3[2], quadrant3[3], quadrant3[4]);
  Serial.println(" ---------- Quadrant 4 ---------- ");
  formatQ(quadrant4[0], quadrant4[1], quadrant4[2], quadrant4[3], quadrant4[4]);
  canSend = true;
  flushData();
  type = 'U';
  data = B00110;
  sendFrame(true);
  initialize = false;
}

void formatQ( String c1, String c2, String c3, String c4, String mean)
{
  int intC1 = c1.toInt() - 100;
  int intC2 = c2.toInt() - 100;
  int intC3 = c3.toInt() - 100;
  int intC4 = c4.toInt() - 100;
  int intMean = mean.toInt() - 100;

  Serial.print("(");
  Serial.print(intC1);
  Serial.print(", ");
  Serial.print(intC2);
  Serial.print(", ");
  Serial.print(intC3);
  Serial.print(", ");
  Serial.print(intC4);
  Serial.print(")");
  Serial.print(" | Mean : ");
  Serial.println(intMean);
}
void receiveFrame() {
  int tmp = analogRead(A0);

  if (tmp > r_slope and prev < r_slope and !check_amp ) // check amplitude
  {
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

      int onebit = ((count - 5) / 9);

      test[20 - baud_check] = onebit;

      baud_check++;
      if (baud_check == receiveFullFrameSize) // 21 bits
      {
        for (int i = 0; i < receiveFullFrameSize; i++)
        {
          inFrame <<= 1;
          inFrame |= test[i];
        }
        checkFrame();
        Serial.flush();
        radio.setFrequency(frequency);
        inFrame = 0;
        clearInFrame();
        baud_check = 0;
        bit_check = -1;
      }

      check_baud = false;
      count = 0;
    }
  }

  if (tmp > r_slope and check_amp) {
    count++;
    check_baud = true;
    check_amp = false;
  }
  prev = tmp;

}

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


void loop()
{

  receiveFrame();
  timer();
  if (Serial.available() > 0 && canSend == true)
  {

    cmd = Serial.readString();
    if (initialize && cmd.equalsIgnoreCase("start") || cmd.equalsIgnoreCase("s")) // เริ่มต้นการทำงาน
    {
      type = 'U';
      data = B00110;
      sendFrame(true);
      initialize = false;
    }

    else if (cmd.equalsIgnoreCase("end") || cmd.equalsIgnoreCase("e")) // จบการทำงาน
    {
      Serial.println("---- Terminate ----");
      type = 'U';
      data = B00111;
      sendFrame(true);
    }
    else if (cmd.equalsIgnoreCase("restart") || cmd.equalsIgnoreCase("r")) // เริ่มต้นการทำงานใหม่
    {
      Serial.println("---- Restart ----");
      type = 'U';
      data = B00001;
      sendFrame(true);
    }
    else
    {
      if (mode == CAPTURE_MODE && cmd.equalsIgnoreCase("capture") || cmd.equalsIgnoreCase("c")) // เริ่มต้นการทำงานโหมด Capture (ถ่ายภาพ 3 มุม)
      {
        data = B1111;
        mode = DISPLAY_MODE;
      }
      else if (mode == ANALYSIS_MODE) // Map data ใส่เฟรมส่งไป PC2 เพื่อเริ่มวิเคราะห์รหัสค่าที่กรอก
      {
        // The most preformance code in the world.
        if (cmd == "0000") data = 0;
        else if (cmd == "0001") data = 1;
        else if (cmd == "0010") data = 2;
        else if (cmd == "0011") data = 3;
        else if (cmd == "0100") data = 4;
        else if (cmd == "0101") data = 5;
        else if (cmd == "0110") data = 6;
        else if (cmd == "0111") data = 7;
        else if (cmd == "1000") data = 8;
        else if (cmd == "1001") data = 9;
        else if (cmd == "1010") data = 10;
        else if (cmd == "1011") data = 11;
        else if (cmd == "1100") data = 12;
        else if (cmd == "1101") data = 13;
        else if (cmd == "1110") data = 14;
        else if (cmd == "1111") data = 15;
      }
      type = 'I';
      sendFrame(true);
      canSend = false;
      boolFrame = !boolFrame;
    }

  }




}
